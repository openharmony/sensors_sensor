/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

describe("BarometerJsTest", function () {
    function callback(data) {
        console.info("callback" + JSON.stringify(data));
        expect(typeof(data.pressure)).assertEqual("number");
    }

    function callback2(data) {
        console.info("callback2" + JSON.stringify(data));
        expect(typeof(data.pressure)).assertEqual("number");
    }
 
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

    const PARAMETER_ERROR_CODE = 401
    const SERVICE_EXCEPTION_CODE = 14500101
    const PARAMETER_ERROR_MSG = 'The parameter invalid.'
    const SERVICE_EXCEPTION_MSG = 'Service exception.'

    /*
     * @tc.name:BarometerJsTest_001
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("BarometerJsTest_001", 0, async function (done) {
        console.info('----------------------BarometerJsTest_001---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.BAROMETER, (err, data) => {
                if(err) {
                    console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                expect(typeof(data)).assertEqual("object");
                if(data) {
                    try {
                        sensor.on(sensor.SensorId.BAROMETER, callback);
                        setTimeout(()=>{
                            sensor.off(sensor.SensorId.BAROMETER);
                            done();
                        }, 500);
                    } catch (error) {
                        console.info('On fail, errCode:' + error.code + ' ,msg:' + error.message);
                        expect(false).assertTrue();
                        done();
                    }
                } else {
                    console.info('Program run failed');
                    expect(false).assertTrue();
                    done();
                }
            });
        } catch (error) {
            console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
    * @tc.name:BarometerJsTest_002
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("BarometerJsTest_002", 0, async function (done) {
        console.info('----------------------BarometerJsTest_002---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.BAROMETER, (err, data) => {
                if(err) {
                    console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                expect(typeof(data)).assertEqual("object");
                if(data) {
                    try {
                        sensor.on(sensor.SensorId.BAROMETER, callback, {'interval':100000000});
                        setTimeout(()=>{
                            console.info('------------------BarometerJsTest_002 off in-----------------------');
                            sensor.off(sensor.SensorId.BAROMETER);
                            console.info('------------------BarometerJsTest_002 off end-----------------------');
                            done();
                        }, 500);
                    } catch (error) {
                        console.info('On fail, errCode:' + error.code + ' ,msg:' + error.message);
                        expect(false).assertTrue();
                        done();
                    }
                } else {
                    console.info('Non-mobile devices cannot be excluded');
                    expect(true).assertTrue();
                    done();
                }
            });
        } catch (error) {
            console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
    * @tc.name:BarometerJsTest_003
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("BarometerJsTest_003", 0, async function (done) {
        console.info('----------------------BarometerJsTest_003---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.BAROMETER, (err, data) => {
                if(err) {
                    console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                expect(typeof(data)).assertEqual("object");
                function onSensorCallback(data) {
                    console.info('BarometerJsTest_003  callback in');
                    expect(true).assertTrue();
                    done();
                }
                if(data) {
                    try {
                        sensor.on(sensor.SensorId.BAROMETER, onSensorCallback, {'interval': 100000000}, 5);
                        setTimeout(()=>{
                            console.info('----------------------BarometerJsTest_003 off in---------------------------');
                            sensor.off(sensor.SensorId.BAROMETER);
                            console.info('----------------------BarometerJsTest_003 off end---------------------------');
                            done();
                        }, 500);
                    } catch (error) {
                        console.info('On fail, errCode:' + error.code + ' ,msg:' + error.message);
                        expect(false).assertTrue();
                        done();
                    }
                } else {
                    console.info('Non-mobile devices cannot be excluded');
                    expect(true).assertTrue();
                    done();
                }
            });
        } catch (error) {
            console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
    * @tc.name:BarometerJsTest_004
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("BarometerJsTest_004", 0, async function (done) {
        console.info('----------------------BarometerJsTest_004---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.BAROMETER, (err, data) => {
                if(err) {
                    console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                expect(typeof(data)).assertEqual("object");
                if(data) {
                    try {
                        sensor.on(sensor.SensorId.BAROMETER, callback, {'interval': -100000000});
                        console.info('----------------------BarometerJsTest_004 off in---------------------------');
                        sensor.off(sensor.SensorId.BAROMETER);
                        console.info('----------------------BarometerJsTest_004 off end---------------------------');
                        done();
                    } catch (error) {
                        console.info('On fail, errCode:' + error.code + ' ,msg:' + error.message);
                        expect(error.code).assertEqual(SERVICE_EXCEPTION_CODE);
                        expect(error.message).assertEqual(SERVICE_EXCEPTION_MSG);
                        done();
                    }
                } else {
                    console.info('Non-mobile devices cannot be excluded');
                    expect(true).assertTrue();
                    done();
                }
            });
        } catch (error) {
            console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
    * @tc.name:BarometerJsTest_005
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("BarometerJsTest_005", 0, async function (done) {
        console.info('----------------------BarometerJsTest_005---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.BAROMETER, (err, data) => {
                if(err) {
                    console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                expect(typeof(data)).assertEqual("object");
                if(data) {            
                    try {
                        sensor.once(sensor.SensorId.BAROMETER, callback);
                        setTimeout(()=>{
                            expect(true).assertTrue();
                            done();
                        }, 500);
                    } catch (error) {
                        console.error('Once fail, errCode:' + error.code + ' ,msg:' + error.message);
                        expect(false).assertTrue();
                        done();
                    }
                } else {
                    console.info('Non-mobile devices cannot be excluded');
                    expect(true).assertTrue();
                    done();
                }
            });
        } catch (error) {
            console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
    * @tc.name:BarometerJsTest_006
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("BarometerJsTest_006", 0, async function (done) {
        console.info('----------------------BarometerJsTest_006---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.BAROMETER, (err, data) => {
                if(err) {
                    console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                expect(typeof(data)).assertEqual("object");
                if(data) {            
                    try{
                        sensor.once(sensor.SensorId.BAROMETER, callback, 5);
                        setTimeout(()=>{
                            expect(true).assertTrue();
                            done();
                        }, 500);
                    } catch (error) {
                        console.error('Once fail, errCode:' + error.code + ' ,msg:' + error.message);
                        expect(false).assertTrue();
                        done();
                    }
                } else {
                    console.info('Non-mobile devices cannot be excluded');
                    expect(true).assertTrue();
                    done();
                }
            });
        } catch (error) {
            console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
    * @tc.name:BarometerJsTest_007
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("BarometerJsTest_007", 0, async function (done) {
        console.info('----------------------BarometerJsTest_007---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.BAROMETER, (err, data) => {
                if(err) {
                    console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                expect(typeof(data)).assertEqual("object");
                if(data) {            
                    try{
                        sensor.once(sensor.SensorId.BAROMETER, 5);
                    } catch (error) {
                        console.error('On fail, errCode:' + error.code + ' ,msg:' + error.message);
                        expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
                        expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
                        done();
                    }
                } else {
                    console.info('Non-mobile devices cannot be excluded');
                    expect(true).assertTrue();
                    done();
                }
            });
        } catch (error) {
            console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
    * @tc.name:BarometerJsTest_008
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("BarometerJsTest_008", 0, async function (done) {
        console.info('----------------------BarometerJsTest_008---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.BAROMETER, (err, data) => {
                if(err) {
                    console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                expect(typeof(data)).assertEqual("object");
                if(data) {            
                    try {
                        sensor.off(-1, callback);
                    } catch (error) {
                        expect(error.code).assertEqual(PARAMETER_ERROR_CODE)
                        expect(error.message).assertEqual(PARAMETER_ERROR_MSG)
                        done();
                    }
                } else {
                    console.info('Non-mobile devices cannot be excluded');
                    expect(true).assertTrue();
                    done();
                }
            });
        } catch (error) {
            console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
    * @tc.name:BarometerJsTest_009
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("BarometerJsTest_009", 0, async function (done) {
        console.info('----------------------BarometerJsTest_009---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.BAROMETER, (err, data) => {
                if(err) {
                    console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                expect(typeof(data)).assertEqual("object");
                if(data) {            
                    try {
                        sensor.on(sensor.SensorId.BAROMETER, callback);
                        sensor.on(sensor.SensorId.BAROMETER, callback2);
                        setTimeout(()=>{
                            console.info('----------------------BarometerJsTest_009 off in---------------------------');
                            sensor.off(sensor.SensorId.BAROMETER);
                            console.info('----------------------BarometerJsTest_009 off end---------------------------');
                            done();
                        }, 1000);
                    } catch (error) {
                        console.error('On fail, errCode:' + error.code + ' ,msg:' + error.message);
                        expect(false).assertTrue();
                        done();
                    }
                } else {
                    console.info('Non-mobile devices cannot be excluded');
                    expect(true).assertTrue();
                    done();
                }
            });
        } catch (error) {
            console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
    * @tc.name:BarometerJsTest_010
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("BarometerJsTest_010", 0, async function (done) {
        console.info('----------------------BarometerJsTest_010---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.BAROMETER, (err, data) => {
                if(err) {
                    console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                expect(typeof(data)).assertEqual("object");
                if(data) {            
                    try {
                        sensor.on(sensor.SensorId.BAROMETER, callback);
                        sensor.on(sensor.SensorId.BAROMETER, callback2);
                        setTimeout(()=>{
                            console.info('----------------------BarometerJsTest_010 off in---------------------------');
                            sensor.off(sensor.SensorId.BAROMETER, callback);
                            console.info('----------------------BarometerJsTest_010 off end---------------------------');
                        }, 500);
                        setTimeout(()=>{
                            console.info('----------------------BarometerJsTest_010 off in---------------------------');
                            sensor.off(sensor.SensorId.BAROMETER, callback2);
                            console.info('----------------------BarometerJsTest_010 off end---------------------------');
                            done();
                        }, 1000);
                    } catch (error) {
                        console.error('On fail, errCode:' + error.code + ' ,msg:' + error.message);
                        expect(false).assertTrue();
                        done();
                    }
                } else {
                    console.info('Non-mobile devices cannot be excluded');
                    expect(true).assertTrue();
                    done();
                }
            });
        } catch (error) {
            console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
    * @tc.name:BarometerJsTest_011
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("BarometerJsTest_011", 0, async function (done) {
        console.info('----------------------BarometerJsTest_011---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.BAROMETER, (err, data) => {
                if(err) {
                    console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                expect(typeof(data)).assertEqual("object");
                if(data) {            
                    try {
                        sensor.on(sensor.SensorId.BAROMETER, callback, {'interval': 100000000});
                        sensor.once(sensor.SensorId.BAROMETER, callback2);
                        setTimeout(()=>{
                            console.info('----------------------BarometerJsTest_011 off in---------------------------');
                            sensor.off(sensor.SensorId.BAROMETER);
                            console.info('----------------------BarometerJsTest_011 off end---------------------------');
                            done();
                        }, 1000);
                    } catch (error) {
                        console.error('On fail, errCode:' + error.code + ' ,msg:' + error.message);
                        expect(false).assertTrue();
                        done();
                    }
                } else {
                    console.info('Non-mobile devices cannot be excluded');
                    expect(true).assertTrue();
                    done();
                }
            });
        } catch (error) {
            console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
    * @tc.name:BarometerJsTest_012
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("BarometerJsTest_012", 0, async function (done) {
        console.info('----------------------BarometerJsTest_012---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.BAROMETER, (err, data) => {
                if(err) {
                    console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                expect(typeof(data)).assertEqual("object");
                if(data) {            
                    try {
                        sensor.on(sensor.SensorId.BAROMETER, callback, {'interval': 100000000});
                        sensor.on(sensor.SensorId.BAROMETER, callback2, {'interval': 100000000});
                        setTimeout(()=>{
                            console.info('----------------------BarometerJsTest_012 off in---------------------------');
                            sensor.off(sensor.SensorId.BAROMETER, callback);
                            console.info('----------------------BarometerJsTest_012 off end---------------------------');
                        }, 500);
                        setTimeout(()=>{
                            console.info('----------------------BarometerJsTest_012 off in---------------------------');
                            sensor.off(sensor.SensorId.BAROMETER, callback2);
                            console.info('----------------------BarometerJsTest_012 off end---------------------------');
                            done();
                        }, 1000);
                    } catch (error) {
                        console.error('On fail, errCode:' + error.code + ' ,msg:' + error.message);
                        expect(false).assertTrue();
                        done();
                    }
                } else {
                    console.info('Non-mobile devices cannot be excluded');
                    expect(true).assertTrue();
                    done();
                }
            });
        } catch (error) {
            console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
    * @tc.name:BarometerJsTest_013
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("BarometerJsTest_013", 0, async function (done) {
        console.info('----------------------BarometerJsTest_013---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.BAROMETER, (err, data) => {
                if(err) {
                    console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                expect(typeof(data)).assertEqual("object");
                if(data) {            
                    try {
                        sensor.on(sensor.SensorId.BAROMETER, callback, {'interval': 100000000});
                        sensor.on(sensor.SensorId.BAROMETER, callback2, {'interval': 100000000});
                        setTimeout(()=>{
                            console.info('----------------------BarometerJsTest_013 off in---------------------------');
                            sensor.off(sensor.SensorId.BAROMETER);
                            console.info('----------------------BarometerJsTest_013 off end---------------------------');
                            done();
                        }, 1000);
                    } catch (error) {
                        console.error('On fail, errCode:' + error.code + ' ,msg:' + error.message);
                        expect(false).assertTrue();
                        done();
                    }
                } else {
                    console.info('Non-mobile devices cannot be excluded');
                    expect(true).assertTrue();
                    done();
                }
            });
        } catch (error) {
            console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })
})