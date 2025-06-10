/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
 
import sensor from '@ohos.sensor';
import {BusinessError} from '@ohos.base';

function testGetRotationMatrix() {
    console.log('taihetest testGetRotationMatrix begin');
    try {
        let gravity: float[] = [-0.27775216, 0.5351276, 9.788099];
        let geomagnetic: float[] = [210.87253, -78.6096, -111.44444];
        sensor.getRotationMatrix(gravity, geomagnetic, (err: BusinessError, data: sensor.RotationMatrixResponse) => {
            if (err.code != 0) {
                console.error(`Failed to get rotationMatrix. Code: ${err.code}, message: ${err.message}`);
                return;
            }
            console.info('Succeeded in getting rotationMatrix: ' + JSON.stringify(data));
        })
    } catch (error) {
        let e: BusinessError = error as BusinessError;
        console.error(`Failed to get rotationMatrix. Code: ${e.code}, message: ${e.message}`);
    }
    console.log('taihetest testGetRotationMatrix end');
}

function testGetOrientation() {
    console.log('taihetest testGetOrientation begin');
    try {
        let preRotationMatrix: float[] = [1, 0, 0, 0, 0.87, -0.50, 0, 0.50, 0.87];
        sensor.getOrientation(preRotationMatrix, (err: BusinessError, data: Array<float>) => {
            if (err.code != 0) {
                console.error(`Failed to get orientation. Code: ${err.code}, message: ${err.message}`);
                return;
            }
            console.info('Succeeded in getting data: ' + JSON.stringify(data));
        })
    } catch (error) {
        let e: BusinessError = error as BusinessError;
        console.error(`Failed to get orientation. Code: ${e.code}, message: ${e.message}`);
    }
    console.log('taihetest testGetOrientation end');
}

function testGetRotationMatrixSync() {
    console.log('taihetest testGetRotationMatrixSync begin');
    try {
        let rotationVector: float[] = [0.20046076, 0.21907, 0.73978853, 0.60376877];
        sensor.getRotationMatrix(rotationVector, (err: BusinessError, data: Array<float>) => {
            if (err.code != 0) {
                console.error(`Failed to get rotationMatrix. Code: ${err.code}, message: ${err.message}`);
                return;
            }
            console.info('GetRotationMatrixSync succeeded in getting data: ' + JSON.stringify(data));
        })
    } catch (error) {
        let e: BusinessError = error as BusinessError;
        console.error(`Failed to get rotationMatrix. Code: ${e.code}, message: ${e.message}`);
    }
    console.log('taihetest testGetRotationMatrixSync end');
}

function testGetSensorList() {
    console.log('taihetest testGetSensorList begin');
    try {
        sensor.getSensorList((err: BusinessError, data: Array<sensor.Sensor>) => {
            if (err.code != 0) {
                console.error(`Failed to get sensorList. Code: ${err.code}, message: ${err.message}`);
                return;
            }
            console.info('GetSensorList succeeded in getting data: ' + JSON.stringify(data));
        });
    } catch (error) {
        let e: BusinessError = error as BusinessError;
        console.error(`Failed to get sensorList. Code: ${e.code}, message: ${e.message}`);
    }
    console.log('taihetest testGetSensorList end');
}

function testOnAccelerometer() {
    console.log('taihetest testOnAccelerometer begin');
    try {
        let intervalData: long = 100000000;
        let optionTest: sensor.Options = {
            interval: intervalData
        };
        sensor.on(sensor.SensorId.ACCELEROMETER, (data: sensor.AccelerometerResponse): void => {
            console.info('Succeeded in invoking on accelerometer data: ' + JSON.stringify(data));
        }, optionTest);
    } catch (error) {
        let e: BusinessError = error as BusinessError;
        console.error(`Failed to invoke onAccelerometer. Code: ${e.code}, message: ${e.message}`);
    }
    console.log('taihetest testOnAccelerometer end');
}

function testOnceAccelerometer() {
    console.log('taihetest testOnceAccelerometer begin');
    try {
        sensor.once(sensor.SensorId.ACCELEROMETER, (data: sensor.AccelerometerResponse): void => {
            console.info('Succeeded in invoking once accelerometer data: ' + JSON.stringify(data));
        });
    } catch (error) {
        let e: BusinessError = error as BusinessError;
        console.error(`Failed to invoke onceAccelerometer. Code: ${e.code}, message: ${e.message}`);
    }
    console.log('taihetest testOnceAccelerometer end');
}

function testOffAccelerometer() {
    console.log('taihetest testOffAccelerometer begin');
    try {
        sensor.off(sensor.SensorId.ACCELEROMETER, (data: sensor.AccelerometerResponse): void => {
            console.info('Succeeded in invoking off accelerometer data: ' + JSON.stringify(data));
        });
    } catch (error) {
        let e: BusinessError = error as BusinessError;
        console.error(`Failed to invoke offAccelerometer. Code: ${e.code}, message: ${e.message}`);
    }
    console.log('taihetest testOffAccelerometer end');
}

function testOnAmbientLight() {
    console.log('taihetest testOnAmbientLight begin');
    try {
        let intervalData: long = 100000000;
        let optionTest: sensor.Options = {
            interval: intervalData
        };
        sensor.on(sensor.SensorId.AMBIENT_LIGHT, (data: sensor.LightResponse): void => {
            console.info('Succeeded in invoking on onAmbientLight data: ' + JSON.stringify(data));
        }, optionTest);
    } catch (error) {
        let e: BusinessError = error as BusinessError;
        console.error(`Failed to invoke onAmbientLight. Code: ${e.code}, message: ${e.message}`);
    }
    console.log('taihetest testOnAmbientLight end');
}

function testOnceAmbientLight() {
    console.log('taihetest testOnceAmbientLight begin');
    try {
        sensor.once(sensor.SensorId.AMBIENT_LIGHT, (data: sensor.LightResponse): void => {
            console.info('Succeeded in invoking once onAmbientLight data: ' + JSON.stringify(data));
        });
    } catch (error) {
        let e: BusinessError = error as BusinessError;
        console.error(`Failed to invoke onAmbientLight. Code: ${e.code}, message: ${e.message}`);
    }
    console.log('taihetest testOnceAmbientLight end');
}

function testOffAmbientLight() {
    console.log('taihetest testOffAmbientLight begin');
    try {
        sensor.off(sensor.SensorId.AMBIENT_LIGHT, (data: sensor.LightResponse): void => {
            console.info('Succeeded in invoking off onAmbientLight data: ' + JSON.stringify(data));
        });
    } catch (error) {
        let e: BusinessError = error as BusinessError;
        console.error(`Failed to invoke onAmbientLight. Code: ${e.code}, message: ${e.message}`);
    }
    console.log('taihetest testOffAmbientLight end');
}

function main() {
    testGetRotationMatrix();
    testGetOrientation();
    testGetRotationMatrixSync();
    testGetSensorList();
    testOnAccelerometer();
    testOnceAccelerometer();
    testOffAccelerometer();
    testOnAmbientLight();
    testOnceAmbientLight();
    testOffAmbientLight();
}