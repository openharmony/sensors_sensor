/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef GEOMAGNETIC_FIELD_H
#define GEOMAGNETIC_FIELD_H

#include <cmath>
#include <ctime>
#include <cstring>
#include <iostream>
#include <vector>

#include <stdint.h>
#include <stdio.h>
#include <time.h>

class GeomagneticField {
public:

    GeomagneticField(float latitude, float longitude, float altitude, int64_t timeMillis);

    ~GeomagneticField() = default;

    float obtainX();

    float obtainY();

    float obtainZ();

    float obtainGeomagneticDip();

    float obtainDeflectionAngle();

    float obtainLevelIntensity();

    float obtainTotalIntensity();

private:
    float northComponent;
    float eastComponent;
    float downComponent;
    static float geocentricLatitude;
    static float geocentricLongitude;
    static float geocentricRadius;
    const static float EARTH_MAJOR_AXIS_RADIUS;
    const static float EARTH_MINOR_AXIS_RADIUS;
    const static float EARTH_REFERENCE_RADIUS;
    const static float PRECISION;
    const static float LATITUDE_MAX;
    const static float LATITUDE_MIN;
    const static float CONVERSION_FACTOR;
    const static float DERIVATIVE_FACTOR;
    const static int64_t WMM_BASE_TIME;

    const static float GAUSS_COEFFICIENT_G[13][13];
    const static float GAUSS_COEFFICIENT_H[13][13];
    const static float DELTA_GAUSS_COEFFICIENT_G[13][13];
    const static float DELTA_GAUSS_COEFFICIENT_H[13][13];
    const static int32_t GAUSSIAN_COEFFICIENT_DIMENSION;
    static std::vector<std::vector<float>> schmidtQuasiNormalFactors;

    static std::vector<std::vector<float>> polynomials;
    static std::vector<std::vector<float>> polynomialsDerivative;
    static std::vector<float> relativeRadiusPower;
    static std::vector<float> sinMLongitude;
    static std::vector<float> cosMLongitude;

    static std::vector<std::vector<float>> getSchmidtQuasiNormalFactors(int32_t expansionDegree);
    void calculateGeomagneticComponent(double latDiffRad, int64_t timeMillis);
    static void getLongitudeTrigonometric();
    static void getRelativeRadiusPower();
    static void calibrateGeocentricCoordinates(float latitude, float longitude, float altitude);
    void initLegendreTable(int32_t expansionDegree, float thetaRad);
    double toDegrees(double angrad);
    static double toRadians(double angdeg);
};
#endif // GEOMAGNETIC_FIELD_H