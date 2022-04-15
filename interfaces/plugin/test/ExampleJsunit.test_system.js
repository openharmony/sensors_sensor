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
     * @tc.require: Issue Number
     */
    it("SensorJsTest001", 0, async function (done) {
        console.info('----------------------SensorJsTest001---------------------------');
        sensor.subscribeAccelerometer({
            interval: 'normal',
            success: function(data) {
              expect(typeof(data.x)).assertEqual("number");
              expect(typeof(data.y)).assertEqual("number");
              expect(typeof(data.z)).assertEqual("number");
              console.info("callback2" + JSON.stringify(data));
              done();
            },
            fail: function(data, code) {
              expect(false).assertTrue();
              console.error('Subscription failed. Code: ' + code + '; Data: ' + data);
              done();
            },
          });
    })

    /*
     * @tc.name:SensorJsTest002
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest002", 0, async function (done) {
        console.info('----------------------SensorJsTest002---------------------------');
        try {
            sensor.unsubscribeAccelerometer();
        } catch (error) {
            console.info(error);
            expect(false).assertTrue();
            done();
        }
        expect(true).assertTrue();
    })

    /*
     * @tc.name:SensorJsTest003
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest003", 0, async function (done) {
        console.info('----------------------SensorJsTest003---------------------------');
        sensor.subscribeCompass({
            success: function(data) {
               console.log('get data direction:' + ret.direction);
               expect(typeof(data.direction)).assertEqual("number");
               done();
            },
            fail: function(data, code) {
                console.info(error);
                expect(false).assertTrue();
                done();
            },
          });
    })

    /*
     * @tc.name:SensorJsTest004
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest004", 0, function (done) {
        console.info('----------------------SensorJsTest004---------------------------');
        try {
            sensor.unsubscribeCompass();
        } catch (error) {
            console.info(error);
            expect(false).assertTrue();
            done();
        }
        expect(true).assertTrue();
    })

    /*
     * @tc.name:SensorJsTest005
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest005", 0, async function (done) {
        sensor.subscribeProximity({
            success: function(data) {
                expect(typeof(data.distance)).assertEqual("number");
                console.info("subscribeProximity" + JSON.stringify(data));
                done();
            },
            fail: function(data, code) {
                console.info(error);
                expect(false).assertTrue();
                done();
            },
          });
    })

    /*
     * @tc.name:SensorJsTest006
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest006", 0, async function (done) {
        try {
            sensor.unsubscribeProximity();
        } catch (error) {
            console.info(error);
            expect(false).assertTrue();
            done();
        }
        expect(true).assertTrue();
    })

    /*
     * @tc.name:SensorJsTest007
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest007", 0, function (done) {
        sensor.subscribeLight({
            success: function(data) {
                expect(typeof(data.intensity)).assertEqual("number");
                console.info("subscribeLight" + JSON.stringify(data));
                done();
            },
            fail: function(data, code) {
                console.info(error);
                expect(false).assertTrue();
                done();
            },
          });
    })

    /*
     * @tc.name:SensorJsTest008
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest008", 0, async function (done) {
        try {
            sensor.unsubscribeLight();
        } catch (error) {
            console.info(error);
            expect(false).assertTrue();
            done();
        }
        expect(true).assertTrue();
    })

    /*
     * @tc.name:SensorJsTest009
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest009", 0, async function (done) {
        sensor.subscribeStepCounter({
            success: function(data) {
                expect(typeof(data.steps)).assertEqual("number");
                console.info("subscribeStepCounter" + JSON.stringify(data));
                done();
            },
            fail: function(data, code) {
                console.info(error);
                expect(false).assertTrue();
                done();
            },
          });
    })

    /*
     * @tc.name:SensorJsTest010
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest010", 0, async function (done) {
        try {
            sensor.unsubscribeStepCounter();
        } catch (error) {
            console.info(error);
            expect(false).assertTrue();
            done();
        }
        expect(true).assertTrue();
    })

    /*
     * @tc.name:SensorJsTest011
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest011", 0, async function (done) {
        sensor.subscribeBarometer({
            success: function(data) {
                expect(typeof(data.pressure)).assertEqual("number");
                console.info("subscribeBarometer" + JSON.stringify(data));
                done();
            },
            fail: function(data, code) {
                console.info(error);
                expect(false).assertTrue();
                done();
            },
          });
    })

    /*
     * @tc.name:SensorJsTest012
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest012", 0, async function (done) {
        try {
            sensor.unsubscribeBarometer();
        } catch (error) {
            console.info(error);
            expect(false).assertTrue();
            done();
        }
        expect(true).assertTrue();
    })

    /*
     * @tc.name:SensorJsTest013
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest013", 0, function (done) {
        sensor.subscribeHeartRate({
            success: function(data) {
                expect(typeof(data.heartRate)).assertEqual("number");
                console.info("subscribeHeartRate" + JSON.stringify(data));
                done();
            },
            fail: function(data, code) {
                console.info(error);
                expect(false).assertTrue();
                done();
            },
          });
    })

    /*
     * @tc.name:SensorJsTest014
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest014", 0, async function (done) {
        try {
            sensor.unsubscribeHeartRate();
        } catch (error) {
            console.info(error);
            expect(false).assertTrue();
            done();
        }
        expect(true).assertTrue();
    })

    /*
     * @tc.name:SensorJsTest015
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest015", 0, async function (done) {
        console.info('----------------------SensorJsTest015---------------------------');
        sensor.subscribeOnBodyState({
            success: function(data) {
                expect(typeof(data.value)).assertEqual("number");
                console.info("subscribeOnBodyState" + JSON.stringify(data));
                done();
            },
            fail: function(data, code) {
                console.info(error);
                expect(false).assertTrue();
                done();
            },
          });
    })

     /**
     * test
     *
     * @tc.name: SensorJsTest_016
     * @tc.desc: Verification results of the incorrect parameters of the test interface.
     * @tc.require: AR000GH2U6
     * @tc.author:
     */
      it('SensorJsTest016', 0, async function (done) {
        console.info("---------------------------SensorJsTest016----------------------------------");
        sensor.getOnBodyState({
            success: function(data) {
                expect(typeof(data.value)).assertEqual("number");
                console.info("subscribeOnBodyState" + JSON.stringify(data));
                done();
            }
          });
    })

    /*
     * @tc.name:SensorJsTest017
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest017", 0, async function (done) {
        console.info('----------------------SensorJsTest017---------------------------');
        try {
            sensor.unsubscribeOnBodyState();
        } catch (error) {
            console.info(error);
            expect(false).assertTrue();
            done();
        }
        expect(true).assertTrue();
    })
})
