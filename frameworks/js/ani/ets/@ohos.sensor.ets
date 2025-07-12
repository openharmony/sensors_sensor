/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import type { Callback } from '@ohos.base';
import { BusinessError } from '@ohos.base';

export default namespace sensor {
    loadLibrary("sensor_ani");

    export native function on(type: 'orientationChange', callback: Callback<OrientationResponse>, options?: Options): void;
    export native function off(type: 'orientationChange', callback?: Callback<OrientationResponse>): void;
    export type SensorFrequency = 'game' | 'ui' | 'normal';
    export interface Options {
        interval?: number | SensorFrequency;
    }

    export enum SensorAccuracy {
        ACCURACY_UNRELIABLE = 0,
        ACCURACY_LOW = 1,
        ACCURACY_MEDIUM = 2,
        ACCURACY_HIGH = 3
    }

    export interface Response {
        timestamp: number;
        accuracy: SensorAccuracy;
    }

    export interface OrientationResponse extends Response {
        alpha: number;
        beta: number;
        gamma: number;
    }

    class OrientationResponseImpl implements OrientationResponse {
        timestamp: number;
        accuracy: SensorAccuracy;
        alpha: number;
        beta: number;
        gamma: number;
    }
}