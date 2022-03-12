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
        sensor.on(1, callback);
        setTimeout(()=>{
            sensor.off(1);
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
        function onSensorCallback(data) {
            console.info('SensorJsTest002  on error');
            expect(false).assertTrue();
            done();
        }
        sensor.on(-1, onSensorCallback);
        setTimeout(()=>{
            expect(true).assertTrue();
            done();
        }, 500);
    })

    /*
     * @tc.name:SensorJsTest003
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest003", 0, async function (done) {
        console.info('----------------------SensorJsTest003---------------------------');
        sensor.on(1, callback, {'interval': 100000000});
        setTimeout(()=>{
            console.info('----------------------SensorJsTest003 off in---------------------------');
            sensor.off(1);
            console.info('----------------------SensorJsTest003 off end---------------------------');
            done();
        }, 500);
    })

    /*
     * @tc.name:SensorJsTest004
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest004", 0, function () {
        console.info('----------------------SensorJsTest004---------------------------');
        function onSensorCallback(data) {
            console.info('SensorJsTest004  on error');
            expect(false).assertTrue();
            done();
        }
        sensor.on(1, onSensorCallback, {'interval': 100000000}, 5);
        setTimeout(()=>{
            expect(true).assertTrue();
            done();
        }, 500);
        console.info('----------------------SensorJsTest004--------------------------- end');
    })

    /*
     * @tc.name:SensorJsTest005
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest005", 0, async function (done) {
        sensor.once(1, callback);
        setTimeout(()=>{
            expect(true).assertTrue();
            done();
        }, 500);
    })

    /*
     * @tc.name:SensorJsTest006
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest006", 0, async function (done) {
        function onceSensorCallback(data) {
            console.info('SensorJsTest006  on error');
            expect(false).assertTrue();
            done();
        }
        sensor.once(-1, onceSensorCallback);
        setTimeout(()=>{
            expect(true).assertTrue();
            done();
        }, 500);
    })

    /*
     * @tc.name:SensorJsTest007
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest007", 0, function () {
        function onceSensorCallback(data) {
            console.info('SensorJsTest007  on error');
            expect(false).assertTrue();
            done();
        }
        sensor.once(1, onceSensorCallback, 5);
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
        sensor.off(-1, callback);
        setTimeout(()=>{
            expect(true).assertTrue();
            done();
        }, 500);
    })

    /*
     * @tc.name:SensorJsTest009
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest009", 0, async function (done) {
        function onSensorCallback(data) {
            console.info('SensorJsTest009  on error');
            expect(false).assertTrue();
            done();
        }
        sensor.on(1, onSensorCallback);
        sensor.off(1, onSensorCallback);
        setTimeout(()=>{
            expect(true).assertTrue();
            done();
        }, 500);
    })

    /*
     * @tc.name:SensorJsTest010
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest010", 0, async function (done) {
        function onSensorCallback(data) {
            console.info('SensorJsTest010  on error');
            expect(false).assertTrue();
            done();
        }
        sensor.off(1000000, onSensorCallback);
        setTimeout(()=>{
            expect(true).assertTrue();
            done();
        }, 500);
    })

    /*
     * @tc.name:SensorJsTest011
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest011", 0, async function (done) {
        sensor.on(1, callback);
        sensor.on(1, callback2);
        setTimeout(()=>{
            console.info('----------------------SensorJsTest011 off in---------------------------');
            sensor.off(1);
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
        sensor.on(1, callback);
        sensor.on(1, callback2);
        setTimeout(()=>{
            console.info('----------------------SensorJsTest012 off in---------------------------');
            sensor.off(1, callback);
            sensor.off(1, callback2);
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
    it("SensorJsTest013", 0, function () {
        sensor.off(1, 5);
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
        sensor.on(1, callback, {'interval': 100000000});
        sensor.once(1, callback2);
        setTimeout(()=>{
            console.info('----------------------SensorJsTest014 off in---------------------------');
            sensor.off(1, callback);
            sensor.off(1, callback2);
            console.info('----------------------SensorJsTest014 off end---------------------------');
            done();
        }, 1000);
    })

    /*
     * @tc.name:SensorJsTest015
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest015", 0, async function (done) {
        console.info('----------------------SensorJsTest015---------------------------');
        sensor.on(1, callback, {'interval': 100000000});
        sensor.on(1, callback2, {'interval': 100000000});
        setTimeout(()=>{
            console.info('----------------------SensorJsTest015 off in---------------------------');
            sensor.off(1, callback);
            sensor.off(1, callback2);
            console.info('----------------------SensorJsTest015 off end---------------------------');
            done();
        }, 1000);
    })

    /*
     * @tc.name:SensorJsTest016
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: Issue Number
     */
    it("SensorJsTest016", 0, async function (done) {
        console.info('----------------------SensorJsTest016---------------------------');
        sensor.on(1, callback, {'interval': 100000000});
        sensor.on(1, callback2, {'interval': 100000000});
        setTimeout(()=>{
            console.info('----------------------SensorJsTest016 off in---------------------------');
            sensor.off(1);
            console.info('----------------------SensorJsTest016 off end---------------------------');
            done();
        }, 1000);
    })
})
