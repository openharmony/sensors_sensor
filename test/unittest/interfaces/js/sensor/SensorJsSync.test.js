/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

describe("SensorSyncTest", function () {
    beforeAll(async function() {
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
     * @tc.name: SensorSyncTest_001
     * @tc.desc: verify sensor sync interface
     * @tc.size: MediumTest
     * @tc.type: Function
     * @tc.level: Level 1
     * @tc.require: Issue Number
     * @tc.number: SensorSyncTest_001
     */
    it("SensorSyncTest_001", 0, async function (done) {
        console.info('----------------------SensorSyncTest_001---------------------------');
        try {
            let ret = sensor.getSingleSensorSync(sensor.SensorId.ACCELEROMETER);
            console.info('getSingleSensorSync: ' + JSON.stringify(ret));
            done();
        } catch (err) {
            console.error('getSingleSensorSync err: ' + JSON.stringify(err));
            done();
        }
    })

    /*
     * @tc.name: SensorSyncTest_002
     * @tc.desc: verify sensor sync interface
     * @tc.size: MediumTest
     * @tc.type: Function
     * @tc.level: Level 1
     * @tc.require: Issue Number
     * @tc.number: SensorSyncTest_002
     */
    it("SensorSyncTest_002", 0, async function (done) {
        console.info('----------------------SensorSyncTest_002---------------------------');
        try {
            let ret = sensor.getSingleSensorSync(-1);
            expect(false).assertTrue();
            done();
        } catch (err) {
            console.error('getSingleSensorSync err: ' + JSON.stringify(err));
            expect(true).assertTrue();
            done();
        }
    })

    /*
     * @tc.name: SensorSyncTest_003
     * @tc.desc: verify sensor sync interface
     * @tc.size: MediumTest
     * @tc.type: Function
     * @tc.level: Level 1
     * @tc.require: Issue Number
     * @tc.number: SensorSyncTest_003
     */
    it("SensorSyncTest_003", 0, async function (done) {
        console.info('----------------------SensorSyncTest_003---------------------------');
        try {
            let ret = sensor.getSensorListSync();
            console.info('getSensorListSync: ' + JSON.stringify(ret));
            done();
        } catch (err) {
            console.error('getSensorListSync err: ' + JSON.stringify(err));
            expect(err.code).assertEqual(CommonConstants.SERVICE_EXCEPTION_CODE);
            done();
        }
    })

    /*
     * @tc.name: SensorSyncTest_004
     * @tc.desc: verify sensor sync interface, call on and off if sensor exists
     * @tc.size: MediumTest
     * @tc.type: Function
     * @tc.level: Level 1
     * @tc.require: Issue Number
     * @tc.number: SensorSyncTest_004
     */
    it("SensorSyncTest_004", 0, async function (done) {
        console.info('----------------------SensorSyncTest_004---------------------------');
        try {
            let ret = sensor.getSingleSensorByDeviceSync(sensor.SensorId.ACCELEROMETER);
            console.info('getSingleSensorByDeviceSync: ' + JSON.stringify(ret));
            if (!Array.isArray(ret) || ret.length === 0) {
                console.info('No local sensors found. Test case will return true.');
                expect(true).assertTrue();
                done();
                return;
            }
            const sensorInfo = ret[0];
            const callback = (data) => {
                console.info('Accelerometer data received: ' + JSON.stringify(data));
            };
            const sensorInfoParam = {
                deviceId: sensorInfo.deviceId,
                sensorIndex: sensorInfo.sensorIndex
            };
            const options = {
                interval: 10000000,
                sensorInfoParam: sensorInfoParam
            };
            sensor.on(sensorInfo.sensorId, callback, options);
            sensor.off(sensorInfo.sensorId, sensorInfoParam, callback);
            expect(true).assertTrue();
            done();
        } catch (err) {
            console.error('getSingleSensorByDeviceSync err: ' + JSON.stringify(err));
            expect(false).assertEqual(true);
            done();
        }
    });

    /*
     * @tc.name: SensorSyncTest_005
     * @tc.desc: verify sensor sync interface, call on and off if sensor exists
     * @tc.size: MediumTest
     * @tc.type: Function
     * @tc.level: Level 1
     * @tc.require: Issue Number
     * @tc.number: SensorSyncTest_005
     */
    it("SensorSyncTest_005", 0, async function (done) {
        console.info('----------------------SensorSyncTest_005---------------------------');
        const validDeviceId = -1;
        try {
            let ret = sensor.getSingleSensorByDeviceSync(sensor.SensorId.ACCELEROMETER, validDeviceId);
            console.info(`getSingleSensorByDeviceSync deviceId=${validDeviceId}: ` + JSON.stringify(ret));
            if (!Array.isArray(ret) || ret.length === 0) {
                console.info('No local sensors found. Test case will return true.');
                expect(true).assertTrue();
                done();
                return;
            }
            const sensorInfo = ret[0];
            const callback = (data) => {
                console.info('Accelerometer data received: ' + JSON.stringify(data));
            };
            const sensorInfoParam = {
                deviceId: sensorInfo.deviceId,
                sensorIndex: sensorInfo.sensorIndex
            };
            const options = {
                interval: 10000000,
                sensorInfoParam: sensorInfoParam
            };
            sensor.on(sensorInfo.sensorId, callback, options);
            sensor.off(sensorInfo.sensorId, sensorInfoParam, callback);
            expect(true).assertTrue();
            done();
        } catch (err) {
            console.error(`getSingleSensorByDeviceSync deviceId=${validDeviceId} err: ` + JSON.stringify(err));
            expect(false).assertEqual(true);
            done();
        }
    });

    /*
     * @tc.name: SensorSyncTest_006
     * @tc.desc: verify sensor sync interface
     * @tc.size: MediumTest
     * @tc.type: Function
     * @tc.level: Level 1
     * @tc.require: Issue Number
     * @tc.number: SensorSyncTest_006
     */
    it("SensorSyncTest_006", 0, async function (done) {
        console.info('----------------------SensorSyncTest_006---------------------------');
        const invalidType = -1;
        try {
            let ret = sensor.getSingleSensorByDeviceSync(invalidType);
            console.info(`getSingleSensorByDeviceSync invalidType=${invalidType}: ` + JSON.stringify(ret));
            expect(Array.isArray(ret) && ret.length === 0).assertTrue();
            done();
        } catch (err) {
            console.error(`getSingleSensorByDeviceSync invalidType=${invalidType} err: ` + JSON.stringify(err));
            expect(false).assertEqual(true);
            done();
        }
    });

    /*
     * @tc.name: SensorSyncTest_007
     * @tc.desc: verify sensor sync interface
     * @tc.size: MediumTest
     * @tc.type: Function
     * @tc.level: Level 1
     * @tc.require: Issue Number
     * @tc.number: SensorSyncTest_007
     */
    it("SensorSyncTest_007", 0, async function (done) {
        console.info('----------------------SensorSyncTest_007---------------------------');
        const invalidDeviceId = -2;
        try {
            let ret = sensor.getSingleSensorByDeviceSync(sensor.SensorId.ACCELEROMETER, invalidDeviceId);
            console.info(`getSingleSensorByDeviceSync invalid deviceId=${invalidDeviceId}: ` + JSON.stringify(ret));
            expect(Array.isArray(ret) && ret.length === 0).assertTrue();
            done();
        } catch (err) {
            console.error('getSingleSensorByDeviceSync invalid deviceId err: ' + JSON.stringify(err));
            expect(false).assertEqual(true);
            done();
        }
    });

    /*
     * @tc.name: SensorSyncTest_008
     * @tc.desc: verify sensor sync interface, call on and off if sensor exists
     * @tc.size: MediumTest
     * @tc.type: Function
     * @tc.level: Level 1
     * @tc.require: Issue Number
     * @tc.number: SensorSyncTest_008
     */
    it("SensorSyncTest_008", 0, async function (done) {
        console.info('----------------------SensorSyncTest_008---------------------------');
        try {
            let ret = sensor.getSensorListByDeviceSync();
            console.info('getSensorListByDeviceSync default device: ' + JSON.stringify(ret));
            if (!Array.isArray(ret) || ret.length === 0) {
                console.info('No local sensors found. Test case will return true.');
                expect(true).assertTrue();
                done();
                return;
            }
            const sensorInfo = ret[0];
            const callback = (data) => {
                console.info('Sensor data received: ' + JSON.stringify(data));
            };
            const sensorInfoParam = {
                deviceId: sensorInfo.deviceId,
                sensorIndex: sensorInfo.sensorIndex
            };
            const options = {
                interval: 10000000,
                sensorInfoParam: sensorInfoParam
            };
            sensor.on(sensorInfo.sensorId, callback, options);
            sensor.off(sensorInfo.sensorId, sensorInfoParam, callback);
            expect(true).assertTrue();
            done();
        } catch (err) {
            console.error('getSensorListByDeviceSync default device err: ' + JSON.stringify(err));
            expect(false).assertEqual(true);
            done();
        }
    });

    /*
     * @tc.name: SensorSyncTest_009
     * @tc.desc: verify sensor sync interface, call on and off if sensor exists
     * @tc.size: MediumTest
     * @tc.type: Function
     * @tc.level: Level 1
     * @tc.require: Issue Number
     * @tc.number: SensorSyncTest_009
     */
    it("SensorSyncTest_009", 0, async function (done) {
        console.info('----------------------SensorSyncTest_009---------------------------');
        const validDeviceId = -1;
        try {
            let ret = sensor.getSensorListByDeviceSync(validDeviceId);
            console.info(`getSensorListByDeviceSync deviceId=${validDeviceId}: ` + JSON.stringify(ret));
            if (!Array.isArray(ret) || ret.length === 0) {
                console.info('No local sensors found. Test case will return true.');
                expect(true).assertTrue();
                done();
                return;
            }
            const sensorInfo = ret[0];
            const callback = (data) => {
                console.info(`Sensor ${sensorInfo.type} data received: ` + JSON.stringify(data));
            };
            const sensorInfoParam = {
                deviceId: sensorInfo.deviceId,
                sensorIndex: sensorInfo.sensorIndex
            };
            const options = {
                interval: 10000000,
                sensorInfoParam: sensorInfoParam
            };
            sensor.on(sensorInfo.sensorId, callback, options);
            sensor.off(sensorInfo.sensorId, sensorInfoParam, callback);
            expect(true).assertTrue();
            done();
        } catch (err) {
            console.error(`getSensorListByDeviceSync deviceId=${validDeviceId} err: ` + JSON.stringify(err));
            expect(false).assertEqual(true);
            done();
        }
    });

    /*
     * @tc.name: SensorSyncTest_010
     * @tc.desc: verify sensor sync interface
     * @tc.size: MediumTest
     * @tc.type: Function
     * @tc.level: Level 1
     * @tc.require: Issue Number
     * @tc.number: SensorSyncTest_010
     */
    it("SensorSyncTest_010", 0, async function (done) {
        console.info('----------------------SensorSyncTest_010---------------------------');
        const invalidDeviceId = -2;
        try {
            let ret = sensor.getSensorListByDeviceSync(invalidDeviceId);
            console.info(`getSensorListByDeviceSync invalid deviceId=${invalidDeviceId}: ` + JSON.stringify(ret));
            expect(Array.isArray(ret) && ret.length === 0).assertTrue();
            done();
        } catch (err) {
            console.error('getSensorListByDeviceSync invalid deviceId err: ' + JSON.stringify(err));
            expect(err.code).assertEqual(CommonConstants.INVALID_PARAMETER_CODE);
            done();
        }
    });
})