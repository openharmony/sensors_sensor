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

describe("ColorJsTest", function () {
    function callback(data) {
        console.info("callback" + JSON.stringify(data));
        expect(typeof(data.lightIntensity)).assertEqual("number");
        expect(typeof(data.colorTemperature)).assertEqual("number");
    }

    function callback2(data) {
        console.info("callback2" + JSON.stringify(data));
        expect(typeof(data.lightIntensity)).assertEqual("number");
        expect(typeof(data.colorTemperature)).assertEqual("number");
    }

    beforeAll(function() {
        /*
         * @tc.setup: setup invoked before all testcases
         */
         console.info('beforeAll called')
    })

    afterAll(function() {
        /*
         * @tc.teardown: teardown invoked after all testcases
         */
         console.info('afterAll called')
    })

    beforeEach(function() {
        /*
         * @tc.setup: setup invoked before each testcases
         */
         console.info('beforeEach called')
    })

    afterEach(function() {
        /*
         * @tc.teardown: teardown invoked after each testcases
         */
         console.info('afterEach called')
    })

    const PARAMETER_ERROR_CODE = 401
    const SERVICE_EXCEPTION_CODE = 14500101
    const PARAMETER_ERROR_MSG = 'The parameter invalid.'
    const SERVICE_EXCEPTION_MSG = 'Service exception.'

    /*
     * @tc.name:ColorJsTest_001
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("ColorJsTest_001", 0, async function (done) {
        console.info('----------------------ColorJsTest_001---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.COLOR, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.COLOR, callback);
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.COLOR);
                        done();
                    }, 500);
                } catch (error) {
                    console.error('On fail, errCode:' + error.code + ' ,msg:' + error.message);
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
    * @tc.name:ColorJsTest_002
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("ColorJsTest_002", 0, async function (done) {
        console.info('----------------------ColorJsTest_002---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.COLOR, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.COLOR, callback, {'interval': 100000000});
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.COLOR);
                        done();
                    }, 500);
                } catch (error) {
                    console.error('On fail, errCode:' + error.code + ' ,msg:' + error.message);
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
    * @tc.name:ColorJsTest_003
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("ColorJsTest_003", 0, async function (done) {
        console.info('----------------------ColorJsTest_003---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.COLOR, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                function onSensorCallback(data) {
                    console.error('ColorJsTest_003  callback in');
                    expect(true).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.COLOR, onSensorCallback, {'interval': 100000000}, 5);
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.COLOR);
                        done();
                    }, 500);
                } catch (error) {
                    console.info('On fail, errCode:' + error.code + ' ,msg:' + error.message);
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
    * @tc.name:ColorJsTest_004
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("ColorJsTest_004", 0, async function (done) {
        console.info('----------------------ColorJsTest_004---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.COLOR, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.COLOR, callback, {'interval': -100000000});
                    sensor.off(sensor.SensorId.COLOR);
                    done();
                } catch (error) {
                    console.error('On fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(error.code).assertEqual(SERVICE_EXCEPTION_CODE);
                    expect(error.message).assertEqual(SERVICE_EXCEPTION_MSG);
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
    * @tc.name:ColorJsTest_005
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("ColorJsTest_005", 0, async function (done) {
        console.info('----------------------ColorJsTest_005---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.COLOR, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.once(sensor.SensorId.COLOR, callback);
                    setTimeout(() => {
                        expect(true).assertTrue();
                        done();
                    }, 500);
                } catch (error) {
                    console.error('Once fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
            });
        } catch (error) {
            console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
    * @tc.name:ColorJsTest_006
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("ColorJsTest_006", 0, async function (done) {
        console.info('----------------------ColorJsTest_006---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.COLOR, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try{
                    sensor.once(sensor.SensorId.COLOR, callback, 5);
                    setTimeout(() => {
                        expect(true).assertTrue();
                        done();
                    }, 500);
                } catch (error) {
                    console.error('Once fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
            });
        } catch (error) {
            console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
    * @tc.name:ColorJsTest_007
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("ColorJsTest_007", 0, async function (done) {
        console.info('----------------------ColorJsTest_007---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.COLOR, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try{
                    sensor.once(sensor.SensorId.COLOR, 5);
                } catch (error) {
                    console.error('On fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
                    expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
                    done();
                }
            });
        } catch (error) {
            console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
    * @tc.name:ColorJsTest_008
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("ColorJsTest_008", 0, async function (done) {
        console.info('----------------------ColorJsTest_008---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.COLOR, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.off(-1, callback);
                } catch (error) {
                    expect(error.code).assertEqual(PARAMETER_ERROR_CODE)
                    expect(error.message).assertEqual(PARAMETER_ERROR_MSG)
                    done();
                }
            });
        } catch (error) {
            console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
    * @tc.name:ColorJsTest_009
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("ColorJsTest_009", 0, async function (done) {
        console.info('----------------------ColorJsTest_009---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.COLOR, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.COLOR, callback);
                    sensor.on(sensor.SensorId.COLOR, callback2);
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.COLOR);
                        done();
                    }, 1000);
                } catch (error) {
                    console.error('On fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
            });
        } catch (error) {
            console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
    * @tc.name:ColorJsTest_010
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("ColorJsTest_010", 0, async function (done) {
        console.info('----------------------ColorJsTest_010---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.COLOR, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.COLOR, callback);
                    sensor.on(sensor.SensorId.COLOR, callback2);
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.COLOR, callback);
                    }, 500);
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.COLOR, callback2);
                        done();
                    }, 1000);
                } catch (error) {
                    console.error('On fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
            });
        } catch (error) {
            console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
    * @tc.name:ColorJsTest_011
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("ColorJsTest_011", 0, async function (done) {
        console.info('----------------------ColorJsTest_011---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.COLOR, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.COLOR, callback, {'interval': 100000000});
                    sensor.once(sensor.SensorId.COLOR, callback2);
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.COLOR);
                        done();
                    }, 1000);
                } catch (error) {
                    console.error('On fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
            });
        } catch (error) {
            console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
    * @tc.name:ColorJsTest_012
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("ColorJsTest_012", 0, async function (done) {
        console.info('----------------------ColorJsTest_012---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.COLOR, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.COLOR, callback, {'interval': 100000000});
                    sensor.on(sensor.SensorId.COLOR, callback2, {'interval': 100000000});
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.COLOR, callback);
                    }, 500);
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.COLOR, callback2);
                        done();
                    }, 1000);
                } catch (error) {
                    console.error('On fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
            });
        } catch (error) {
            console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })

    /*
    * @tc.name:ColorJsTest_013
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("ColorJsTest_013", 0, async function (done) {
        console.info('----------------------ColorJsTest_013---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.COLOR, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.COLOR, callback, {'interval': 100000000});
                    sensor.on(sensor.SensorId.COLOR, callback2, {'interval': 100000000});
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.COLOR);
                        done();
                    }, 1000);
                } catch (error) {
                    console.error('On fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
            });
        } catch (error) {
            console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(PARAMETER_ERROR_CODE);
            expect(error.message).assertEqual(PARAMETER_ERROR_MSG);
            done();
        }
    })
})
