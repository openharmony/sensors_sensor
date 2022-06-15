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
    function callback(data) {
        console.info("callback" + JSON.stringify(data));
        expect(typeof(data.x)).assertEqual("number");
        expect(typeof(data.y)).assertEqual("number");
        expect(typeof(data.z)).assertEqual("number");
    }

    function callback2(data) {
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
        sensor.on(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD, callback);
        setTimeout(()=>{
            sensor.off(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD);
            done();
        }, 500);
    })

    /*
     * @tc.name:SensorJsTest002
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest002", 0, async function (done) {
        console.info('----------------------SensorJsTest002---------------------------');
        sensor.on(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD, callback, {'interval': 100000000});
        setTimeout(()=>{
            console.info('----------------------SensorJsTest002 off in---------------------------');
            sensor.off(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD);
            console.info('----------------------SensorJsTest002 off end---------------------------');
            done();
        }, 500);
    })

    /*
     * @tc.name:SensorJsTest003
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest003", 0, function (done) {
        console.info('----------------------SensorJsTest003---------------------------');
        function onSensorCallback(data) {
            console.info('SensorJsTest003  on error');
            expect(false).assertTrue();
            done();
        }
        try {
            sensor.on(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD, onSensorCallback, {'interval': 100000000}, 5);
        } catch (error) {
            console.info(error);
            expect(true).assertTrue();
            done();
        }
    })

    /*
     * @tc.name:SensorJsTest004
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest004", 0, async function (done) {
        sensor.once(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD, callback);
        setTimeout(()=>{
            expect(true).assertTrue();
            done();
        }, 500);
    })

    /*
     * @tc.name:SensorJsTest005
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest005", 0, function (done) {
        function onceSensorCallback(data) {
            console.info('SensorJsTest005  on error');
            expect(false).assertTrue();
            done();
        }
        try{
            sensor.once(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD, onceSensorCallback, 5);
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
     * @tc.require: Issue Number
     */
    it("SensorJsTest006", 0, async function (done) {
        try {
            sensor.off(string, "");
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
     * @tc.require: Issue Number
     */
    it("SensorJsTest007", 0, async function (done) {
        function onSensorCallback(data) {
            console.info('SensorJsTest007  on error');
            expect(false).assertTrue();
            done();
        }
        sensor.on(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD, onSensorCallback);
        sensor.off(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD, onSensorCallback);
        setTimeout(()=>{
            expect(true).assertTrue();
            done();
        }, 500);
    })

    /*
     * @tc.name:SensorJsTest008
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest008", 0, async function (done) {
        function onSensorCallback(data) {
            console.info('SensorJsTest008  on error');
            expect(false).assertTrue();
            done();
        }
        try {
            sensor.off(1000000, onSensorCallback);
        } catch (error) {
            console.info(error);
            expect(true).assertTrue();
            done();
        }
    })

    /*
     * @tc.name:SensorJsTest009
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest009", 0, async function (done) {
        sensor.on(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD, callback);
        sensor.on(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD, callback2);
        setTimeout(()=>{
            console.info('----------------------SensorJsTest009 off in---------------------------');
            sensor.off(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD);
            console.info('----------------------SensorJsTest009 off end---------------------------');
            done();
        }, 1000);
    })

    /*
     * @tc.name:SensorJsTest010
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest010", 0, async function (done) {
        sensor.on(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD, callback);
        sensor.on(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD, callback2);
        setTimeout(()=>{
            console.info('----------------------SensorJsTest010 off in---------------------------');
            sensor.off(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD, callback);
            console.info('----------------------SensorJsTest010 off end---------------------------');
        }, 500);
        setTimeout(()=>{
            console.info('----------------------SensorJsTest010 off in---------------------------');
            sensor.off(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD, callback2);
            console.info('----------------------SensorJsTest010 off end---------------------------');
            done();
        }, 1000);
    })

    /*
     * @tc.name:SensorJsTest011
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest011", 0, async function (done) {
        console.info('----------------------SensorJsTest011---------------------------');
        sensor.on(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD, callback, {'interval': 100000000});
        sensor.once(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD, callback2);
        setTimeout(()=>{
            console.info('----------------------SensorJsTest011 off in---------------------------');
            sensor.off(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD);
            console.info('----------------------SensorJsTest011 off end---------------------------');
            done();
        }, 1000);
    })

    /*
     * @tc.name:SensorJsTest012
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest012", 0, async function (done) {
        console.info('----------------------SensorJsTest012---------------------------');
        sensor.on(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD, callback, {'interval': 100000000});
        sensor.on(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD, callback2, {'interval': 100000000});
        setTimeout(()=>{
            console.info('----------------------SensorJsTest012 off in---------------------------');
            sensor.off(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD, callback);
            console.info('----------------------SensorJsTest012 off end---------------------------');
        }, 500);
        setTimeout(()=>{
            console.info('----------------------SensorJsTest012 off in---------------------------');
            sensor.off(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD, callback2);
            console.info('----------------------SensorJsTest012 off end---------------------------');
            done();
        }, 1000);
    })

    /*
     * @tc.name:SensorJsTest013
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest013", 0, async function (done) {
        console.info('----------------------SensorJsTest013---------------------------');
        sensor.on(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD, callback, {'interval': 100000000});
        sensor.on(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD, callback2, {'interval': 100000000});
        setTimeout(()=>{
            console.info('----------------------SensorJsTest013 off in---------------------------');
            sensor.off(sensor.SensorType.SENSOR_TYPE_ID_MAGNETIC_FIELD);
            console.info('----------------------SensorJsTest013 off end---------------------------');
            done();
        }, 1000);
    })
})