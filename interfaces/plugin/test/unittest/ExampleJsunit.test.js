/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
import sensor from '@ohos.sensor'

import {describe, beforeAll, beforeEach, afterEach, afterAll, it, expect} from 'deccjsunit/index'

describe("SensorJsTest", function () {
    beforeAll(function() {
        /*
         * @tc.setup: setup invoked before all testcases
         */
         console.info('beforeAll caled')
    })
    
    afterAll(function() {
        /*
         * @tc.teardown: teardown invoked after all testcases
         */
         console.info('afterAll caled')
    })
    
    beforeEach(function() {
        /*
         * @tc.setup: setup invoked before each testcases
         */
         console.info('beforeEach caled')
    })
    
    afterEach(function() {
        /*
         * @tc.teardown: teardown invoked after each testcases
         */
         console.info('afterEach caled')
    })

    /*
     * @tc.name:SensorJsTest001
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest001", 0, async function (done) {
        console.info('----------------------SensorJsTest001---------------------------');
        function offPromise() {
            return new Promise((resolve, reject) => {
                    sensor.off(0, (error) =>{
                        if(error) {
                            console.info('SensorJsTest001  off error');
                            expect(false).assertTrue();
                            console.info('setTimeout ..start')
                            setTimeout(()=>{
                                reject();
                            }, 500);
                        } else {
                            console.info('SensorJsTest001  off success');
                            expect(true).assertTrue();
                            setTimeout(()=>{
                                resolve();
                            }, 500);
                        }
                }, 1000)
            })
        }

        let promise = new Promise((resolve, reject) => {
            sensor.on(0, function(error, data) {
                if (error) {
                    console.info('SensorJsTest001  on error');
                    expect(false).assertTrue();
                    setTimeout(()=>{
                        reject();
                    }, 500);
                } else {
                    console.info('SensorJsTest001  on success');
                    expect(typeof(data.x)).assertEqual("number");
                    setTimeout(()=>{
                        resolve();
                    }, 500);
                }
            });
        })

        await promise.then(() =>{
            return offPromise();
        }, ()=>{
            console.info("SensorJsTest001 reject");
        })
        done();
    })

    /*
     * @tc.name:SensorJsTest002
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest002", 0, async function (done) {
        console.info('----------------------SensorJsTest002---------------------------');
        function onSensorCallback(error, data) {
            if (error) {
                console.info('SensorJsTest002  on success');
                expect(true).assertTrue();
            } else {
                console.info('SensorJsTest002  on error');
                expect(false).assertTrue();
            }
            setTimeout(()=>{
                done();
            }, 500);
        }
        sensor.on(-1, onSensorCallback);
    })

    /*
     * @tc.name:SensorJsTest003
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest003", 0, async function (done) {
        console.info('----------------------SensorJsTest003---------------------------');
        function offPromise() {
            return new Promise((resolve, reject) => {
                    sensor.off(0, (error) =>{
                        if(error) {
                            console.info('SensorJsTest003  off error');
                            expect(false).assertTrue();
                            setTimeout(()=>{
                                done();
                                reject();
                            }, 500);
                        } else {
                            console.info('SensorJsTest003  off success');
                            expect(true).assertTrue();
                            setTimeout(()=>{
                                done();
                                resolve();
                            }, 500);
                        }
                }, 1000)
            })
        }

        let promise = new Promise((resolve, reject) => {
            sensor.on(0, function(error, data) {
                if (error) {
                    console.info('SensorJsTest003  on error');
                    expect(false).assertTrue();
                    setTimeout(()=>{
                        reject();
                    }, 500);
                } else {
                    console.info('SensorJsTest003  on success');
                    expect(typeof(data.x)).assertEqual("number");
                    setTimeout(()=>{
                        resolve();
                    }, 500);
                }
            }, {'interval': 200000000});
        })

        await promise.then(() =>{
            return offPromise();
        }, ()=>{
            console.info("SensorJsTest003 reject");
        })
        done();
    })

    /*
     * @tc.name:SensorJsTest004
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest004", 0, function () {
        console.info('----------------------SensorJsTest004---------------------------');
        sensor.on(0, function(){}, {'interval': 100000000}, 5);
        expect(true).assertTrue();
        console.info('----------------------SensorJsTest004--------------------------- end');
    })

    /*
     * @tc.name:SensorJsTest005
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest005", 0, async function (done) {
        function onceSensorCallback(error, data) {
            if (error) {
                console.info('SensorJsTest005  once error');
                expect(false).assertTrue();
            } else {
                console.info('SensorJsTest005  once success');
                expect(typeof(data.x)).assertEqual("number");
            }
            setTimeout(()=>{
                done();
            }, 500);
        }
        sensor.once(0, onceSensorCallback);
    })

    /*
     * @tc.name:SensorJsTest006
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest006", 0, async function (done) {
        function onceSensorCallback(error, data) {
            if (error) {
                console.info('SensorJsTest006  on success');
                expect(true).assertTrue();
            } else {
                console.info('SensorJsTest006  on error' + data.x);
                expect(false).assertTrue();
            }
            setTimeout(()=>{
                done();
            }, 500);
        }
        sensor.once(-1, onceSensorCallback);
    })

    /*
     * @tc.name:SensorJsTest007
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest007", 0, function () {
        sensor.once(0, function(){}, 5);
        expect(true).assertTrue();
    })

    /*
     * @tc.name:SensorJsTest008
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest008", 0, async function (done) {
        function offCallback(error) {
            if (error) {
                console.info('SensorJsTest008  off success');
                expect(true).assertTrue();
            } else {
                console.info('SensorJsTest008  off error');
                expect(false).assertTrue();
            }
            setTimeout(()=>{
                done();
            }, 500);
        }
        sensor.off(-1, offCallback);
    })

    /*
     * @tc.name:SensorJsTest009
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest009", 0, async function (done) {
        function offCallback(error) {
            if (error) {
                console.info('SensorJsTest009  off success');
                expect(true).assertTrue();
            } else {
                console.info('SensorJsTest009  off error');
                expect(false).assertTrue();
            }
            setTimeout(()=>{
                done();
            }, 500);
        }
        sensor.off(0, offCallback);
    })

    /*
     * @tc.name:SensorJsTest010
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest010", 0, async function (done) {
        function offCallback(error) {
            if (error) {
                console.info('SensorJsTest010  off success');
                expect(true).assertTrue();
            } else {
                console.info('SensorJsTest010  off error');
                expect(false).assertTrue();
            }
            setTimeout(()=>{
                done();
            }, 500);
        }
        sensor.off(1000000, offCallback);
    })

    /*
     * @tc.name:SensorJsTest011
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest011", 0, async function (done) {
        function onceSensorCallback(error, data) {
            if (error) {
                console.info('SensorJsTest011  once success');
                expect(true).assertTrue();
            } else {
                console.info('SensorJsTest011  once error' + data.x);
                expect(false).assertTrue();
            }
            setTimeout(()=>{
                done();
            }, 500);
        }
        sensor.once(1000000, onceSensorCallback);
    })

    /*
     * @tc.name:SensorJsTest012
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest012", 0, async function (done) {
        function offCallback(error) {
            if (error) {
                console.info('SensorJsTest012  off success');
                expect(true).assertTrue();
            } else {
                console.info('SensorJsTest012  off error');
                expect(false).assertTrue();
            }
            setTimeout(()=>{
                done();
            }, 500);
        }
        sensor.on(1000000, offCallback);
    })

    /*
     * @tc.name:SensorJsTest013
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest013", 0, function () {
        sensor.off(0, 5);
        expect(true).assertTrue();
    })


    /*
     * @tc.name:SensorJsTest014
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest014", 0, async function (done) {
        console.info('----------------------SensorJsTest014---------------------------');
        function offPromise2() {
            return new Promise((resolve, reject) => {
                    sensor.off(0, (error) =>{
                        if(error) {
                            console.info('SensorJsTest014  off2 success');
                            expect(true).assertTrue();
                            setTimeout(()=>{
                                done();
                                reject()
                            }, 500);
                        } else {
                            console.info('SensorJsTest014  off2 error');
                            expect(false).assertTrue();
                            setTimeout(()=>{
                                done();
                                resolve()
                            }, 500);
                        }
                    });
            })
        }

        function offPromise1() {
            return new Promise((resolve, reject) => {
                    sensor.off(0, (error) =>{
                        if(error) {
                            console.info('SensorJsTest014  off1  error');
                            expect(false).assertTrue();
                            setTimeout(()=>{
                                reject();
                            }, 500);
                        } else {
                            console.info('SensorJsTest014  off1  success');
                            expect(true).assertTrue();
                            setTimeout(()=>{
                                resolve();
                            }, 500);
                        }
                    });
            })
        }

        let promise = new Promise((resolve, reject) => {
            sensor.on(0, function(error, data) {
                if (error) {
                    console.info('SensorJsTest014  on error');
                    expect(false).assertTrue();
                    setTimeout(()=>{
                        reject();
                    }, 500);
                } else {
                    console.info('SensorJsTest014  on success');
                    expect(typeof(data.x)).assertEqual("number");
                    setTimeout(()=>{
                        resolve();
                    }, 500);
                }
            });
        })

        await promise.then(() =>{
            return offPromise1();
        }).then(()=>{
            return offPromise2();
        });
        done();
    })

    /*
     * @tc.name:SensorJsTest015
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest015", 0, async function (done) {
        console.info('----------------------SensorJsTest015---------------------------');
        function offPromise() {
            return new Promise((resolve, reject) => {
                sensor.off(0, (error) =>{
                    if(error) {
                        console.info('SensorJsTest015  off error');
                        expect(false).assertTrue();
                        setTimeout(()=>{
                            done();
                            reject();
                        }, 500);
                    } else {
                        console.info('SensorJsTest015  off success');
                        expect(true).assertTrue();
                        setTimeout(()=>{
                            done();
                            resolve();
                        }, 500);
                    }
                });
            })
        }
        function onPromise2() {
            return new Promise((resolve, reject) => {
                sensor.on(0, function(error, data) {
                    if (error) {
                        console.info('SensorJsTest015  on2 error');
                        expect(false).assertTrue();
                        setTimeout(()=>{
                            reject();
                        }, 500);
                    } else {
                        console.info('SensorJsTest015  on2 success');
                        expect(typeof(data.x)).assertEqual("number");
                        setTimeout(()=>{
                            resolve();
                        }, 500);
                    }
                });
            })
        }

        let onPromise1 = new Promise((resolve, reject) => {
            sensor.on(0, function(error, data) {
                if (error) {
                    console.info('SensorJsTest015  on1 error');
                    expect(false).assertTrue();
                    setTimeout(()=>{
                        reject();
                    }, 500);
                } else {
                    console.info('SensorJsTest015  on1 success');
                    expect(typeof(data.x)).assertEqual("number");
                    setTimeout(()=>{
                        resolve();
                    }, 500);
                }
            });
        })

        await onPromise1.then(() =>{
            return onPromise2();
        }).then(()=>{
            return offPromise();
        });
        done();
    })


    /*
     * @tc.name:SensorJsTest016
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest016", 0, async function (done) {
        console.info('----------------------SensorJsTest016---------------------------');
        function offPromise() {
            return new Promise((resolve, reject) => {
                sensor.off(0, (error) =>{
                    if(error) {
                        console.info('SensorJsTest016  off error');
                        expect(false).assertTrue();
                        setTimeout(()=>{
                            done();
                            reject();
                        }, 500);
                    } else {
                        console.info('SensorJsTest016  off success');
                        expect(true).assertTrue();
                        setTimeout(()=>{
                            done();
                            resolve();
                        }, 500);
                    }
                });
            })
        }
        function oncePromise() {
            return new Promise((resolve, reject) => {
                sensor.once(0, function(error, data) {
                    if (error) {
                        console.info('SensorJsTest016  once error');
                        expect(false).assertTrue();
                        setTimeout(()=>{
                            reject();
                        }, 500);
                    } else {
                        console.info('SensorJsTest016  once success');
                        expect(typeof(data.x)).assertEqual("number");
                        setTimeout(()=>{
                            resolve();
                        }, 500);
                    }
                });
            })
        }

        let onPromise1 = new Promise((resolve, reject) => {
            sensor.on(0, function(error, data) {
                if (error) {
                    console.info('SensorJsTest016  on1 error');
                    expect(false).assertTrue();
                    setTimeout(()=>{
                        reject();
                    }, 500);
                } else {
                    console.info('SensorJsTest016  on1 success');
                    expect(typeof(data.x)).assertEqual("number");
                    setTimeout(()=>{
                        resolve();
                    }, 500);
                }
            });
        })

        await onPromise1.then(() =>{
            return oncePromise();
        }).then(()=>{
            return offPromise();
        });
        done();
    })

    let GEOMAGNETIC_COMPONENT_YEAR_RESULT = [
        [6570.3935546875, -146.3289337158203, 54606.0078125, -1.2758207321166992, 83.13726043701172, 6572.02294921875, 55000.0703125],
        [6554.17041015625, -87.19947052001953, 54649.078125, -0.7622424364089966, 83.16046905517578, 6554.75048828125, 55040.7734375],
        [6537.99169921875, -28.231582641601562, 54692.02734375, -0.24740631878376007, 83.18303680419922, 6538.052734375, 55081.4296875],
        [6521.81201171875, 30.73670768737793, 54734.97265625, 0.2700277864933014, 83.20502471923828, 6521.88427734375, 55122.15625],
        [6505.6328125, 89.70511627197266, 54777.90625, 0.7899921536445618, 83.22642517089844, 6506.2509765625, 55162.9453125]]

    let GEOMAGNETIC_COMPONENT_COORDINATES_RESULT = [
        [6570.3935546875, -146.3289337158203, 54606.0078125, -1.2758207321166992, 83.13726043701172, 6572.02294921875, 55000.0703125],
        [39624.28125, 109.8766098022461, -10932.4638671875, 0.15887857973575592, -15.424291610717773, 39624.43359375, 41104.921875],
        [37636.72265625, 104.90892791748047, -10474.810546875, 0.15970633924007416, -15.552550315856934, 37636.8671875, 39067.3203125],
        [5940.583984375, 15772.0927734375, -52480.7578125, 69.36103820800781, -72.19599914550781, 16853.765625, 55120.58984375],
        [5744.87255859375, 14799.48046875, -49969.40234375, 68.78474426269531, -72.37483215332031, 15875.3955078125, 52430.61328125]]

    let GEOMAGNETIC_COORDINATES = [[80, 0, 0],
                                   [0, 120, 0],
                                   [0, 120, 100000],
                                   [-80, 240, 0],
                                   [-80, 240, 100000]]

    let timeMillis = [1580486400000, 1612108800000, 1643644800000, 1675180800000, 1706716800000]

     /**
     * test
     *
     * @tc.name: SensorJsTest_017
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2U6
     * @tc.author:
     */
    it('SensorJsTest_017', 0, async function (done) {
        console.info("---------------------------SensorJsTest_017----------------------------------");
        let  promiseArray = []
        for (let i = 0; i < timeMillis.length; i++) {
            promiseArray.push(new Promise((resolve, reject) => {
                let j = i
                sensor.getGeomagneticField({'latitude':80, 'longitude':0, 'altitude':0}, timeMillis[j], (error, data) => {
                    if (error) {
                        console.info('SensorJsTest_017 failed');
                        expect(false).assertTrue();
                        setTimeout(() =>{
                            reject()
                        }, 500)
                    } else {
                        console.info('SensorJsTest_017 success x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                        + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                        expect(data.x).assertEqual(GEOMAGNETIC_COMPONENT_YEAR_RESULT[j][0])
                        expect(data.y).assertEqual(GEOMAGNETIC_COMPONENT_YEAR_RESULT[j][1])
                        expect(data.z).assertEqual(GEOMAGNETIC_COMPONENT_YEAR_RESULT[j][2])
                        expect(data.deflectionAngle).assertEqual(GEOMAGNETIC_COMPONENT_YEAR_RESULT[j][3])
                        expect(data.geomagneticDip).assertEqual(GEOMAGNETIC_COMPONENT_YEAR_RESULT[j][4])
                        expect(data.levelIntensity).assertEqual(GEOMAGNETIC_COMPONENT_YEAR_RESULT[j][5])
                        expect(data.totalIntensity).assertEqual(GEOMAGNETIC_COMPONENT_YEAR_RESULT[j][6])
                        setTimeout(() =>{
                            resolve()
                        }, 500)
                    }
                })
            }))
        }
        Promise.all(promiseArray).then(done)
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_018
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2U6
     * @tc.author:
     */
    it('SensorJsTest_018', 0, async function (done) {
        console.info("---------------------------SensorJsTest_018----------------------------------");
        let  promiseArray = []
        for (let i = 0; i < GEOMAGNETIC_COORDINATES.length; i++) {
            promiseArray.push(new Promise((resolve, reject) => {
                let j = i
                sensor.getGeomagneticField({'latitude':GEOMAGNETIC_COORDINATES[j][0], 'longitude':GEOMAGNETIC_COORDINATES[j][1], 'altitude':GEOMAGNETIC_COORDINATES[j][2]}, timeMillis[0], (error, data) => {
                    if (error) {
                        console.info('SensorJsTest_018 failed');
                        expect(false).assertTrue();
                        setTimeout(() =>{
                            reject()
                        }, 500)
                    } else {
                        console.info('SensorJsTest_018 success x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                        + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                        expect(data.x).assertEqual(GEOMAGNETIC_COMPONENT_COORDINATES_RESULT[j][0])
                        expect(data.y).assertEqual(GEOMAGNETIC_COMPONENT_COORDINATES_RESULT[j][1])
                        expect(data.z).assertEqual(GEOMAGNETIC_COMPONENT_COORDINATES_RESULT[j][2])
                        expect(data.deflectionAngle).assertEqual(GEOMAGNETIC_COMPONENT_COORDINATES_RESULT[j][3])
                        expect(data.geomagneticDip).assertEqual(GEOMAGNETIC_COMPONENT_COORDINATES_RESULT[j][4])
                        expect(data.levelIntensity).assertEqual(GEOMAGNETIC_COMPONENT_COORDINATES_RESULT[j][5])
                        expect(data.totalIntensity).assertEqual(GEOMAGNETIC_COMPONENT_COORDINATES_RESULT[j][6])
                        setTimeout(() =>{
                            resolve()
                        }, 500)
                    }
                })
            }))
        }
        Promise.all(promiseArray).then(done)
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_019
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2U6
     * @tc.author:
     */
    it('SensorJsTest_019', 0, async function (done) {
        console.info("---------------------------SensorJsTest_019----------------------------------");
        let geomagneticComponent = [27779.234375, -6214.9794921875, -14924.6611328125, -27.667943954467773, -12.610970497131348, 28465.9765625, 32141.2109375]
        sensor.getGeomagneticField({'latitude':0, 'longitude':0, 'altitude':0}, Number.MAX_VALUE, (error, data) => {
            if (error) {
                console.info('SensorJsTest_019 failed');
                expect(false).assertTrue();
            } else {
                console.info('SensorJsTest_019 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                expect(data.x).assertEqual(geomagneticComponent[0])
                expect(data.y).assertEqual(geomagneticComponent[1])
                expect(data.z).assertEqual(geomagneticComponent[2])
                expect(data.geomagneticDip).assertEqual(geomagneticComponent[3])
                expect(data.deflectionAngle).assertEqual(geomagneticComponent[4])
                expect(data.levelIntensity).assertEqual(geomagneticComponent[5])
                expect(data.totalIntensity).assertEqual(geomagneticComponent[6])
            }
            setTimeout(() =>{
                done()
            }, 500)
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_020
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2U6
     * @tc.author:
     */
    it('SensorJsTest_020', 0, async function (done) {
        console.info("---------------------------SensorJsTest_020----------------------------------");
        let geomagneticComponent = [27779.234375, -6214.9794921875, -14924.6611328125,
            -27.667943954467773, -12.610970497131348, 28465.9765625, 32141.2109375]
        sensor.getGeomagneticField({'latitude':0, 'longitude':0, 'altitude':0}, Number.MIN_VALUE, (error, data) => {
            if (error) {
                console.info('SensorJsTest_020 failed');
                expect(false).assertTrue();
            } else {
                console.info('SensorJsTest_020 success x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                expect(data.x).assertEqual(geomagneticComponent[0])
                expect(data.y).assertEqual(geomagneticComponent[1])
                expect(data.z).assertEqual(geomagneticComponent[2])
                expect(data.geomagneticDip).assertEqual(geomagneticComponent[3])
                expect(data.deflectionAngle).assertEqual(geomagneticComponent[4])
                expect(data.levelIntensity).assertEqual(geomagneticComponent[5])
                expect(data.totalIntensity).assertEqual(geomagneticComponent[6])
            }
            setTimeout(() =>{
                done()
            }, 500)
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_021
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2U6
     * @tc.author:
     */
    it('SensorJsTest_021', 0, async function (done) {
        console.info("---------------------------SensorJsTest_021----------------------------------");
        let geomagneticComponent = [1824.141845703125, 116.58167266845703, 56727.7734375, 88.15447235107422, 3.6568238735198975, 1827.8634033203125, 56757.21484375]
        sensor.getGeomagneticField({'latitude':Number.MAX_VALUE, 'longitude':0, 'altitude':0}, timeMillis[0], (error, data) => {
            if (error) {
                console.info('SensorJsTest_021 failed');
                expect(false).assertTrue();
            } else {
                console.info('SensorJsTest_021 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                expect(data.x).assertEqual(geomagneticComponent[0])
                expect(data.y).assertEqual(geomagneticComponent[1])
                expect(data.z).assertEqual(geomagneticComponent[2])
                expect(data.geomagneticDip).assertEqual(geomagneticComponent[3])
                expect(data.deflectionAngle).assertEqual(geomagneticComponent[4])
                expect(data.levelIntensity).assertEqual(geomagneticComponent[5])
                expect(data.totalIntensity).assertEqual(geomagneticComponent[6])
            }
            setTimeout(() =>{
                done()
            }, 500)
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_022
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2U6
     * @tc.author:
     */
    it('SensorJsTest_022', 0, async function (done) {
        console.info("---------------------------SensorJsTest_022----------------------------------");
        let geomagneticComponent = [1824.141845703125, 116.58167266845703, 56727.7734375, 88.15447235107422, 3.6568238735198975, 1827.8634033203125, 56757.21484375]
        sensor.getGeomagneticField({'latitude':Number.NaN, 'longitude':0, 'altitude':0}, timeMillis[0], (error, data) => {
            if (error) {
                console.info('SensorJsTest_022 failed');
                expect(false).assertTrue();
            } else {
                console.info('SensorJsTest_022 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                expect(data.x).assertEqual(geomagneticComponent[0])
                expect(data.y).assertEqual(geomagneticComponent[1])
                expect(data.z).assertEqual(geomagneticComponent[2])
                expect(data.geomagneticDip).assertEqual(geomagneticComponent[3])
                expect(data.deflectionAngle).assertEqual(geomagneticComponent[4])
                expect(data.levelIntensity).assertEqual(geomagneticComponent[5])
                expect(data.totalIntensity).assertEqual(geomagneticComponent[6])
            }
            setTimeout(() =>{
                done()
            }, 500)
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_023
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2U6
     * @tc.author:
     */
    it('SensorJsTest_023', 0, async function (done) {
        console.info("---------------------------SensorJsTest_023----------------------------------");
        let geomagneticComponent = [14425.57421875, -17156.767578125, -52023.21484375, -66.69005584716797, -49.94255447387695, 22415.4375, 56646.859375]
        sensor.getGeomagneticField({'latitude':Number.NEGATIVE_INFINITY, 'longitude':0, 'altitude':0}, timeMillis[0], (error, data) => {
            if (error) {
                console.info('SensorJsTest_023 failed');
                expect(false).assertTrue();
            } else {
                console.info('SensorJsTest_023 success x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                expect(data.x).assertEqual(geomagneticComponent[0])
                expect(data.y).assertEqual(geomagneticComponent[1])
                expect(data.z).assertEqual(geomagneticComponent[2])
                expect(data.geomagneticDip).assertEqual(geomagneticComponent[3])
                expect(data.deflectionAngle).assertEqual(geomagneticComponent[4])
                expect(data.levelIntensity).assertEqual(geomagneticComponent[5])
                expect(data.totalIntensity).assertEqual(geomagneticComponent[6])
            }
            setTimeout(() =>{
                done()
            }, 500)
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_024
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2U6
     * @tc.author:
     */
    it('SensorJsTest_024', 0, async function (done) {
        console.info("---------------------------SensorJsTest_024----------------------------------");
        sensor.getGeomagneticField({'latitude':0, 'longitude':Number.MAX_VALUE, 'altitude':0}, timeMillis[0], (error, data) => {
            if (error) {
                console.info('SensorJsTest_024 failed');
                expect(false).assertTrue();
            } else {
                console.info('SensorJsTest_024 success x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                expect(Number.isNaN(data.x) && Number.isNaN(data.y) && Number.isNaN(data.z)).assertTrue();
            }
            setTimeout(() =>{
                done()
            }, 500)
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_025
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: SR000GH2A3
     * @tc.author:
     */
    it('SensorJsTest_025', 0, async function (done) {
        console.info("---------------------------SensorJsTest_025----------------------------------");
        sensor.getGeomagneticField({'latitude':0, 'longitude':Number.NaN, 'altitude':0}, timeMillis[0], (error, data) => {
            if (error) {
                console.info('SensorJsTest_025 failed');
                expect(false).assertTrue();
            } else {
                console.info('SensorJsTest_025 success x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                expect(Number.isNaN(data.x) && Number.isNaN(data.y) && Number.isNaN(data.z)).assertTrue()
            }
            setTimeout(() =>{
                done()
            }, 500)
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_026
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: SR000GH2A3
     * @tc.author:
     */
    it('SensorJsTest_026', 0, async function (done) {
        console.info("---------------------------SensorJsTest_026----------------------------------");
        sensor.getGeomagneticField({'latitude':0, 'longitude':Number.NEGATIVE_INFINITY, 'altitude':0}, timeMillis[0], (error, data) => {
            if (error) {
                console.info('SensorJsTest_026 failed');
                expect(false).assertTrue();
            } else {
                console.info('SensorJsTest_026 success x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                expect(Number.isNaN(data.x) && Number.isNaN(data.y) && Number.isNaN(data.z)).assertTrue()
            }
            setTimeout(() =>{
                done()
            }, 500)
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_027
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: SR000GH2A3
     * @tc.author:
     */
    it('SensorJsTest_027', 0, async function (done) {
        console.info("---------------------------SensorJsTest_027----------------------------------");
        sensor.getGeomagneticField({'latitude':0, 'longitude':0, 'altitude':Number.MAX_VALUE}, timeMillis[0], (error, data) => {
            if (error) {
                console.info('SensorJsTest_027 failed');
                expect(false).assertTrue();
            } else {
                console.info('SensorJsTest_027 success x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                expect(Number.isNaN(data.x) && Number.isNaN(data.y) && Number.isNaN(data.z)).assertTrue()
            }
            setTimeout(() =>{
                done()
            }, 500)
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_028
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: SR000GH2A4
     * @tc.author:
     */
    it('SensorJsTest_028', 0, async function (done) {
        console.info("---------------------------SensorJsTest_028----------------------------------");
        let geomagneticComponent = [27536.40234375, -2248.586669921875, -16022.4306640625, -30.110872268676758, -4.66834020614624, 27628.05859375, 31937.875]
        sensor.getGeomagneticField({'latitude':0, 'longitude':0, 'altitude':Number.MIN_VALUE}, timeMillis[0], (error, data) => {
            if (error) {
                console.info('SensorJsTest_028 failed');
                expect(false).assertTrue();
            } else {
                console.info('SensorJsTest_028 success x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                expect(data.x).assertEqual(geomagneticComponent[0])
                expect(data.y).assertEqual(geomagneticComponent[1])
                expect(data.z).assertEqual(geomagneticComponent[2])
                expect(data.geomagneticDip).assertEqual(geomagneticComponent[3])
                expect(data.deflectionAngle).assertEqual(geomagneticComponent[4])
                expect(data.levelIntensity).assertEqual(geomagneticComponent[5])
                expect(data.totalIntensity).assertEqual(geomagneticComponent[6])
            }
            setTimeout(() =>{
                done()
            }, 500)
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_029
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: SR000GH2A4
     * @tc.author:
     */
    it('SensorJsTest_029', 0, async function (done) {
        console.info("---------------------------SensorJsTest_029----------------------------------");
        sensor.getGeomagneticField({'latitude':0, 'longitude':0, 'altitude':Number.NaN}, timeMillis[0], (error, data) => {
            if (error) {
                console.info('SensorJsTest_029 failed');
                expect(false).assertTrue();
            } else {
                console.info('SensorJsTest_029 success x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                expect(Number.isNaN(data.x) && Number.isNaN(data.y) && Number.isNaN(data.z)).assertTrue()
            }
            setTimeout(() =>{
                done()
            }, 500)
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_030
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: SR000GH2A4
     * @tc.author:
     */
    it('SensorJsTest_030', 0, async function (done) {
        console.info("---------------------------SensorJsTest_030----------------------------------");
        sensor.getGeomagneticField({'latitude':0, 'longitude':0, 'altitude':Number.NEGATIVE_INFINITY}, timeMillis[0], (error, data) => {
            if (error) {
                console.info('SensorJsTest_030 failed');
                expect(false).assertTrue();
            } else {
                console.info('SensorJsTest_030 success x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                expect(Number.isNaN(data.x) && Number.isNaN(data.y) && Number.isNaN(data.z)).assertTrue()
            }
            setTimeout(() =>{
                done()
            }, 500)
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_031
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2UB
     * @tc.author:
     */
    it('SensorJsTest_031', 0, async function (done) {
        console.info("---------------------------SensorJsTest_031----------------------------------");
        sensor.getGeomagneticField({'latitude':0, 'longitude':Number.NaN, 'altitude':0}, timeMillis[0], (error, data) => {
            if (error) {
                console.info('SensorJsTest_031 failed');
                expect(false).assertTrue();
            } else {
                console.info('SensorJsTest_031 success x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                expect(Number.isNaN(data.deflectionAngle) && Number.isNaN(data.geomagneticDip)).assertTrue()
            }
            setTimeout(() =>{
                done()
            }, 500)
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_032
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2UB
     * @tc.author:
     */
    it('SensorJsTest_032', 0, async function (done) {
        console.info("---------------------------SensorJsTest_032----------------------------------");
        sensor.getGeomagneticField({'latitude':0, 'longitude':Number.NEGATIVE_INFINITY, 'altitude':0}, timeMillis[0], (error, data) => {
            if (error) {
                console.info('SensorJsTest_032 once success');
                expect(false).assertTrue();
            } else {
                console.info('SensorJsTest_032 success x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                expect(Number.isNaN(data.deflectionAngle) && Number.isNaN(data.geomagneticDip)).assertTrue()
            }
            setTimeout(() =>{
                done()
            }, 500)
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_033
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2UB
     * @tc.author:
     */
    it('SensorJsTest_033', 0, async function (done) {
        console.info("---------------------------SensorJsTest_033----------------------------------");
        sensor.getGeomagneticField({'latitude':0, 'longitude':0, 'altitude':Number.MAX_VALUE}, timeMillis[0], (error, data) => {
            if (error) {
                console.info('SensorJsTest_033 failed');
                expect(false).assertTrue();
            } else {
                console.info('SensorJsTest_033 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                expect(Number.isNaN(data.deflectionAngle) && Number.isNaN(data.geomagneticDip)).assertTrue()
            }
            setTimeout(() =>{
                done()
            }, 500)
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_034
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2UB
     * @tc.author:
     */
    it('SensorJsTest_034', 0, async function (done) {
        console.info("---------------------------SensorJsTest_034----------------------------------");
        sensor.getGeomagneticField({'latitude':0, 'longitude':0, 'altitude':Number.NaN}, timeMillis[0], (error, data) => {
            if (error) {
                console.info('SensorJsTest_034 failed');
                expect(false).assertTrue();
            } else {
                console.info('SensorJsTest_034 success x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                expect(Number.isNaN(data.deflectionAngle) && Number.isNaN(data.geomagneticDip)).assertTrue()
            }
            setTimeout(() =>{
                done()
            }, 500)
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_035
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2UB
     * @tc.author:
     */
    it('SensorJsTest_035', 0, async function (done) {
        console.info("---------------------------SensorJsTest_035----------------------------------");
        sensor.getGeomagneticField({'latitude':0, 'longitude':0, 'altitude':Number.NEGATIVE_INFINITY}, timeMillis[0], (error, data) => {
            if (error) {
                console.info('SensorJsTest_035 failed');
                expect(false).assertfalse();
            } else {
                console.info('SensorJsTest_035 success x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                expect(Number.isNaN(data.deflectionAngle) && Number.isNaN(data.geomagneticDip)).assertTrue()
            }
            setTimeout(() =>{
                done()
            }, 500)
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_036
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2UB
     * @tc.author:
     */
    it('SensorJsTest_036', 0, async function (done) {
        console.info("---------------------------SensorJsTest_036----------------------------------");
        sensor.getGeomagneticField({'latitude':0, 'longitude':Number.MAX_VALUE, 'altitude':0}, timeMillis[0], (error, data) => {
            if (error) {
                console.info('SensorJsTest_036 failed');
                expect(false).assertTrue();
            } else {
                console.info('SensorJsTest_036 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                expect(Number.isNaN(data.levelIntensity) && Number.isNaN(data.totalIntensity)).assertTrue();
            }
            setTimeout(() =>{
                done()
            }, 500)
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_037
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2UB
     * @tc.author:
     */
    it('SensorJsTest_037', 0, async function (done) {
        console.info("---------------------------SensorJsTest_037----------------------------------");
        sensor.getGeomagneticField({'latitude':0, 'longitude':Number.NaN, 'altitude':0}, timeMillis[0], (error, data) => {
            if (error) {
                console.info('SensorJsTest_037 failed');
                expect(false).assertTrue();
            } else {
                console.info('SensorJsTest_037 success x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                expect(Number.isNaN(data.levelIntensity) && Number.isNaN(data.totalIntensity)).assertTrue()
            }
            setTimeout(() =>{
                done()
            }, 500)
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_038
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2UB
     * @tc.author:
     */
    it('SensorJsTest_038', 0, async function (done) {
        console.info("---------------------------SensorJsTest_038----------------------------------");
        sensor.getGeomagneticField({'latitude':0, 'longitude':Number.NEGATIVE_INFINITY, 'altitude':0}, timeMillis[0], (error, data) => {
            if (error) {
                console.info('SensorJsTest_038 failed');
                expect(false).assertTrue();
            } else {
                console.info('SensorJsTest_038 success x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                expect(Number.isNaN(data.deflectionAngle) && Number.isNaN(data.geomagneticDip)).assertTrue()
            }
            setTimeout(() =>{
                done()
            }, 500)
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_039
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2UB
     * @tc.author:
     */
    it('SensorJsTest_039', 0, async function (done) {
        console.info("---------------------------SensorJsTest_039----------------------------------");
        sensor.getGeomagneticField({'latitude':0, 'longitude':0, 'altitude':Number.MAX_VALUE}, timeMillis[0], (error, data) => {
            if (error) {
                console.info('SensorJsTest_039 failed');
                expect(false).assertTrue();
            } else {
                console.info('SensorJsTest_039 success x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                expect(Number.isNaN(data.levelIntensity) && Number.isNaN(data.totalIntensity)).assertTrue()
            }
            setTimeout(() =>{
                done()
            }, 500)
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_040
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2UB
     * @tc.author:
     */
    it('SensorJsTest_040', 0, async function (done) {
        console.info("---------------------------SensorJsTest_040----------------------------------");
        sensor.getGeomagneticField({'latitude':0, 'longitude':0, 'altitude':Number.NaN}, timeMillis[0], (error, data) => {
            if (error) {
                console.info('SensorJsTest_040 once success');
                expect(false).assertTrue();
            } else {
                console.info('SensorJsTest_040 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                expect(Number.isNaN(data.levelIntensity) && Number.isNaN(data.totalIntensity)).assertTrue()
            }
            setTimeout(() =>{
                done()
            }, 500)
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_041
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2UB
     * @tc.author:
     */
    it('SensorJsTest_041', 0, async function (done) {
        console.info("---------------------------SensorJsTest_041----------------------------------");
        sensor.getGeomagneticField({'latitude':0, 'longitude':0, 'altitude':Number.NEGATIVE_INFINITY}, timeMillis[0], (error, data) => {
            if (error) {
                console.info('SensorJsTest_041 once success');
                expect(false).assertTrue();
            } else {
                console.info('SensorJsTest_041 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
                expect(Number.isNaN(data.levelIntensity) && Number.isNaN(data.totalIntensity)).assertTrue()
            }
            setTimeout(() =>{
                done()
            }, 500)
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_042
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2UB
     * @tc.author:
     */
    it("SensorJsTest_042", 0, async function (done) {
        console.info("---------------------------SensorJsTest_042----------------------------------");
        for (var i = 0; i < timeMillis.length; i++) {
            await sensor.getGeomagneticField({'latitude':80, 'longitude':0, 'altitude':0}, timeMillis[i]).then((data) => {
                console.info('SensorJsTest_042 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
                + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity);
                expect(data.x).assertEqual(GEOMAGNETIC_COMPONENT_YEAR_RESULT[i][0])
                expect(data.y).assertEqual(GEOMAGNETIC_COMPONENT_YEAR_RESULT[i][1])
                expect(data.z).assertEqual(GEOMAGNETIC_COMPONENT_YEAR_RESULT[i][2])
                expect(data.deflectionAngle).assertEqual(GEOMAGNETIC_COMPONENT_YEAR_RESULT[i][3])
                expect(data.geomagneticDip).assertEqual(GEOMAGNETIC_COMPONENT_YEAR_RESULT[i][4])
                expect(data.levelIntensity).assertEqual(GEOMAGNETIC_COMPONENT_YEAR_RESULT[i][5])
                expect(data.totalIntensity).assertEqual(GEOMAGNETIC_COMPONENT_YEAR_RESULT[i][6])
            }).catch((error) => {
                console.info("promise::catch", error);
            })
        }
        done()
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_043
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2UB
     * @tc.author:
     */
    it("SensorJsTest_043", 0, async function (done) {
        console.info('----------------------SensorJsTest_043---------------------------');
        let geomagneticComponent = [27779.234375, -6214.9794921875, -14924.6611328125, -27.667943954467773, -12.610970497131348, 28465.9765625, 32141.2109375]
        await sensor.getGeomagneticField({'latitude':0, 'longitude':0, 'altitude':0}, Number.MAX_VALUE).then((data) => {
            console.info('SensorJsTest_043 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
            + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
            expect(data.x).assertEqual(geomagneticComponent[0])
            expect(data.y).assertEqual(geomagneticComponent[1])
            expect(data.z).assertEqual(geomagneticComponent[2])
            expect(data.geomagneticDip).assertEqual(geomagneticComponent[3])
            expect(data.deflectionAngle).assertEqual(geomagneticComponent[4])
            expect(data.levelIntensity).assertEqual(geomagneticComponent[5])
            expect(data.totalIntensity).assertEqual(geomagneticComponent[6])
        }).catch((error) => {
            console.info("promise::catch", error)
        });
        done()
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_044
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2UD
     * @tc.author:
     */
    it("SensorJsTest_044", 0, async function (done) {
        console.info('----------------------SensorJsTest_044---------------------------');
        let geomagneticComponent = [27779.234375, -6214.9794921875, -14924.6611328125, -27.667943954467773, -12.610970497131348, 28465.9765625, 32141.2109375]
        await sensor.getGeomagneticField({'latitude':0, 'longitude':0, 'altitude':0}, Number.MIN_VALUE).then((data) => {
            console.info('SensorJsTest_044 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
            + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
            expect(data.x).assertEqual(geomagneticComponent[0])
            expect(data.y).assertEqual(geomagneticComponent[1])
            expect(data.z).assertEqual(geomagneticComponent[2])
            expect(data.geomagneticDip).assertEqual(geomagneticComponent[3])
            expect(data.deflectionAngle).assertEqual(geomagneticComponent[4])
            expect(data.levelIntensity).assertEqual(geomagneticComponent[5])
            expect(data.totalIntensity).assertEqual(geomagneticComponent[6])
        }).catch((error) => {
            console.info("promise::catch", error);
        });
        done()
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_045
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2UD
     * @tc.author:
     */
    it("SensorJsTest_045", 0, async function (done) {
        console.info('----------------------SensorJsTest_045---------------------------');
        let geomagneticComponent = [1824.141845703125, 116.58167266845703, 56727.7734375, 88.15447235107422, 3.6568238735198975, 1827.8634033203125, 56757.21484375]
        await sensor.getGeomagneticField({'latitude':Number.MAX_VALUE, 'longitude':0, 'altitude':0}, timeMillis[0]).then((data) => {
            console.info('SensorJsTest_045 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
            + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
            expect(data.x).assertEqual(geomagneticComponent[0])
            expect(data.y).assertEqual(geomagneticComponent[1])
            expect(data.z).assertEqual(geomagneticComponent[2])
            expect(data.geomagneticDip).assertEqual(geomagneticComponent[3])
            expect(data.deflectionAngle).assertEqual(geomagneticComponent[4])
            expect(data.levelIntensity).assertEqual(geomagneticComponent[5])
            expect(data.totalIntensity).assertEqual(geomagneticComponent[6])
        }).catch((error) => {
            console.info("promise::catch", error)
        });
        done()
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_046
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2UD
     * @tc.author:
     */
    it("SensorJsTest_046", 0, async function (done) {
        console.info('----------------------SensorJsTest_030---------------------------');
        let geomagneticComponent = [1824.141845703125, 116.58167266845703, 56727.7734375, 88.15447235107422, 3.6568238735198975, 1827.8634033203125, 56757.21484375]
        await sensor.getGeomagneticField({'latitude':Number.NaN, 'longitude':0, 'altitude':0}, timeMillis[0]).then((data) => {
            console.info('SensorJsTest_030 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
            + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
            expect(data.x).assertEqual(geomagneticComponent[0])
            expect(data.y).assertEqual(geomagneticComponent[1])
            expect(data.z).assertEqual(geomagneticComponent[2])
            expect(data.geomagneticDip).assertEqual(geomagneticComponent[3])
            expect(data.deflectionAngle).assertEqual(geomagneticComponent[4])
            expect(data.levelIntensity).assertEqual(geomagneticComponent[5])
            expect(data.totalIntensity).assertEqual(geomagneticComponent[6])
        }).catch((error) => {
            console.info("promise::catch", error)
        });
        done()
    })

    /*
     * @tc.name:SensorJsTest_047
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest_047", 0, async function (done) {
        console.info('----------------------SensorJsTest_047---------------------------');
        let geomagneticComponent = [14425.57421875, -17156.767578125, -52023.21484375, -66.69005584716797, -49.94255447387695, 22415.4375, 56646.859375]
        await sensor.getGeomagneticField({'latitude':Number.NEGATIVE_INFINITY, 'longitude':0, 'altitude':0}, timeMillis[0]).then((data) => {
            console.info('SensorJsTest_047 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
            + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
            expect(data.x).assertEqual(geomagneticComponent[0])
            expect(data.y).assertEqual(geomagneticComponent[1])
            expect(data.z).assertEqual(geomagneticComponent[2])
            expect(data.geomagneticDip).assertEqual(geomagneticComponent[3])
            expect(data.deflectionAngle).assertEqual(geomagneticComponent[4])
            expect(data.levelIntensity).assertEqual(geomagneticComponent[5])
            expect(data.totalIntensity).assertEqual(geomagneticComponent[6])
        }).catch((error) => {
            console.info("promise::catch", error)
        });
        done()
    })

    /*
     * @tc.name:SensorJsTest_048
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest_048", 0, async function (done) {
        console.info('----------------------SensorJsTest_048---------------------------');
        let geomagneticComponent = [NaN, NaN, NaN]
        await sensor.getGeomagneticField({'latitude':0, 'longitude':Number.MAX_VALUE, 'altitude':0}, timeMillis[0]).then((data) => {
            console.info('SensorJsTest_048 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
            + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
            expect(Number.isNaN(data.x) && Number.isNaN(data.y) && Number.isNaN(data.z)).assertTrue()
        }).catch((error) => {
            console.info("promise::catch", error)
        });
        done()
    })

    /*
     * @tc.name:SensorJsTest_049
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest_049", 0, async function (done) {
        console.info('----------------------SensorJsTest_049---------------------------');
        await sensor.getGeomagneticField({'latitude':0, 'longitude':Number.NaN, 'altitude':0}, timeMillis[0]).then((data) => {
            console.info('SensorJsTest_049 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
            + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
            expect(Number.isNaN(data.x) && Number.isNaN(data.y) && Number.isNaN(data.z)).assertTrue()
        }).catch((error) => {
            console.info("promise::catch", error)
        });
        done()
    })

    /*
     * @tc.name:SensorJsTest_050
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest_050", 0, async function (done) {
        console.info('----------------------SensorJsTest_050---------------------------');
        await sensor.getGeomagneticField({'latitude':0, 'longitude':Number.NEGATIVE_INFINITY, 'altitude':0}, timeMillis[0]).then((data) => {
            console.info('SensorJsTest_050 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
            + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
            expect(Number.isNaN(data.x) && Number.isNaN(data.y) && Number.isNaN(data.z)).assertTrue()
        }).catch((error) => {
            console.info("promise::catch", error)
        });
        done()
    })

    /*
     * @tc.name:SensorJsTest_051
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest_051", 0, async function (done) {
        console.info('----------------------SensorJsTest_051---------------------------');
        await sensor.getGeomagneticField({'latitude':0, 'longitude':0, 'altitude':Number.MAX_VALUE}, timeMillis[0]).then((data) => {
            console.info('SensorJsTest_051 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
            + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
            expect(Number.isNaN(data.x) && Number.isNaN(data.y) && Number.isNaN(data.z)).assertTrue()
        }).catch((error) => {
            console.info("promise::catch", error)
        });
        done()
    })

    /*
     * @tc.name:SensorJsTest_052
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest_052", 0, async function (done) {
        console.info('----------------------SensorJsTest_052---------------------------');
        let geomagneticComponent = [27536.40234375, -2248.586669921875, -16022.4306640625, -30.110872268676758, -4.66834020614624, 27628.05859375, 31937.875]
        await sensor.getGeomagneticField({'latitude':0, 'longitude':0, 'altitude':Number.MIN_VALUE}, timeMillis[0]).then((data) => {
            console.info('SensorJsTest_052 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
            + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
            expect(data.x).assertEqual(geomagneticComponent[0])
            expect(data.y).assertEqual(geomagneticComponent[1])
            expect(data.z).assertEqual(geomagneticComponent[2])
            expect(data.geomagneticDip).assertEqual(geomagneticComponent[3])
            expect(data.deflectionAngle).assertEqual(geomagneticComponent[4])
            expect(data.levelIntensity).assertEqual(geomagneticComponent[5])
            expect(data.totalIntensity).assertEqual(geomagneticComponent[6])
        }).catch((error) => {
            console.info("promise::catch", error)
        });
        done()
    })

    /*
     * @tc.name:SensorJsTest_053
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest_053", 0, async function (done) {
        console.info('----------------------SensorJsTest_053---------------------------start');
        await sensor.getGeomagneticField({'latitude':0, 'longitude':0, 'altitude':Number.NaN}, timeMillis[0]).then((data) => {
            console.info('SensorJsTest_053 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
            + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
            expect(Number.isNaN(data.x) && Number.isNaN(data.y) && Number.isNaN(data.z)).assertTrue()
        }).catch((error) => {
            console.info("promise::catch", error)
        });
        done()
    })

    /*
     * @tc.name:SensorJsTest_054
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest_054", 0, async function (done) {
        console.info('----------------------SensorJsTest_054---------------------------');
        await sensor.getGeomagneticField({'latitude':0, 'longitude':0, 'altitude':Number.NEGATIVE_INFINITY}, timeMillis[0]).then((data) => {
            console.info('SensorJsTest_054 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
            + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
            expect(Number.isNaN(data.x) && Number.isNaN(data.y) && Number.isNaN(data.z)).assertTrue()
        }).catch((error) => {
            console.info("promise::catch", error)
        });
        done()
    })

    /*
     * @tc.name:SensorJsTest_055
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it('SensorJsTest_055', 0, async function (done) {
        for (var i = 0; i < timeMillis.length; i++) {
            console.info('----------------------SensorJsTest_055---------------------------');
            await sensor.getGeomagneticField({'latitude':80, 'longitude':0, 'altitude':0}, timeMillis[i]).then((data) => {
               console.info('SensorJsTest_055 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
               + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity);
               expect(data.x).assertEqual(GEOMAGNETIC_COMPONENT_YEAR_RESULT[i][0])
               expect(data.y).assertEqual(GEOMAGNETIC_COMPONENT_YEAR_RESULT[i][1])
               expect(data.z).assertEqual(GEOMAGNETIC_COMPONENT_YEAR_RESULT[i][2])
               expect(data.deflectionAngle).assertEqual(GEOMAGNETIC_COMPONENT_YEAR_RESULT[i][3])
               expect(data.geomagneticDip).assertEqual(GEOMAGNETIC_COMPONENT_YEAR_RESULT[i][4])
               expect(data.levelIntensity).assertEqual(GEOMAGNETIC_COMPONENT_YEAR_RESULT[i][5])
               expect(data.totalIntensity).assertEqual(GEOMAGNETIC_COMPONENT_YEAR_RESULT[i][6])
            }).catch((error) => {
               console.info("promise::catch", error)
            });
        }
        done()
   })

    /*
     * @tc.name:SensorJsTest_056
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest_056", 0, async function (done) {
        console.info('----------------------SensorJsTest_056---------------------------');
        await sensor.getGeomagneticField({'latitude':0, 'longitude':Number.NaN, 'altitude':0}, timeMillis[0]).then((data) => {
            console.info('SensorJsTest_056 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
            + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
            expect(Number.isNaN(data.deflectionAngle) && Number.isNaN(data.geomagneticDip)).assertTrue()
        }).catch((error) => {
            console.info("promise::catch", error)
        });
        done()
    })

    /*
     * @tc.name:SensorJsTest_057
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest_057", 0, async function (done) {
        console.info('----------------------SensorJsTest_057---------------------------');
        await sensor.getGeomagneticField({'latitude':0, 'longitude':Number.NEGATIVE_INFINITY, 'altitude':0}, timeMillis[0]).then((data) => {
            console.info('SensorJsTest_057 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
            + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
            expect(Number.isNaN(data.deflectionAngle) && Number.isNaN(data.geomagneticDip)).assertTrue()
        }).catch((error) => {
            console.info("promise::catch", error)
        });
        done()
    })

    /*
     * @tc.name:SensorJsTest_058
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest_058", 0, async function (done) {
        console.info('----------------------SensorJsTest_058 max ---------------------------');
        await sensor.getGeomagneticField({'latitude':0, 'longitude':0, 'altitude':Number.MAX_VALUE}, timeMillis[0]).then((data) => {
            console.info('SensorJsTest_058 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
            + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
            expect(Number.isNaN(data.deflectionAngle) && Number.isNaN(data.geomagneticDip)).assertTrue()
        }).catch((error) => {
            console.info("promise::catch", error)
        });
        done()
    })

    /*
     * @tc.name:SensorJsTest_059
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest_059", 0, async function (done) {
        console.info('----------------------SensorJsTest_059---------------------------');
        await sensor.getGeomagneticField({'latitude':0, 'longitude':0, 'altitude':Number.NaN}, timeMillis[0]).then((data) => {
            console.info('SensorJsTest_059 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
            + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
            expect(Number.isNaN(data.deflectionAngle) && Number.isNaN(data.geomagneticDip)).assertTrue()
        }).catch((error) => {
            console.info("promise::catch", error)
        });
        done()
    })

    /*
     * @tc.name:SensorJsTest_060
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest_060", 0, async function (done) {
        console.info('----------------------SensorJsTest_060---------------------------');
        await sensor.getGeomagneticField({'latitude':0, 'longitude':0, 'altitude':Number.NEGATIVE_INFINITY}, timeMillis[0]).then((data) => {
            console.info('SensorJsTest_060 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
            + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
            expect(Number.isNaN(data.deflectionAngle) && Number.isNaN(data.geomagneticDip)).assertTrue()
        }).catch((error) => {
            console.info("promise::catch", error)
        });
        done()
    })

    /*
     * @tc.name:SensorJsTest_061
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
   it('SensorJsTest_061', 0, async function (done) {
        console.info('----------------------SensorJsTest_061---------------------------');
       for (var i = 0; i < GEOMAGNETIC_COORDINATES.length; i++) {
            await sensor.getGeomagneticField({'latitude':GEOMAGNETIC_COORDINATES[i][0], 'longitude':GEOMAGNETIC_COORDINATES[i][1], 'altitude':GEOMAGNETIC_COORDINATES[i][2]}, timeMillis[0]).then((data) => {
               console.info('SensorJsTest_061 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
               + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
               expect(data.x).assertEqual(GEOMAGNETIC_COMPONENT_COORDINATES_RESULT[i][0])
               expect(data.y).assertEqual(GEOMAGNETIC_COMPONENT_COORDINATES_RESULT[i][1])
               expect(data.z).assertEqual(GEOMAGNETIC_COMPONENT_COORDINATES_RESULT[i][2])
               expect(data.deflectionAngle).assertEqual(GEOMAGNETIC_COMPONENT_COORDINATES_RESULT[i][3])
               expect(data.geomagneticDip).assertEqual(GEOMAGNETIC_COMPONENT_COORDINATES_RESULT[i][4])
               expect(data.levelIntensity).assertEqual(GEOMAGNETIC_COMPONENT_COORDINATES_RESULT[i][5])
               expect(data.totalIntensity).assertEqual(GEOMAGNETIC_COMPONENT_COORDINATES_RESULT[i][6])
           }).catch((error) => {
               console.info("promise::catch", error);
           });
       }
       done()
   })

    /*
     * @tc.name:SensorJsTest_062
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest_062", 0, async function (done) {
        console.info('----------------------SensorJsTest_062---------------------------');
        await sensor.getGeomagneticField({'latitude':0, 'longitude':Number.MAX_VALUE, 'altitude':0}, timeMillis[0]).then((data) => {
            console.info('SensorJsTest_062 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
            + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
            expect(Number.isNaN(data.levelIntensity) && Number.isNaN(data.totalIntensity)).assertTrue();
        }).catch((error) => {
            console.info("promise::catch", error);
        });
        done()
    })

    /*
     * @tc.name:SensorJsTest_063
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest_063", 0, async function (done) {
        console.info('----------------------SensorJsTest_063---------------------------');
        await sensor.getGeomagneticField({'latitude':0, 'longitude':Number.NaN, 'altitude':0}, timeMillis[0]).then((data) => {
            console.info('SensorJsTest_063 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
            + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
            expect(Number.isNaN(data.levelIntensity) && Number.isNaN(data.totalIntensity)).assertTrue()
        }).catch((error) => {
            console.info("promise::catch", error);
        });
        done()
    })

    /*
     * @tc.name:SensorJsTest_064
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest_064", 0, async function (done) {
        console.info('----------------------SensorJsTest_064---------------------------');
        await sensor.getGeomagneticField({'latitude':0, 'longitude':Number.NEGATIVE_INFINITY, 'altitude':0}, timeMillis[0]).then((data) => {
            console.info('SensorJsTest_064 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
            + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
            expect(Number.isNaN(data.deflectionAngle) && Number.isNaN(data.geomagneticDip)).assertTrue()
        }).catch((error) => {
            console.info("promise::catch", error);
        });
        done()
    })

    /*
     * @tc.name:SensorJsTest_065
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest_065", 0, async function (done) {
        console.info('----------------------SensorJsTest_065---------------------------');
        await sensor.getGeomagneticField({'latitude':0, 'longitude':0, 'altitude':Number.MAX_VALUE}, timeMillis[0]).then((data) => {
            console.info('SensorJsTest_065 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
            + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
            expect(Number.isNaN(data.levelIntensity) && Number.isNaN(data.totalIntensity)).assertTrue()
        }).catch((error) => {
            console.info("promise::catch", error);
        });
        done()
    })

    /*
     * @tc.name:SensorJsTest_066
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest_066", 0, async function (done) {
        console.info('----------------------SensorJsTest_066---------------------------');
        await sensor.getGeomagneticField({'latitude':0, 'longitude':0, 'altitude':Number.NaN}, timeMillis[0]).then((data) => {
            console.info('SensorJsTest_066 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
            + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
            expect(Number.isNaN(data.levelIntensity) && Number.isNaN(data.totalIntensity)).assertTrue()
        }).catch((error) => {
            console.info("promise::catch", error);
        });
        done()
    })

    /*
     * @tc.name:SensorJsTest_067
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest_067", 0, async function (done) {
        console.info('----------------------SensorJsTest_067---------------------------');
        await sensor.getGeomagneticField({'latitude':0, 'longitude':0, 'altitude':Number.NEGATIVE_INFINITY}, timeMillis[0]).then((data) => {
            console.info('SensorJsTest_067 x: ' + data.x + ',y: ' + data.y + ',z: ' + data.z + ',geomagneticDip: ' + data.geomagneticDip
            + ',deflectionAngle: ' + data.deflectionAngle + ',levelIntensity: ' + data.levelIntensity + ',totalIntensity: ' + data.totalIntensity)
            expect(Number.isNaN(data.levelIntensity) && Number.isNaN(data.totalIntensity)).assertTrue()
        }).catch((error) => {
            console.info("promise::catch", error);
        });
        done()
    })

    var SENSOR_DATA_MATRIX = [
        {
            "rotation": [-0.7980074882507324, 0.5486301183700562, 0.24937734007835388, -0.17277367413043976,
                -0.6047078967094421, 0.7774815559387207, 0.5773502588272095, 0.5773502588272095,0.5773502588272095],
            "inclination":[1, 0, 0, 0, 0.20444221794605255,0.9788785576820374, 0, -0.9788785576820374, 0.20444221794605255]
        },
        {
            "rotation": [-0.8206444382667542, 0.3832680284976959, 0.42384934425354004, 0.021023601293563843,
                -0.7209705710411072,0.6926466822624207, 0.5710522532463074,0.57732754945755,0.5836028456687927],
            "inclination":[1, 0, 0, 0, 0.2584352493286133,0.9660285115242004, 0, -0.9660285115242004, 0.2584352493286133]
        },
        {
            "rotation": [0.9583651423454285, 0.08038506656885147, -0.27399733662605286, 0.160231813788414,
                -0.9456362724304199, 0.2830156981945038, -0.23635157942771912, -0.3151354491710663, -0.9191450476646423],
            "inclination":[1, 0, 0, 0, 0.34239840507507324, 0.9395548701286316, 0, -0.9395548701286316, 0.34239840507507324]
        },
        {
            "rotation":[null, null, null, null, null, null, null, null, null],
            "inclination":[1, 0, 0, 0, null, null, 0, null ,null]
        },
        {
            "rotation":[null, null, null, null, null, null,0, 0, 0],
            "inclination":[1, 0, 0, 0, null, 0, 0, 0, null]
        }
    ]

    let GET_DIRECTION = [
        [ 0.38050639629364014, -0.9783217310905457, -0.6610431671142578],
        [-2.7610862255096436, 1.5018651485443115, 2.987273931503296],
        [0.32175055146217346, -1.006853699684143, -0.6857295036315918],
        [1.3332617282867432, -1.5440233945846558, -0.6627295017242432],
        [NaN, NaN, NaN],
        [0.7853981852531433, -0.6154797077178955, -0.7853981852531433],
        [0.7853981852531433, -0.7853981852531433, -0.7853981852531433],
        [0.785398, -0.615480, -0.785398],
        [0.785398, -0.785398, -0.785398]
    ]

    let rotationMatrix = [
        [1, 2, 3, 4, 5, 6, 7, 8, 9],
        [-1, -2, -3, -4, -5, -6, -7, -78, -45],
        [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        [11111111, 21111111, 31111111, 4111111, 5111111, 61111111, 71111111, 811111111, 91111111],
        [NaN, NaN, NaN, NaN, NaN, NaN, NaN, NaN, NaN],
        [3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38],
        [3.40282e+39, 3.40282e+39, 3.40282e+39, 3.40282e+39, 3.40282e+39, 3.40282e+39, 3.40282e+39, 3.40282e+39, 3.40282e+39],
        [3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38],
        [3.40282e+39, 3.40282e+39, 3.40282e+39, 3.40282e+39, 3.40282e+39, 3.40282e+39, 3.40282e+39, 3.40282e+39, 3.40282e+39]
    ]

    let gravity = [
        [9, 9, 9], [91, 92, 93], [-9, -12, -35], [NaN, NaN, NaN], [3.40282e+38, 3.40282e+38, 3.40282e+38], [3.40282e+39, 3.40282e+39, 3.40282e+39]
    ]
    let geomagnetic = [
        [30, 25, 41], [3, 2, 4], [-123, -456, -564], [3.40282e+38, 3.40282e+38, 3.40282e+38], [NaN, NaN, NaN]
    ]

    /**
     * test
     *
     * @tc.name: SensorJsTest_068
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2RV
     * @tc.author:
     */
     it('SensorJsTest_068', 0, async function (done) {
        sensor.createRotationMatrix(gravity[0], geomagnetic[0], (error,data)=>{
            if (error) {
                console.info('SensorJsTest_068 failed');
                expect(false).assertTrue();
            } else {
                console.info("SensorJsTest_068" + JSON.stringify(data))
                expect(JSON.stringify(data)).assertEqual(JSON.stringify(SENSOR_DATA_MATRIX[0]))
            }
            done()
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_069
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2RV
     * @tc.author:
     */

    it('SensorJsTest_069', 0, async function (done) {
        sensor.createRotationMatrix(gravity[2],geomagnetic[2],(error,data)=>{
		    if (error) {
                console.info('SensorJsTest_069 failed');
                expect(false).assertTrue();
            } else {
                console.info("SensorJsTest_069" + JSON.stringify(data))
                expect(JSON.stringify(data)).assertEqual(JSON.stringify(SENSOR_DATA_MATRIX[2]))
            }
			done()
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_070
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2RV
     * @tc.author:
     */
    it('SensorJsTest_070', 0, async function (done) {
        sensor.getDirection(rotationMatrix[0],(error,data)=>{
            if (error) {
                console.info('SensorJsTest_070 failed');
                expect(false).assertTrue();
            } else {
                for (var i = 1; i < data.length; i++) {
                    console.info("SensorJsTest_070" + data[i])
                    expect(data[i]).assertEqual(GET_DIRECTION[0][i])
                }
            }
            done()
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_071
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2RV
     * @tc.author:
     */
    it('SensorJsTest_071', 0, async function (done) {
        sensor.getDirection(rotationMatrix[1],function(error,data){
            if (error) {
                console.info('SensorJsTest_071 failed');
                expect(false).assertTrue();
            } else {
                for (var i = 1; i < data.length; i++) {
                    console.info("SensorJsTest_071" + data[i])
                    expect(data[i]).assertEqual(GET_DIRECTION[1][i])
                }
            }
            done()
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_072
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2RV
     * @tc.author:
     */
    it('SensorJsTest_072', 0, async function (done) {
        sensor.createRotationMatrix(gravity[0],geomagnetic[0]).then((data) => {
            console.info("SensorJsTest_072" + JSON.stringify(data))
            expect(JSON.stringify(data)).assertEqual(JSON.stringify(SENSOR_DATA_MATRIX[0]))
            done()
        }, (error) =>{
            expect(false).assertTrue();
            done()
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_073
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2RV
     * @tc.author:
     */
    it('SensorJsTest_073', 0, async function (done) {
        sensor.createRotationMatrix(gravity[1],geomagnetic[1]).then((data) => {
            console.info("SensorJsTest_073" + JSON.stringify(data))
            expect(JSON.stringify(data)).assertEqual(JSON.stringify(SENSOR_DATA_MATRIX[1]))
            done()
        }, (error) =>{
            expect(false).assertTrue();
            done()
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_074
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2RV
     * @tc.author:
     */
    it('SensorJsTest_074', 0, async function (done) {
        sensor.createRotationMatrix(gravity[2],geomagnetic[2]).then((data) => {
            console.info("SensorJsTest_074" + JSON.stringify(data))
            expect(JSON.stringify(data)).assertEqual(JSON.stringify(SENSOR_DATA_MATRIX[2]))
            done()
        }, (error) =>{
            expect(false).assertTrue();
            done()
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_075
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2RN
     * @tc.author:
     */
    it('SensorJsTest_075', 0, async function (done) {
        sensor.getDirection(rotationMatrix[0]).then((data) => {
            for (var i = 0; i<data.length; i++) {
                console.info("SensorJsTest_075" + data[i])
                expect(data[i]).assertEqual(GET_DIRECTION[0][i])
            }
            done()
        }, (error) =>{
            expect(false).assertTrue();
            done()
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_076
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2RN
     * @tc.author:
     */
    it('SensorJsTest_076', 0, async function (done) {
        sensor.getDirection(rotationMatrix[1]).then((data) => {
            for (var i = 0; i<data.length; i++) {
                console.info("SensorJsTest_076" + data[i])
                expect(data[i]).assertEqual(GET_DIRECTION[1][i])
            }
            done()
        }, (error) =>{
            expect(false).assertTrue();
            done()
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_077
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2RN
     * @tc.author:
     */
    it('SensorJsTest_077', 0, async function (done) {
        sensor.getDirection([1,2,3,1,2,3,1,2,3,0]).then((data) => {
            for (var i = 0; i<data.length; i++) {
                console.info("SensorJsTest_077 failed")
                expect(false).assertTrue();
            }
            done()
        }, (error) =>{
            expect(true).assertTrue();
            console.info("SensorJsTest_077 success")
            done()
        })
    })

    let ANGLECHANGE_9_RESULT = [
        [0.7853981852531433, NaN, -0.32175055146217346],  //123123123
        [0.7853981852531433, NaN, -0.7853981852531433],   //FLOAT.MAXVALUE
        [0.0, -0.0, -0.0],                                //FLOAT.MINVALUE
        [0.7853981852531433, NaN, -0.7853981852531433],   //FLOAT.MAXVALUE+1
        ];

    /**
     * test
     *
     * @tc.name: SensorJsTest_078
     * @tc.desc:
     * @tc.require: AR000GH2SL
     * @tc.author:
     */
    it('SensorJsTest_078', 0, async function (done) {
        console.info("SensorJsAPI--->SensorJsTest_078");
        sensor.getAngleModify([1,2,3,1,2,3,1,2,3], [2,2,2,2,2,2,2,2,2], function(error, data) {
            if (error) {
                console.info('SensorJsTest_078 failed');
                expect(false).assertTrue();
            } else {
                for(var i = 0; i < data.length; i++) {
                    console.info("SensorJsAPI--->SensorJsTest_078 [" + i + "] = " + data[i]);
                    expect(data[0]).assertEqual(ANGLECHANGE_9_RESULT[0][0]) && expect(Number.isNaN(data[1])).assertTrue() &&
                    expect(data[2]).assertEqual(ANGLECHANGE_9_RESULT[0][2]);
                }
            }
            done()
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_079
     * @tc.desc:
     * @tc.require: AR000GH2SL
     * @tc.author:
     */
    it('SensorJsTest_079', 0, async function (done) {
        console.info("SensorJsAPI--->SensorJsTest_079");
        sensor.getAngleModify([3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38],
            [3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38],
            function(error, data) {
                if (error) {
                    console.info('SensorJsTest_079 failed');
                    expect(false).assertTrue();
                } else {
                    for(var i = 0; i < data.length; i++) {
                        console.info("SensorJsAPI--->SensorJsTest_079 [" + i + "] = " + data[i]);
                        expect(data[0]).assertEqual(ANGLECHANGE_9_RESULT[1][0]) && expect(Number.isNaN(data[1])).assertTrue() &&
                        expect(data[2]).assertEqual(ANGLECHANGE_9_RESULT[1][2]);
                    }
                }
                done()
            })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_080
     * @tc.desc:
     * @tc.require: AR000GH2SL
     * @tc.author:
     */
    it('SensorJsTest_080', 0, async function (done) {
        console.info("SensorJsAPI--->SensorJsTest_080");
        sensor.getAngleModify([1.17549e-38, 1.17549e-38, 1.17549e-38, 1.17549e-38, 1.17549e-38, 1.17549e-38, 1.17549e-38, 1.17549e-38, 1.17549e-38],
            [1.17549e-38, 1.17549e-38, 1.17549e-38, 1.17549e-38, 1.17549e-38, 1.17549e-38, 1.17549e-38, 1.17549e-38, 1.17549e-38],
            function(error, data) {
                if (error) {
                    console.info('SensorJsTest_080 failed');
                    expect(false).assertTrue();
                } else {
                    for(var i = 0; i < data.length; i++) {
                        console.info("SensorJsAPI--->SensorJsTest_080 [" + i + "] = " + data[i]);
                        expect(data[0]).assertEqual(ANGLECHANGE_9_RESULT[2][0])
                        && expect(data[1]).assertEqual(ANGLECHANGE_9_RESULT[2][1])
                        && expect(data[2]).assertEqual(ANGLECHANGE_9_RESULT[2][2]);
                    }
                }
                done()
            });
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_081
     * @tc.desc:
     * @tc.require: AR000GH2SL
     * @tc.author:
     */
    it('SensorJsTest_081', 0, async function (done) {
        console.info("SensorJsAPI--->SensorJsTest_081");
        sensor.getAngleModify([3.40282e+38+1, 3.40282e+38+1, 3.40282e+38+1, 3.40282e+38+1, 3.40282e+38+1, 3.40282e+38+1, 3.40282e+38+1, 3.40282e+38+1, 3.40282e+38+1],
            [3.40282e+38+1, 3.40282e+38+1, 3.40282e+38+1, 3.40282e+38+1, 3.40282e+38+1, 3.40282e+38+1, 3.40282e+38+1, 3.40282e+38+1, 3.40282e+38+1],
            function(error, data) {
                if (error) {
                    console.info('SensorJsTest_081 failed');
                    expect(false).assertTrue();
                } else {
                    for(var i = 0; i < data.length; i++) {
                        console.info("SensorJsAPI--->SensorJsTest_081 [" + i + "] = " + data[i]);
                        expect(data[0]).assertEqual(ANGLECHANGE_9_RESULT[3][0]) && expect(Number.isNaN(data[1])).assertTrue() && expect(data[2]).assertEqual(ANGLECHANGE_9_RESULT[3][2]);
                    }
                }
                done()
            });
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_082
     * @tc.desc:
     * @tc.require: AR000GH2SL
     * @tc.author:
     */
    it('SensorJsTest_082', 0, async function (done) {
        console.info("SensorJsAPI--->SensorJsTest_082");
        sensor.getAngleModify([0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0],
            [0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0],
            function(error, data) {
                if (error) {
                    console.info('SensorJsTest_082 failed');
                    expect(false).assertTrue();
                } else {
                    for(var i = 0; i < data.length; i++) {
                        console.info("SensorJsAPI--->SensorJsTest_082 [" + i + "] = " + data[i]);
                        expect(Number.isNaN(data[0]) && Number.isNaN(data[1]) && Number.isNaN(data[2])).assertTrue();
                    }
                }
                done()
            });
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_083
     * @tc.desc:
     * @tc.require: AR000GH2SL
     * @tc.author:
     */
    it('SensorJsTest_083', 0, async function (done) {
        console.info("SensorJsAPI--->SensorJsTest_083");
        sensor.getAngleModify([1,2,3,1,2,3,1,2,3], [2,2,2,2,2,2,2,2,2]).then((data) => {
			console.info("SensorJsAPI--->SensorJsTest_083");
			for(var i = 0; i < data.length; i++) {
				console.info("SensorJsAPI--->SensorJsTest_083 [" + i + "] = " + data[i]);
				expect(data[0]).assertEqual(ANGLECHANGE_9_RESULT[0][0]) && expect(Number.isNaN(data[1])).assertTrue() &&
                expect(data[2]).assertEqual(ANGLECHANGE_9_RESULT[0][2]);
			}
            done();
        }, (error) =>{
            expect(false).assertTrue();
            done()
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_084
     * @tc.desc:
     * @tc.require: AR000GH2SL
     * @tc.author:
     */
    it('SensorJsTest_084', 0, async function (done) {
        console.info("SensorJsAPI--->SensorJsTest_084");
        sensor.getAngleModify([3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38],
            [3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38]).then((data) => {
			console.info("SensorJsAPI--->SensorJsTest_084");
			for(var i = 0; i < data.length; i++) {
				console.info("SensorJsAPI--->SensorJsTest_084 [" + i + "] = " + data[i]);
				expect(data[0]).assertEqual(ANGLECHANGE_9_RESULT[1][0]) && expect(Number.isNaN(data[1])).assertTrue() && expect(data[2]).assertEqual(ANGLECHANGE_9_RESULT[1][2]);
			}
            done()
        }, (error) =>{
            expect(false).assertTrue();
            done()
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_085
     * @tc.desc:
     * @tc.require: AR000GH2SL
     * @tc.author:
     */
    it('SensorJsTest_085', 0, async function (done) {
        console.info("SensorJsAPI--->SensorJsTest_085");
        sensor.getAngleModify([1.17549e-38, 1.17549e-38, 1.17549e-38, 1.17549e-38, 1.17549e-38, 1.17549e-38, 1.17549e-38, 1.17549e-38, 1.17549e-38],
            [1.17549e-38, 1.17549e-38, 1.17549e-38, 1.17549e-38, 1.17549e-38, 1.17549e-38, 1.17549e-38, 1.17549e-38, 1.17549e-38]).then((data) => {
            console.info("SensorJsAPI--->SensorJsTest_085");
            for(var i = 0; i < data.length; i++) {
                console.info("SensorJsAPI--->SensorJsTest_085 [" + i + "] = " + data[i]);
                expect(data[0]).assertEqual(ANGLECHANGE_9_RESULT[2][0])
                && expect(data[1]).assertEqual(ANGLECHANGE_9_RESULT[2][1])
                && expect(data[2]).assertEqual(ANGLECHANGE_9_RESULT[2][2]);
            }
            done()
        }, (error) =>{
            expect(false).assertTrue();
            done()
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_086
     * @tc.desc:
     * @tc.require: AR000GH2SL
     * @tc.author:
     */
    it('SensorJsTest_086', 0, async function (done) {
        console.info("SensorJsAPI--->SensorJsTest_076");
        sensor.getAngleModify([3.40282e+38 + 1,3.40282e+38 + 1,3.40282e+38 + 1,3.40282e+38+1,3.40282e+38+1,3.40282e+38+1,3.40282e+38+1,3.40282e+38+1,3.40282e+38+1],
            [3.40282e+38+1,3.40282e+38+1,3.40282e+38+1,3.40282e+38+1,3.40282e+38+1,3.40282e+38+1,3.40282e+38+1,3.40282e+38+1,3.40282e+38+1])
        .then((data) => {
            console.info("SensorJsAPI--->SensorJsTest_086");
            for(var i = 0; i < data.length; i++) {
                console.info("SensorJsAPI--->SensorJsTest_086 [" + i + "] = " + data[i]);
                expect(data[0]).assertEqual(ANGLECHANGE_9_RESULT[3][0]) && expect(Number.isNaN(data[1])).assertTrue() && expect(data[2]).assertEqual(ANGLECHANGE_9_RESULT[3][2]);
            }
            done()
        }, (error) =>{
            expect(false).assertTrue();
            done()
        })
    })

    /**
     * test
     *
     * @tc.name: SensorJsTest_087
     * @tc.desc:
     * @tc.require: AR000GH2SL
     * @tc.author:
     */
    it('SensorJsTest_087', 0, async function (done) {
        console.info("SensorJsAPI--->SensorJsTest_087");
        sensor.getAngleModify([0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0],
            [0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0]).then((data) => {
            console.info("SensorJsAPI--->SensorJsTest_087");
            for(var i = 0; i < data.length; i++) {
                console.info("SensorJsAPI--->SensorJsTest_087 [" + i + "] = " + data[i]);
                expect(Number.isNaN(data[0]) && Number.isNaN(data[1]) && Number.isNaN(data[2])).assertTrue();
            }
            done()
        }, (error) =>{
            expect(false).assertTrue();
            done()
        })
    })

    var result = [
        [0.7441122531890869, 0.5199999809265137, -0.335999995470047, -0.25099998712539673],
        [0, 3.402820018375656e+38, 3.402820018375656e+38, 3.402820018375656e+38],
        [1, 0, 0, 0],
        [0.7183529734611511, -0.32499998807907104, -0.5619999766349792, -0.25],
        [0, 0, 0, 0]
    ]

    /*
    * @tc.name: SensorJsTest_088
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2RP
    * @tc.author:
    */
    it('SensorJsTest_088', 0,async function (done) {
        console.info('SensorJsTest_088 start')
        sensor.createQuaternion([0.52, -0.336, -0.251], (error, data) =>{
            console.info('SensorJsTest_088' + 'lengh:' + data.length);
			if (error) {
                console.info('SensorJsTest_088 failed');
                expect(false).assertTrue();
            } else {
				for (var i = 0; i < data.length; i++) {
					console.info("data[" + i + "]: " + data[i])
					expect(data[i]).assertEqual(result[0][i])
				}
            }
			done()
        })
    })

    /*
    * @tc.name: SensorJsTest_089
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2RP
    * @tc.author:
    */
    it('SensorJsTest_089', 0,async function (done) {
        console.info('SensorJsTest_089 start')
        sensor.createQuaternion([3.40282e+38, 3.40282e+38, 3.40282e+38], (error, data) =>{
			if (error) {
                console.info('SensorJsTest_089 failed');
                expect(false).assertTrue();
            } else {
				for (var i = 0; i < data.length; i++) {
					console.info("data[" + i + "]: " + data[i])
					expect(data[i]).assertEqual(result[1][i])
				}
            }
            done()
        })
    })

    /*
    * @tc.name: SensorJsTest_090
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2RP
    * @tc.author:
    */
    it('SensorJsTest_090', 0,async function (done) {
        console.info('SensorJsTest_090 start')
        sensor.createQuaternion([0, 0, 0], (error, data) =>{
			if (error) {
                console.info('SensorJsTest_090 failed');
                expect(false).assertTrue();
            } else {
				for (var i = 0; i < data.length; i++) {
					console.info("data[" + i + "]: " + data[i])
					expect(data[i]).assertEqual(result[2][i])
				}
            }
            done()
        })
        console.info("SensorJsTest_090 end")
    })

    /*
    * @tc.name: SensorJsTest_091
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2RP
    * @tc.author:
    */
    it('SensorJsTest_091', 0,async function (done) {
        console.info('SensorJsTest_091 start')
        sensor.createQuaternion([-0.325, -0.562, -0.25], (error, data) =>{
			if (error) {
                console.info('SensorJsTest_091 failed');
                expect(false).assertTrue();
            } else {
				for (var i = 0; i < data.length; i++) {
					console.info("data[" + i + "]: " + data[i])
					expect(data[i]).assertEqual(result[3][i])
				}
            }
            done()
        })
        console.info("SensorJsTest_091 end")
    })

    /*
    * @tc.name: SensorJsTest_092
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2RP
    * @tc.author:
    */
    it('SensorJsTest_092', 0,async function (done) {
        console.info('SensorJsTest_092 start')
        sensor.createQuaternion([0.25, 0.14], (error, data) =>{
			if (error) {
                console.info('SensorJsTest_092 failed');
                expect(true).assertTrue();
            } else {
                expect(false).assertTrue();
            }
            done()
        })
        console.info("SensorJsTest_092 end")
    })

    /*
    * @tc.name: SensorJsTest_093
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2RP
    * @tc.author:
    */
    it('SensorJsTest_093', 0,async function (done) {
        console.info('SensorJsTest_093 start')
        sensor.createQuaternion([0.52, -0.336, -0.251]).then((data) => {
            console.info('SensorJsTest_093');
            for (var i = 0; i < data.length; i++) {
                console.info("data[" + i + "]: " + data[i]);
                expect(data[i]).assertEqual(result[0][i])
            }
            done()
        }, (error) => {
            expect(false).assertTrue();
            console.info('promise failed')
            done()
        })
        console.info("SensorJsTest_093 end")
    })

    /*
    * @tc.name: SensorJsTest_094
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2RP
    * @tc.author:
    */
    it('SensorJsTest_094', 0,async function (done) {
        console.info('SensorJsTest_094 start')
        sensor.createQuaternion([0, 0]).then((data) => {
            console.info('SensorJsTest_094');
            expect(false).assertTrue();
            done()
        }, (error) => {
            expect(true).assertTrue();
            console.info('promise failed')
            done()
        })
        console.info("SensorJsTest_094 end")
    })

    /*
    * @tc.name: SensorJsTest_095
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2RP
    * @tc.author:
    */
    it('SensorJsTest_095', 0,async function (done) {
        console.info('SensorJsTest_095 start')
        sensor.createQuaternion([0, 0, 0]).then((data) => {
            console.info('SensorJsTest_095');
            for (var i = 0; i < data.length; i++) {
                console.info("data[" + i + "]: " + data[i]);
                expect(data[i]).assertEqual(result[2][i])
            }
            done()
        }, (error) => {
            expect(false).assertTrue();
            console.info('promise failed')
            done()
        })
        console.info("SensorJsTest_095 end")
    })

    /*
    * @tc.name: SensorJsTest_096
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2RP
    * @tc.author:
    */
    it('SensorJsTest_096', 0,async function (done) {
        console.info('SensorJsTest_096 start')
        sensor.createQuaternion([-0.325, -0.562, -0.25]).then((data) => {
            console.info('SensorJsTest_096');
            for (var i = 0; i < data.length; i++) {
                console.info("data[" + i + "]: " + data[i]);
                expect(data[i]).assertEqual(result[3][i])
            }
            done()
        },(error) => {
            expect(false).assertTrue();
            console.info('promise failed')
            done()
        })
    })

    /*
    * @tc.name: SensorJsTest_097
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2RP
    * @tc.author:
    */
    it('SensorJsTest_097', 0,async function (done) {
        console.info('SensorJsTest_097 start')
        sensor.createQuaternion([0.25, 0.14]).then((data) => {
            console.info('SensorJsTest_097');
            expect(false).assertTrue();
            done()
        },(error) => {
            expect(true).assertTrue();
            console.info('promise failed')
            done()
        })
    })

    let createRotationMatrixResult = [
        [0.6724675297737122,-0.10471208393573761,0.7326819896697998,0.06531608104705811,0.9944750070571899,
            0.08217836916446686,-0.7372390031814575,-0.007406365126371384,0.6755914688110352],
        [1,0,0,0,1,0,0,0,1]
        ]
    /*
    * @tc.name: SensorJsTest_098
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2RV
    * @tc.author:
    */
    it('SensorJsTest_098', 0, async function (done) {
        console.info("SensorJsTest_098 start");
        sensor.createRotationMatrix([-0.0245, 0.402, 0.0465], (error, data) =>{
            console.info("SensorJsTest_098");
			if (error) {
                console.info('SensorJsTest_098 failed');
                expect(false).assertTrue();
            } else {
                for(var i = 0;i < data.length; i++) {
                    console.info("SensorJsTest_098 [" + i + ") = " + data[i]);
                    expect(data[i]).assertEqual(createRotationMatrixResult[0][i])
                }
            }
			done()
        })
        console.info(LABEL + "SensorJsTest_098 end");
    })

    /*
    * tc.name: SensorJsTest_099
    * tc.desc: Verfication results of the incorrect parameters of test interface.
    * tc.require: SR000GH2A2
    * @tc.author:
    */
    it('SensorJsTest_099', 0,async function (done) {
        console.info('SensorJsTest_099 start')
        sensor.createRotationMatrix([-0.0245, 0.402, 0.0465]).then((data) => {
            for(var i = 0;i < data.length; i++) {
                console.info("SensorJsTest_099 [" + i + "] : " + data[i]);
                expect(data[i]).assertEqual(createRotationMatrixResult[0][i])
            }
            done()
        },(error) => {
            expect(false).assertTrue();
            console.info('promise failed', error)
            done()
        })
        console.info( "SensorJsTest_099 end")
    })

    /*
    * tc.name: SensorJsTest_100
    * tc.desc: Verfication results of the incorrect parameters of test interface.
    * tc.require: AR000GH2RV
    * @tc.author:
    */
    it('SensorJsTest_100', 0,async function (done) {
        console.info('SensorJsTest_100 start')
        sensor.createRotationMatrix([0, 0, 0]).then((data) => {
            for(var i = 0;i < data.length; i++) {
                console.info("SensorJsTest_100 [" + i + "] : " + data[i]);
                expect(data[i]).assertEqual(createRotationMatrixResult[1][i])
            }
            done()
        }, (error) => {
            expect(false).assertTrue();
            console.info('promise failed', error)
            done()
        })
        console.info( "SensorJsTest_100 end")
    })

	var getGeomagneticDipResult = [ 0.8760581016540527, 0.862170, -Infinity, 44330]

    /*
    * @tc.name: SensorJsTest_101
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2OG
    * @tc.author:
    */
    it('SensorJsTest_101', 0, async function (done) {
        console.info('SensorJsTest_101 start')
        sensor.getGeomagneticDip([1, 2, 3, 4, 5, 6, 7, 8, 9], (error, data) => {
		    if (error) {
                console.info('SensorJsTest_101 failed');
                expect(false).assertTrue();
            } else {
			   console.info("SensorJsTest_101" + data)
			   expect(data).assertEqual(getGeomagneticDipResult[0])
            }
			done()
            console.info('SensorJsTest_101' + 'lengh:' + data.length);
        })
        console.info("SensorJsTest_101 end")
    })

    /*
    * @tc.name: SensorJsTest_102
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2OG
    * @tc.author:
    */
    it('SensorJsTest_102', 0, async function (done) {
        console.info('SensorJsTest_102 start')
        sensor.getGeomagneticDip([1, 2, 3, 4], (error,data) => {
			if (error) {
                console.info('SensorJsTest_102 success');
                expect(true).assertTrue();
            } else {
			   console.info("SensorJsTest_102 failed")
			   expect(false).assertTrue();
            }
			done()
        })
        console.info("SensorJsTest_102 end")
    })

    /*
    * @tc.name: SensorJsTest_103
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2OG
    * @tc.author:
    */
    it('SensorJsTest_103', 0, async function (done) {
        console.info('SensorJsTest_103 start')
        sensor.getAltitude(0, 100, (error, data) => {
			if (error) {
                console.info('SensorJsTest_103 failed');
                expect(false).assertTrue();
            } else {
			   console.info("SensorJsTest_103" + data)
			   expect(data).assertEqual(getGeomagneticDipResult[2])
            }
            done()
            console.info("SensorJsTest_103 end")
        })
    })

    /*
    * @tc.name: SensorJsTest_104
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2OG
    * @tc.author:
    */
    it('SensorJsTest_104', 0, async function (done) {
        console.info('SensorJsTest_104 start')
        sensor.getAltitude(5, 0, (error, data) => {
			if (error) {
                console.info('SensorJsTest_104 failed');
                expect(false).assertTrue();
            } else {
			   console.info("SensorJsTest_104" + data)
			   expect(data).assertEqual(getGeomagneticDipResult[3])
            }
            done()
        })
        console.info("SensorJsTest_104 end")
    })

    /*
    * @tc.name: SensorJsTest_105
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2OG
    * @tc.author:
    */
    it('SensorJsTest_105', 0, async function (done) {
        sensor.getAltitude(0, 100).then((data)=>{
            console.info("SensorJsTest_104" + data)
            expect(data).assertEqual(getGeomagneticDipResult[2])
            done()
        }, (error)=>{
            console.info('SensorJsTest_104 failed');
            expect(false).assertTrue();
            done()
        });
    })

    /*
    * @tc.name: SensorJsTest_106
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2OG
    * @tc.author:
    */
    it('SensorJsTest_106', 0, async function (done) {
        sensor.getAltitude(5, 0).then((data)=>{
            console.info("SensorJsTest_104" + data)
            expect(data).assertEqual(getGeomagneticDipResult[3])
            done()
        }, (error)=>{
            console.info('SensorJsTest_104 failed');
            expect(false).assertTrue();
            done()
        });
    })

    let transformCoordinateSystemResult = [
    [1.500000, 1.500000, 1.500000, 1.500000, 1.500000, 1.500000, 1.500000, 1.500000, 1.500000],
    [340282001837565600000000000000000000000.000000, 340282001837565600000000000000000000000.000000, 340282001837565600000000000000000000000.000000,
     340282001837565600000000000000000000000.000000, 340282001837565600000000000000000000000.000000, 340282001837565600000000000000000000000.000000,
     340282001837565600000000000000000000000.000000, 340282001837565600000000000000000000000.000000, 340282001837565600000000000000000000000.000000],
     [Infinity, -Infinity, Infinity, Infinity, -Infinity, Infinity, Infinity, -Infinity, Infinity]]

    /*
    * @tc.name: SensorJsTest_107
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2TR
    * @tc.author:
    */
     it('SensorJsTest_107', 0, async function (done) {
        console.info("---------------------------SensorJsTest_107----------------------------------");
        sensor.transformCoordinateSystem([1.5, 1.5, 1.5, 1.5, 1.5, 1.5, 1.5, 1.5, 1.5], {'axisX':1, 'axisY':2}, (error, data) => {
            if (error) {
                console.info('SensorJsTest_107 failed');
                expect(false).assertTrue();
            } else {
                console.info("SensorJsTest_107 " + JSON.stringify(data));
                expect(JSON.stringify(data)).assertEqual(JSON.stringify(transformCoordinateSystemResult[0]))
            }
            done()
        })
    })

    /*
    * @tc.name: SensorJsTest_108
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2TR
    * @tc.author:
    */
    it('SensorJsTest_108', 0, async function (done) {
        console.info("---------------------------SensorJsTest_108----------------------------------");
        sensor.transformCoordinateSystem([3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38, 3.40282e+38], {'axisX':1, 'axisY':2}, (error, data) => {
            if (error) {
                console.info('SensorJsTest_108 failed');
                expect(false).assertTrue();
            } else {
                console.info("SensorJsTest_108 " + JSON.stringify(data));
                expect(JSON.stringify(data)).assertEqual(JSON.stringify(transformCoordinateSystemResult[1]))
            }
            done()
        })
    })

    /*
    * @tc.name: SensorJsTest_109
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2TR
    * @tc.author:
    */
    it("SensorJsTest_109", 0, async function (done) {
        console.info("---------------------------SensorJsTest_109----------------------------------");
        sensor.transformCoordinateSystem([1.5, 1.5, 1.5, 1.5, 1.5, 1.5, 1.5, 1.5, 1.5], {'axisX':1, 'axisY':2}).then((data) => {
            for (var i = 0; i < data.length; i++) {
                console.info("SensorJsTest_109 data[ " + i + "] = " + data[i]);
                expect(data[i]).assertEqual(transformCoordinateSystemResult[0][i]);
            }
            done()
        }, (error)=>{
            console.info('SensorJsTest_109 failed');
            expect(false).assertTrue();
            done()
        });
    })

    /*
    * @tc.name: SensorJsTest_110
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2TR
    * @tc.author:
    */
    it("SensorJsTest_110", 0, async function (done) {
        console.info("---------------------------SensorJsTest_110----------------------------------");
        sensor.transformCoordinateSystem([3.40282e+39, 3.40282e+39, 3.40282e+39, 3.40282e+39, 3.40282e+39, 3.40282e+39, 3.40282e+39, 3.40282e+39, 3.40282e+39], {'axisX':1, 'axisY':3}).then((data) => {
            for (var i = 0; i < data.length; i++) {
                console.info("SensorJsTest_110 data[ " + i + "] = " + data[i]);
                expect(data[i]).assertEqual(transformCoordinateSystemResult[2][i]);
            }
            done()
        }, (error)=>{
            console.info('SensorJsTest_109 failed');
            expect(false).assertTrue();
            done()
        });
    })

    /*
    * @tc.name: SensorJsTest_111
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2TR
    * @tc.author:
    */
    it("SensorJsTest_111", 0, async function (done) {
        console.info("---------------------------SensorJsTest_111----------------------------------");
        sensor.getSensorList().then((data) => {
            console.info("---------------------------SensorJsTest_111 callback in-----------" + data.length);
            for (var i = 0; i < data.length; i++) {
                console.info("SensorJsTest_111 " + JSON.stringify(data[i]));
            }
            expect(true).assertTrue();
            done();
        }, (error)=>{
            console.info('SensorJsTest_111 failed');
            expect(false).assertTrue();
            done();
        });
    })

    /*
    * @tc.name: SensorJsTest_112
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2TR
    * @tc.author:
    */
    it("SensorJsTest_112", 0, async function (done) {
        console.info("---------------------------SensorJsTest_112----------------------------------");
        sensor.getSensorList((error, data) => {
            if (error) {
                console.info('SensorJsTest_112 failed');
                expect(false).assertTrue();
            } else {
                console.info("---------------------------SensorJsTest_112 callback in-----------" + data.length);
                for (var i = 0; i < data.length; i++) {
                    console.info("SensorJsTest_112 " + JSON.stringify(data[i]));
                }
                expect(true).assertTrue();
            }
            done()
        });
    })

    /*
    * @tc.name: SensorJsTest_113
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2TR
    * @tc.author:
    */
    it("SensorJsTest_113", 0, async function (done) {
        console.info("---------------------------SensorJsTest_113----------------------------------");
        sensor.getSingleSensor(0, (error, data) => {
            if (error) {
                console.info('SensorJsTest_113 failed');
                expect(false).assertTrue();
            } else {
                console.info("---------------------------SensorJsTest_113 callback in-----------" + data.length);
                console.info("SensorJsTest_113 " + JSON.stringify(data));
                expect(true).assertTrue();
            }
            done()
        });
    })

    /*
    * @tc.name: SensorJsTest_114
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2TR
    * @tc.author:
    */
    it("SensorJsTest_114", 0, async function (done) {
        console.info("---------------------------SensorJsTest_114----------------------------------");
        sensor.getSingleSensor(-1, (error, data) => {
            if (error) {
                console.info('SensorJsTest_114 failed');
                expect(true).assertTrue();
            } else {
                console.info("---------------------------SensorJsTest_114 callback in-----------" + data.length);
                console.info("SensorJsTest_114 " + JSON.stringify(data));
                expect(false).assertTrue();
            }
            done()
        });
    })

    /*
    * @tc.name: SensorJsTest_115
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2TR
    * @tc.author:
    */
    it("SensorJsTest_115", 0, async function (done) {
        console.info("---------------------------SensorJsTest_115----------------------------------");
        sensor.getSingleSensor(0).then((data) => {
            console.info("SensorJsTest_115 " + JSON.stringify(data));
            expect(true).assertTrue();
            done()
        }, (error)=>{
            console.info('SensorJsTest_115 failed');
            expect(false).assertTrue();
            done()
        });
    })

    /*
    * @tc.name: SensorJsTest_116
    * @tc.desc: Verfication results of the incorrect parameters of test interface.
    * @tc.require: AR000GH2TR
    * @tc.author:
    */
    it("SensorJsTest_116", 0, async function (done) {
        console.info("---------------------------SensorJsTest_116----------------------------------");
        sensor.getSingleSensor(-1).then((data) => {
            console.info("SensorJsTest_116 " + JSON.stringify(data));
            expect(false).assertTrue();
            done()
        }, (error)=>{
            console.info('SensorJsTest_116 success');
            expect(true).assertTrue();
            done()
        });
    })
})
