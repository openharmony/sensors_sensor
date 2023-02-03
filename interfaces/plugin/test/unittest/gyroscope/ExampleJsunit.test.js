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
import deviceInfo from '@ohos.deviceInfo'
import sensor from '@ohos.sensor'

import {describe, beforeAll, beforeEach, afterEach, afterAll, it, expect} from 'deccjsunit/index'

describe("GyroscopeJsTest", function () {
    var g_execute = false;
    function callback(data) {
        console.info("callback" + JSON.stringify(data));
        expect(typeof(data.x)).assertEqual("number");
        expect(typeof(data.y)).assertEqual("number");
        expect(typeof(data.z)).assertEqual("number");
    }

    function callback2() {
        console.info("callback2" + JSON.stringify(data));
        expect(typeof(data.x)).assertEqual("number");
        expect(typeof(data.y)).assertEqual("number");
        expect(typeof(data.z)).assertEqual("number");
    }

    beforeAll(function() {
        /*
         * @tc.setup: setup invoked before all testcases
         */
         console.info('beforeAll caled')
         if(deviceInfo.deviceType != "default") {
             g_execute = true;
         }
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
     * @tc.name:GyroscopeJsTest_001
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("GyroscopeJsTest_001", 0, async function (done) {
        console.info('----------------------GyroscopeJsTest_001---------------------------');
        if(g_execute) {
            try {
                sensor.on(sensor.SensorId.GYROSCOPE, callback);
                setTimeout(()=>{
                    sensor.off(sensor.SensorId.GYROSCOPE);
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
    })

    /*
    * @tc.name:GyroscopeJsTest_002
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("GyroscopeJsTest_002", 0, async function (done) {
        console.info('----------------------GyroscopeJsTest_002---------------------------');
        if(g_execute) {
            try {
                sensor.on(sensor.SensorId.GYROSCOPE, callback, {'interval':100000000});
                setTimeout(()=>{
                    console.info('------------------GyroscopeJsTest_002 off in-----------------------');
                    sensor.off(sensor.SensorId.GYROSCOPE);
                    console.info('------------------GyroscopeJsTest_002 off end-----------------------');
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
    })

    /*
    * @tc.name:GyroscopeJsTest_003
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("GyroscopeJsTest_003", 0, async function (done) {
        console.info('----------------------GyroscopeJsTest_003---------------------------');
        if(g_execute) {
            function onSensorCallback(data) {
                console.info('GyroscopeJsTest_003  callback in');
                expect(true).assertTrue();
                done();
            }
            try {
                sensor.on(sensor.SensorId.GYROSCOPE, onSensorCallback, {'interval': 100000000}, 5);
                setTimeout(()=>{
                    console.info('----------------------GyroscopeJsTest_003 off in---------------------------');
                    sensor.off(sensor.SensorId.GYROSCOPE);
                    console.info('----------------------GyroscopeJsTest_003 off end---------------------------');
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
    })

    /*
    * @tc.name:GyroscopeJsTest_004
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("GyroscopeJsTest_004", 0, async function (done) {
        console.info('----------------------GyroscopeJsTest_004---------------------------');
        if(g_execute) {
            try {
                sensor.on(sensor.SensorId.GYROSCOPE, callback, {'interval': -100000000});
                console.info('----------------------GyroscopeJsTest_004 off in---------------------------');
                sensor.off(sensor.SensorId.GYROSCOPE);
                console.info('----------------------GyroscopeJsTest_004 off end---------------------------');
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
    })

    /*
    * @tc.name:GyroscopeJsTest_005
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("GyroscopeJsTest_005", 0, async function (done) {
        console.info('----------------------GyroscopeJsTest_005---------------------------');
        if(g_execute) {
            try {
                sensor.once(sensor.SensorId.GYROSCOPE, callback);
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
    })

    /*
    * @tc.name:GyroscopeJsTest_006
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("GyroscopeJsTest_006", 0, async function (done) {
        console.info('----------------------GyroscopeJsTest_006---------------------------');
        if(g_execute) {
            try{
                sensor.once(sensor.SensorId.ACCELEROMETER, callback, 5);
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
    })

    /*
    * @tc.name:GyroscopeJsTest_007
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("GyroscopeJsTest_007", 0, async function (done) {
        console.info('----------------------GyroscopeJsTest_007---------------------------');
        if(g_execute) {
            try{
                sensor.once(sensor.SensorId.GYROSCOPE, 5);
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
    })

    /*
    * @tc.name:GyroscopeJsTest_008
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("GyroscopeJsTest_008", 0, async function (done) {
        console.info('----------------------GyroscopeJsTest_008---------------------------');
        if(g_execute) {
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
    })

    /*
    * @tc.name:GyroscopeJsTest_009
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("GyroscopeJsTest_009", 0, async function (done) {
        console.info('----------------------GyroscopeJsTest_009---------------------------');
        if(g_execute) {
            try {
                sensor.on(sensor.SensorId.GYROSCOPE, callback);
                sensor.on(sensor.SensorId.GYROSCOPE, callback2);
                setTimeout(()=>{
                    console.info('----------------------GyroscopeJsTest_009 off in---------------------------');
                    sensor.off(sensor.SensorId.GYROSCOPE);
                    console.info('----------------------GyroscopeJsTest_009 off end---------------------------');
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
    })

    /*
    * @tc.name:GyroscopeJsTest_010
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("GyroscopeJsTest_010", 0, async function (done) {
        console.info('----------------------GyroscopeJsTest_010---------------------------');
        if(g_execute) {
            try {
                sensor.on(sensor.SensorId.GYROSCOPE, callback);
                sensor.on(sensor.SensorId.GYROSCOPE, callback2);
                setTimeout(()=>{
                    console.info('----------------------GyroscopeJsTest_010 off in---------------------------');
                    sensor.off(sensor.SensorId.GYROSCOPE, callback);
                    console.info('----------------------GyroscopeJsTest_010 off end---------------------------');
                }, 500);
                setTimeout(()=>{
                    console.info('----------------------GyroscopeJsTest_010 off in---------------------------');
                    sensor.off(sensor.SensorId.GYROSCOPE, callback2);
                    console.info('----------------------GyroscopeJsTest_010 off end---------------------------');
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
    })

    /*
    * @tc.name:GyroscopeJsTest_011
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("GyroscopeJsTest_011", 0, async function (done) {
        console.info('----------------------GyroscopeJsTest_011---------------------------');
        if(g_execute) {
            try {
                sensor.on(sensor.SensorId.GYROSCOPE, callback, {'interval': 100000000});
                sensor.once(sensor.SensorId.GYROSCOPE, callback2);
                setTimeout(()=>{
                    console.info('----------------------GyroscopeJsTest_011 off in---------------------------');
                    sensor.off(sensor.SensorId.GYROSCOPE);
                    console.info('----------------------GyroscopeJsTest_011 off end---------------------------');
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
    })

    /*
    * @tc.name:GyroscopeJsTest_012
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("GyroscopeJsTest_012", 0, async function (done) {
        console.info('----------------------GyroscopeJsTest_012---------------------------');
        if(g_execute) {
            try {
                sensor.on(sensor.SensorId.GYROSCOPE, callback, {'interval': 100000000});
                sensor.on(sensor.SensorId.GYROSCOPE, callback2, {'interval': 100000000});
                setTimeout(()=>{
                    console.info('----------------------GyroscopeJsTest_012 off in---------------------------');
                    sensor.off(sensor.SensorId.GYROSCOPE, callback);
                    console.info('----------------------GyroscopeJsTest_012 off end---------------------------');
                }, 500);
                setTimeout(()=>{
                    console.info('----------------------GyroscopeJsTest_012 off in---------------------------');
                    sensor.off(sensor.SensorId.GYROSCOPE, callback2);
                    console.info('----------------------GyroscopeJsTest_012 off end---------------------------');
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
    })

    /*
    * @tc.name:GyroscopeJsTest_013
    * @tc.desc:verify app info is not null
    * @tc.type: FUNC
    * @tc.require: Issue Number
    */
    it("GyroscopeJsTest_013", 0, async function (done) {
        console.info('----------------------GyroscopeJsTest_013---------------------------');
        if(g_execute) {
            try {
                sensor.on(sensor.SensorId.GYROSCOPE, callback, {'interval': 100000000});
                sensor.on(sensor.SensorId.GYROSCOPE, callback2, {'interval': 100000000});
                setTimeout(()=>{
                    console.info('----------------------GyroscopeJsTest_013 off in---------------------------');
                    sensor.off(sensor.SensorId.GYROSCOPE);
                    console.info('----------------------GyroscopeJsTest_013 off end---------------------------');
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
    })
})
