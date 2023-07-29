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

describe("PostureJsTest", function () {
    function callback(data) {
        console.info("callback" + JSON.stringify(data));
        expect(typeof(data.mX)).assertEqual("number");
        expect(typeof(data.mY)).assertEqual("number");
        expect(typeof(data.mZ)).assertEqual("number");
        expect(typeof(data.mXSecond)).assertEqual("number");
        expect(typeof(data.mYSecond)).assertEqual("number");
        expect(typeof(data.mZSecond)).assertEqual("number");
        expect(typeof(data.mPosture)).assertEqual("number");
    }

    function callback2(data) {
        console.info("callback2" + JSON.stringify(data));
        expect(typeof(data.mX)).assertEqual("number");
        expect(typeof(data.mY)).assertEqual("number");
        expect(typeof(data.mZ)).assertEqual("number");
        expect(typeof(data.mXSecond)).assertEqual("number");
        expect(typeof(data.mYSecond)).assertEqual("number");
        expect(typeof(data.mZSecond)).assertEqual("number");
        expect(typeof(data.mPosture)).assertEqual("number");
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
     * @tc.name:PostureJsTest_001
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("PostureJsTest_001", 0, async function (done) {
        console.info('----------------------PostureJsTest_001---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.POSTURE, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.POSTURE, callback);
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.POSTURE);
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
    * @tc.name:PostureJsTest_002
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("PostureJsTest_002", 0, async function (done) {
        console.info('----------------------PostureJsTest_002---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.POSTURE, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.POSTURE, callback, {'interval': 100000000});
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.POSTURE);
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
    * @tc.name:PostureJsTest_003
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("PostureJsTest_003", 0, async function (done) {
        console.info('----------------------PostureJsTest_003---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.POSTURE, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                function onSensorCallback(data) {
                    console.error('PostureJsTest_003  callback in');
                    expect(true).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.POSTURE, onSensorCallback, {'interval': 100000000}, 5);
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.POSTURE);
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
    * @tc.name:PostureJsTest_004
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("PostureJsTest_004", 0, async function (done) {
        console.info('----------------------PostureJsTest_004---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.POSTURE, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.POSTURE, callback, {'interval': -100000000});
                    sensor.off(sensor.SensorId.POSTURE);
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
    * @tc.name:PostureJsTest_005
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("PostureJsTest_005", 0, async function (done) {
        console.info('----------------------PostureJsTest_005---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.POSTURE, (error, data) => {
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
    * @tc.name:PostureJsTest_006
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("PostureJsTest_006", 0, async function (done) {
        console.info('----------------------PostureJsTest_006---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.POSTURE, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.POSTURE, callback);
                    sensor.on(sensor.SensorId.POSTURE, callback2);
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.POSTURE);
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
    * @tc.name:PostureJsTest_007
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("PostureJsTest_007", 0, async function (done) {
        console.info('----------------------PostureJsTest_007---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.POSTURE, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.POSTURE, callback);
                    sensor.on(sensor.SensorId.POSTURE, callback2);
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.POSTURE, callback);
                    }, 500);
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.POSTURE, callback2);
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
    * @tc.name:PostureJsTest_008
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("PostureJsTest_008", 0, async function (done) {
        console.info('----------------------PostureJsTest_008---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.POSTURE, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.POSTURE, callback, {'interval': 100000000});
                    sensor.on(sensor.SensorId.POSTURE, callback2, {'interval': 100000000});
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.POSTURE, callback);
                    }, 500);
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.POSTURE, callback2);
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
    * @tc.name:PostureJsTest_009
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("PostureJsTest_009", 0, async function (done) {
        console.info('----------------------PostureJsTest_009---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.POSTURE, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.POSTURE, callback, {'interval': 100000000});
                    sensor.on(sensor.SensorId.POSTURE, callback2, {'interval': 100000000});
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.POSTURE);
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
