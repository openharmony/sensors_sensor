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
import sensor from '@system.sensor'

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
     * @tc.require: SR000H0ALK, AR000H0ALM
     */
    it("SensorJsTest001", 0, async function (done) {
        console.info('----------------------SensorJsTest001---------------------------');
        sensor.subscribeAccelerometer({
            interval: 'normal',
            success: function(data) {
              expect(typeof(data.x)).assertEqual("number");
              expect(typeof(data.y)).assertEqual("number");
              expect(typeof(data.z)).assertEqual("number");
              console.info("SensorJsTest001 success" + JSON.stringify(data));
            },
            fail: function(data, code) {
              expect(false).assertTrue();
              console.error('Subscription failed. Code: ' + code + '; Data: ' + data);
            },
        });
        setTimeout(()=>{
            try {
                sensor.unsubscribeAccelerometer();
            } catch (error) {
                console.info('SensorJsTest001 unsubscribe failed' + error);
                expect(false).assertTrue();
            }
            setTimeout(()=>{
                expect(true).assertTrue();
                done();
            }, 500);
        }, 1000);
    })

    /*
     * @tc.name:SensorJsTest002
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: SR000H0ALK, AR000H0ALM
     */
    it("SensorJsTest002", 0, async function (done) {
        console.info('----------------------SensorJsTest002---------------------------');
        sensor.subscribeAccelerometer({
            interval: 'xxx',
            success: function(data) {
              expect(false).assertTrue();
              console.info("SensorJsTest002 success" + JSON.stringify(data));
              done();
            },
            fail: function(data, code) {
              expect(true).assertTrue();
              console.error('SensorJsTest002 Subscription failed. Code: ' + code + '; Data: ' + data);
              done();
            },
          });
    })

    /*
     * @tc.name:SensorJsTest003
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: SR000H0ALK, AR000H0ALM
     */
    it("SensorJsTest003", 0, async function (done) {
        console.info('----------------------SensorJsTest003---------------------------');
        try {
            sensor.subscribeAccelerometer({
                interval: 'xxx',
                success: function(data) {
                  expect(false).assertTrue();
                  console.info("SensorJsTest003 success" + JSON.stringify(data));
                  done();
                }
              });
        } catch (error) {
            console.info('SensorJsTest003 Subscription failed' + error);
            expect(true).assertTrue();
            done();
        }
    })

    /*
     * @tc.name:SensorJsTest004
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: SR000H0ALK, AR000H0ALM
     */
    it("SensorJsTest004", 0, async function (done) {
        console.info('----------------------SensorJsTest004---------------------------');
        try {
            sensor.subscribeAccelerometer({
                interval: 'normal',
            });
        } catch (error) {
            console.info('SensorJsTest004 Subscription failed' + error);
            expect(true).assertTrue();
            done();
        }
    })

    /*
     * @tc.name:SensorJsTest005
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: SR000H0ALK, AR000H0ALM
     */
    it("SensorJsTest005", 0, async function (done) {
        console.info('----------------------SensorJsTest005---------------------------');
        try {
            sensor.unsubscribeAccelerometer();
        } catch (error) {
            console.info(error);
            expect(true).assertTrue();
            done();
        }
    })

    /*
     * @tc.name:SensorJsTest006
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: SR000H0ALK, AR000H0ALM
     */
    it("SensorJsTest006", 0, async function (done) {
        console.info('----------------------SensorJsTest006---------------------------');
        try {
            sensor.unsubscribeAccelerometer('xxx');
        } catch (error) {
            console.info(error);
            expect(true).assertTrue();
            done();
        }
    })

    /*
     * @tc.name:SensorJsTest007
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: SR000H0ALK, AR000H0ALM
     */
    it("SensorJsTest007", 0, async function (done) {
        console.info('----------------------SensorJsTest007---------------------------');
        sensor.subscribeCompass({
            success: function(data) {
               console.log('SensorJsTest007 get data direction:' + ret.direction);
               expect(typeof(data.direction)).assertEqual("number");
            },
            fail: function(data, code) {
                console.error('SensorJsTest007 Subscription failed. Code: ' + code + '; Data: ' + data);
                expect(false).assertTrue();
            },
        });
        setTimeout(()=>{
            try {
                sensor.unsubscribeCompass();
            } catch (error) {
                console.info(error);
                expect(false).assertTrue();
            }
            setTimeout(()=>{
                expect(true).assertTrue();
                done();
            }, 500);
        }, 1000);
    })

    /*
     * @tc.name:SensorJsTest008
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: SR000H0ALK, AR000H0ALM
     */
    it("SensorJsTest008", 0, async function (done) {
        sensor.subscribeProximity({
            success: function(data) {
                expect(typeof(data.distance)).assertEqual("number");
                console.info("SensorJsTest008 subscribeProximity" + JSON.stringify(data));
            },
            fail: function(data, code) {
                console.error('SensorJsTest008 Subscription failed. Code: ' + code + '; Data: ' + data);
                expect(true).assertTrue();
            },
          });
        setTimeout(()=>{
            try {
                sensor.unsubscribeProximity();
            } catch (error) {
                console.info(error);
            }
            setTimeout(()=>{
                expect(true).assertTrue();
                done();
            }, 500);
        }, 1000);
    })

    /*
     * @tc.name:SensorJsTest009
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: SR000H0ALK, AR000H0ALM
     */
    it("SensorJsTest009", 0, function (done) {
        sensor.subscribeLight({
            success: function(data) {
                expect(typeof(data.intensity)).assertEqual("number");
                console.info("SensorJsTest009 subscribeLight" + JSON.stringify(data));
            },
            fail: function(data, code) {
                console.error('SensorJsTest009 Subscription failed. Code: ' + code + '; Data: ' + data);
                expect(false).assertTrue();
            },
          });
        setTimeout(()=>{
            try {
                sensor.unsubscribeLight();
            } catch (error) {
                console.info(error);
                expect(false).assertTrue();
            }
            setTimeout(()=>{
                expect(true).assertTrue();
                done();
            }, 500);
        }, 1000);
    })

    /*
     * @tc.name:SensorJsTest010
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: SR000H0ALK, AR000H0ALM
     */
    it("SensorJsTest010", 0, async function (done) {
        sensor.subscribeStepCounter({
            success: function(data) {
                expect(typeof(data.steps)).assertEqual("number");
                console.info("SensorJsTest010 subscribeStepCounter" + JSON.stringify(data));
            },
            fail: function(data, code) {
                console.error('SensorJsTest010 Subscription failed. Code: ' + code + '; Data: ' + data);
                expect(true).assertTrue();
            },
          });
        setTimeout(()=>{
            try {
                sensor.unsubscribeStepCounter();
            } catch (error) {
                console.info(error);
            }
            setTimeout(()=>{
                expect(true).assertTrue();
                done();
            }, 500);
        }, 1000);
    })

    /*
     * @tc.name:SensorJsTest011
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: SR000H0ALK, AR000H0ALM
     */
    it("SensorJsTest011", 0, async function (done) {
        sensor.subscribeBarometer({
            success: function(data) {
                expect(typeof(data.pressure)).assertEqual("number");
                console.info("subscribeBarometer" + JSON.stringify(data));
            },
            fail: function(data, code) {
                console.error('SensorJsTest011 Subscription failed. Code: ' + code + '; Data: ' + data);
                expect(true).assertTrue();
            },
          });
        setTimeout(()=>{
            try {
                sensor.unsubscribeBarometer();
            } catch (error) {
                console.info(error);
            }
            setTimeout(()=>{
                expect(true).assertTrue();
                done();
            }, 500);
        }, 1000);
    })

    /*
     * @tc.name:SensorJsTest012
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: SR000H0ALK, AR000H0ALM
     */
    it("SensorJsTest012", 0, function (done) {
        sensor.subscribeHeartRate({
            success: function(data) {
                expect(typeof(data.heartRate)).assertEqual("number");
                console.info("SensorJsTest012 subscribeHeartRate" + JSON.stringify(data));
            },
            fail: function(data, code) {
                console.error('SensorJsTest012 Subscription failed. Code: ' + code + '; Data: ' + data);
                expect(true).assertTrue();
            },
          });
        setTimeout(()=>{
            try {
                sensor.unsubscribeHeartRate();
            } catch (error) {
                console.info(error);
            }
            setTimeout(()=>{
                expect(true).assertTrue();
                done();
            }, 500);
        }, 1000);
    })

    /*
     * @tc.name:SensorJsTest013
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: SR000H0ALK, AR000H0ALM
     */
    it("SensorJsTest013", 0, async function (done) {
        console.info('----------------------SensorJsTest013---------------------------');
        sensor.subscribeOnBodyState({
            success: function(data) {
                expect(typeof(data.value)).assertEqual("boolean");
                console.info("SensorJsTest013 subscribeOnBodyState" + JSON.stringify(data));
            },
            fail: function(data, code) {
                console.error('SensorJsTest013 Subscription failed. Code: ' + code + '; Data: ' + data);
                expect(true).assertTrue();
            },
          });
        setTimeout(()=>{
            try {
                sensor.unsubscribeOnBodyState();
            } catch (error) {
                console.info(error);
            }
            setTimeout(()=>{
                expect(true).assertTrue();
                done();
            }, 500);
        }, 1000);
    })

    /*
     * @tc.name:SensorJsTest014
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: SR000H0ALK, AR000H0ALM
     */
      it('SensorJsTest014', 0, async function (done) {
        console.info("---------------------------SensorJsTest014----------------------------------");
        try {
            sensor.getOnBodyState({
                success: function(data) {
                    expect(typeof(data.value)).assertEqual("boolean");
                    console.info("subscribeOnBodyState" + JSON.stringify(data));
                    done();
                }
            });
        } catch (error) {
            console.info(error);
            expect(false).assertTrue();
            done();
        }
    })

    /*
     * @tc.name:SensorJsTest015
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: SR000H0ALK, AR000H0ALM
     */
    it('SensorJsTest015', 0, async function (done) {
        console.info("---------------------------SensorJsTest015----------------------------------");
        try {
            sensor.getOnBodyState();
        } catch (error) {
            console.info(error);
            expect(true).assertTrue();
            done();
        }
    })

    /*
     * @tc.name:SensorJsTest016
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: SR000H0ALK, AR000H0ALM
     */
    it("SensorJsTest016", 0, async function (done) {
        console.info('----------------------SensorJsTest016---------------------------');
        sensor.subscribeDeviceOrientation({
            interval: 'normal',
            success: function(data) {
                expect(typeof(data.alpha)).assertEqual("number");
                expect(typeof(data.beta)).assertEqual("number");
                expect(typeof(data.gamma)).assertEqual("number");
                console.info("SensorJsTest016 subscribeDeviceOrientation" + JSON.stringify(data));
            },
            fail: function(data, code) {
                console.error('SensorJsTest016 Subscription failed. Code: ' + code + '; Data: ' + data);
                expect(false).assertTrue();
            },
          });
        setTimeout(()=>{
            try {
                sensor.unsubscribeDeviceOrientation();
            } catch (error) {
                console.info(error);
                expect(false).assertTrue();
            }
            setTimeout(()=>{
                expect(true).assertTrue();
                done();
            }, 500);
        }, 1000);
    })

    /*
     * @tc.name:SensorJsTest017
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: SR000H0ALK, AR000H0ALM
     */
    it("SensorJsTest017", 0, async function (done) {
        console.info('----------------------SensorJsTest017---------------------------');
        sensor.subscribeGyroscope({
            interval: 'normal',
            success: function(data) {
                expect(typeof(data.x)).assertEqual("number");
                expect(typeof(data.y)).assertEqual("number");
                expect(typeof(data.z)).assertEqual("number");
                console.info("SensorJsTest017 subscribeGyroscope" + JSON.stringify(data));
            },
            fail: function(data, code) {
                console.error('SensorJsTest017 Subscription failed. Code: ' + code + '; Data: ' + data);
                expect(false).assertTrue();
            },
          });
        setTimeout(()=>{
            try {
                sensor.unsubscribeGyroscope();
            } catch (error) {
                console.info(error);
                expect(false).assertTrue();
            }
            setTimeout(()=>{
                expect(true).assertTrue();
                done();
            }, 500);
        }, 1000);
    })
})
