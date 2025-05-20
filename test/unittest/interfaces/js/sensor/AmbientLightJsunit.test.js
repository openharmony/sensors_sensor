/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
import CommonConstants from './CommonConstants';
import sensor from '@ohos.sensor'

import {describe, beforeAll, beforeEach, afterEach, afterAll, it, expect} from 'deccjsunit/index'

describe("AmbientLightJsTest", function () {
    function callback(data) {
        console.info("callback" + JSON.stringify(data));
        expect(typeof(data.intensity)).assertEqual("number");
        expect(typeof(data.colorTemperature)).assertEqual("number");
        expect(typeof(data.infraredLuminance)).assertEqual("number");
    }

    function callback2(data) {
        console.info("callback2" + JSON.stringify(data));
        expect(typeof(data.intensity)).assertEqual("number");
        expect(typeof(data.colorTemperature)).assertEqual("number");
        expect(typeof(data.infraredLuminance)).assertEqual("number");
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

    /*
     * @tc.number: AmbientLightJsTest_001
     * @tc.name: AmbientLightJsTest
     * @tc.desc: verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     * @tc.size: MediumTest
     * @tc.type: Function
     * @tc.level: Level 1
     */
    it("AmbientLightJsTest_001", 0, async function (done) {
        console.info('----------------------AmbientLightJsTest_001---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.AMBIENT_LIGHT, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.AMBIENT_LIGHT, callback);
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.AMBIENT_LIGHT);
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
            expect(error.code).assertEqual(CommonConstants.SENSOR_NO_SUPPORT_CODE);
            expect(error.message).assertEqual(CommonConstants.SENSOR_NO_SUPPOR_MSG);
            done();
        }
    })

    /*
    * @tc.number: AmbientLightJsTest_002
    * @tc.name: AmbientLightJsTest
    * @tc.desc: verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    * @tc.size: MediumTest
    * @tc.type: Function
    * @tc.level: Level 1
    */
    it("AmbientLightJsTest_002", 0, async function (done) {
        console.info('----------------------AmbientLightJsTest_002---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.AMBIENT_LIGHT, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.AMBIENT_LIGHT, callback, {'interval': 100000000});
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.AMBIENT_LIGHT);
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
            expect(error.code).assertEqual(CommonConstants.SENSOR_NO_SUPPORT_CODE);
            expect(error.message).assertEqual(CommonConstants.SENSOR_NO_SUPPOR_MSG);
            done();
        }
    })

    /*
    * @tc.number: AmbientLightJsTest_003
    * @tc.name: AmbientLightJsTest
    * @tc.desc: verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    * @tc.size: MediumTest
    * @tc.type: Function
    * @tc.level: Level 1
    */
    it("AmbientLightJsTest_003", 0, async function (done) {
        console.info('----------------------AmbientLightJsTest_003---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.AMBIENT_LIGHT, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                function onSensorCallback(data) {
                    console.error('AmbientLightJsTest_003  callback in');
                    expect(true).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.AMBIENT_LIGHT, onSensorCallback, {'interval': 100000000}, 5);
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.AMBIENT_LIGHT);
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
            expect(error.code).assertEqual(CommonConstants.SENSOR_NO_SUPPORT_CODE);
            expect(error.message).assertEqual(CommonConstants.SENSOR_NO_SUPPOR_MSG);
            done();
        }
    })

    /*
    * @tc.number: AmbientLightJsTest_004
    * @tc.name: AmbientLightJsTest
    * @tc.desc: verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    * @tc.size: MediumTest
    * @tc.type: Function
    * @tc.level: Level 1
    */
    it("AmbientLightJsTest_004", 0, async function (done) {
        console.info('----------------------AmbientLightJsTest_004---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.AMBIENT_LIGHT, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.AMBIENT_LIGHT, callback, {'interval': -100000000});
                    sensor.off(sensor.SensorId.AMBIENT_LIGHT);
                    done();
                } catch (error) {
                    console.error('On fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(error.code).assertEqual(CommonConstants.SERVICE_EXCEPTION_CODE);
                    expect(error.message).assertEqual(CommonConstants.SERVICE_EXCEPTION_MSG);
                    done();
                }
            });
        } catch (error) {
            console.info('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(CommonConstants.SENSOR_NO_SUPPORT_CODE);
            expect(error.message).assertEqual(CommonConstants.SENSOR_NO_SUPPOR_MSG);
            done();
        }
    })

    /*
    * @tc.number: AmbientLightJsTest_005
    * @tc.name: AmbientLightJsTest
    * @tc.desc: verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    * @tc.size: MediumTest
    * @tc.type: Function
    * @tc.level: Level 1
    */
    it("AmbientLightJsTest_005", 0, async function (done) {
        console.info('----------------------AmbientLightJsTest_005---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.AMBIENT_LIGHT, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.once(sensor.SensorId.AMBIENT_LIGHT, callback);
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
            expect(error.code).assertEqual(CommonConstants.SENSOR_NO_SUPPORT_CODE);
            expect(error.message).assertEqual(CommonConstants.SENSOR_NO_SUPPOR_MSG);
            done();
        }
    })

    /*
    * @tc.number: AmbientLightJsTest_006
    * @tc.name: AmbientLightJsTest
    * @tc.desc: verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    * @tc.size: MediumTest
    * @tc.type: Function
    * @tc.level: Level 1
    */
    it("AmbientLightJsTest_006", 0, async function (done) {
        console.info('----------------------AmbientLightJsTest_006---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.AMBIENT_LIGHT, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try{
                    sensor.once(sensor.SensorId.AMBIENT_LIGHT, callback, 5);
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
            expect(error.code).assertEqual(CommonConstants.SENSOR_NO_SUPPORT_CODE);
            expect(error.message).assertEqual(CommonConstants.SENSOR_NO_SUPPOR_MSG);
            done();
        }
    })

    /*
    * @tc.number: AmbientLightJsTest_007
    * @tc.name: AmbientLightJsTest
    * @tc.desc: verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    * @tc.size: MediumTest
    * @tc.type: Function
    * @tc.level: Level 1
    */
    it("AmbientLightJsTest_007", 0, async function (done) {
        console.info('----------------------AmbientLightJsTest_007---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.AMBIENT_LIGHT, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try{
                    sensor.once(sensor.SensorId.AMBIENT_LIGHT, 5);
                } catch (error) {
                    console.error('On fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(error.code).assertEqual(CommonConstants.PARAMETER_ERROR_CODE);
                    done();
                }
            });
        } catch (error) {
            console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(CommonConstants.SENSOR_NO_SUPPORT_CODE);
            expect(error.message).assertEqual(CommonConstants.SENSOR_NO_SUPPOR_MSG);
            done();
        }
    })

    /*
    * @tc.number: AmbientLightJsTest_008
    * @tc.name: AmbientLightJsTest
    * @tc.desc: verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    * @tc.size: MediumTest
    * @tc.type: Function
    * @tc.level: Level 1
    */
    it("AmbientLightJsTest_008", 0, async function (done) {
        console.info('----------------------AmbientLightJsTest_008---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.AMBIENT_LIGHT, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.off(-1, callback);
                } catch (error) {
                    expect(error.code).assertEqual(CommonConstants.PARAMETER_ERROR_CODE);
                    done();
                }
            });
        } catch (error) {
            console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
            expect(error.code).assertEqual(CommonConstants.SENSOR_NO_SUPPORT_CODE);
            expect(error.message).assertEqual(CommonConstants.SENSOR_NO_SUPPOR_MSG);
            done();
        }
    })

    /*
    * @tc.number: AmbientLightJsTest_009
    * @tc.name: AmbientLightJsTest
    * @tc.desc: verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    * @tc.size: MediumTest
    * @tc.type: Function
    * @tc.level: Level 1
    */
    it("AmbientLightJsTest_009", 0, async function (done) {
        console.info('----------------------AmbientLightJsTest_009---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.AMBIENT_LIGHT, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.AMBIENT_LIGHT, callback);
                    sensor.on(sensor.SensorId.AMBIENT_LIGHT, callback2);
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.AMBIENT_LIGHT);
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
            expect(error.code).assertEqual(CommonConstants.SENSOR_NO_SUPPORT_CODE);
            expect(error.message).assertEqual(CommonConstants.SENSOR_NO_SUPPOR_MSG);
            done();
        }
    })

    /*
    * @tc.number: AmbientLightJsTest_010
    * @tc.name: AmbientLightJsTest
    * @tc.desc: verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    * @tc.size: MediumTest
    * @tc.type: Function
    * @tc.level: Level 1
    */
    it("AmbientLightJsTest_010", 0, async function (done) {
        console.info('----------------------AmbientLightJsTest_010---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.AMBIENT_LIGHT, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.AMBIENT_LIGHT, callback);
                    sensor.on(sensor.SensorId.AMBIENT_LIGHT, callback2);
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.AMBIENT_LIGHT, callback);
                    }, 500);
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.AMBIENT_LIGHT, callback2);
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
            expect(error.code).assertEqual(CommonConstants.SENSOR_NO_SUPPORT_CODE);
            expect(error.message).assertEqual(CommonConstants.SENSOR_NO_SUPPOR_MSG);
            done();
        }
    })

    /*
    * @tc.number: AmbientLightJsTest_011
    * @tc.name: AmbientLightJsTest
    * @tc.desc: verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    * @tc.size: MediumTest
    * @tc.type: Function
    * @tc.level: Level 1
    */
    it("AmbientLightJsTest_011", 0, async function (done) {
        console.info('----------------------AmbientLightJsTest_011---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.AMBIENT_LIGHT, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.AMBIENT_LIGHT, callback, {'interval': 100000000});
                    sensor.once(sensor.SensorId.AMBIENT_LIGHT, callback2);
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.AMBIENT_LIGHT);
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
            expect(error.code).assertEqual(CommonConstants.SENSOR_NO_SUPPORT_CODE);
            expect(error.message).assertEqual(CommonConstants.SENSOR_NO_SUPPOR_MSG);
            done();
        }
    })

    /*
    * @tc.number: AmbientLightJsTest_012
    * @tc.name: AmbientLightJsTest
    * @tc.desc: verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    * @tc.size: MediumTest
    * @tc.type: Function
    * @tc.level: Level 1
    */
    it("AmbientLightJsTest_012", 0, async function (done) {
        console.info('----------------------AmbientLightJsTest_012---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.AMBIENT_LIGHT, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.AMBIENT_LIGHT, callback, {'interval': 100000000});
                    sensor.on(sensor.SensorId.AMBIENT_LIGHT, callback2, {'interval': 100000000});
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.AMBIENT_LIGHT, callback);
                    }, 500);
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.AMBIENT_LIGHT, callback2);
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
            expect(error.code).assertEqual(CommonConstants.SENSOR_NO_SUPPORT_CODE);
            expect(error.message).assertEqual(CommonConstants.SENSOR_NO_SUPPOR_MSG);
            done();
        }
    })

    /*
    * @tc.number: AmbientLightJsTest_013
    * @tc.name: AmbientLightJsTest
    * @tc.desc: verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    * @tc.size: MediumTest
    * @tc.type: Function
    * @tc.level: Level 1
    */
    it("AmbientLightJsTest_013", 0, async function (done) {
        console.info('----------------------AmbientLightJsTest_013---------------------------');
        try {
            sensor.getSingleSensor(sensor.SensorId.AMBIENT_LIGHT, (error, data) => {
                if (error) {
                    console.error('getSingleSensor fail, errCode:' + error.code + ' ,msg:' + error.message);
                    expect(false).assertTrue();
                    done();
                }
                try {
                    sensor.on(sensor.SensorId.AMBIENT_LIGHT, callback, {'interval': 100000000});
                    sensor.on(sensor.SensorId.AMBIENT_LIGHT, callback2, {'interval': 100000000});
                    setTimeout(() => {
                        sensor.off(sensor.SensorId.AMBIENT_LIGHT);
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
            expect(error.code).assertEqual(CommonConstants.SENSOR_NO_SUPPORT_CODE);
            expect(error.message).assertEqual(CommonConstants.SENSOR_NO_SUPPOR_MSG);
            done();
        }
    })
})
