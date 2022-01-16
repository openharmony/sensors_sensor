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

#include "geomagnetic_field.h"
#include "sensors_log_domain.h"

using namespace OHOS::HiviewDFX;
using namespace std;
static const HiLogLabel LABEL = {LOG_CORE, OHOS::SensorsLogDomain::SENSORS_INTERFACE, "GeomagneticField"};

const float GeomagneticField::EARTH_MAJOR_AXIS_RADIUS = 6378.137f;
const float GeomagneticField::EARTH_MINOR_AXIS_RADIUS = 6356.7523142f;
const float GeomagneticField::EARTH_REFERENCE_RADIUS = 6371.2f;
const float GeomagneticField::PRECISION = 1e-5f;
const float GeomagneticField::LATITUDE_MAX = 90.0f;
const float GeomagneticField::LATITUDE_MIN = -90.0f;
const float GeomagneticField::CONVERSION_FACTOR  = 1000.0f;
const float GeomagneticField::DERIVATIVE_FACTOR  = 1.0f;
// the time from 1970-01-01 to 2020-01-01 as UTC milliseconds from the epoch
const int64_t GeomagneticField::WMM_BASE_TIME = 1580486400000;

/**
* The following Gaussian coefficients are derived from the US/ United Kingdom World Magnetic Model 2020-2025.
*/
const float GeomagneticField::GAUSS_COEFFICIENT_G[13][13] = {
    {0.0f},
    {-29404.5f, -1450.7f},
    {-2500.0f, 2982.0f, 1676.8f},
    {1363.9f, -2381.0f, 1236.2f, 525.7f},
    {903.1f, 809.4f, 86.2f, -309.4f, 47.9f},
    {-234.4f, 363.1f, 187.8f, -140.7f, -151.2f, 13.7f},
    {65.9f, 65.6f, 73.0f, -121.5f, -36.2f, 13.5f, -64.7f},
    {80.6f, -76.8f, -8.3f, 56.5f, 15.8f, 6.4f, -7.2f, 9.8f},
    {23.6f, 9.8f, -17.5f, -0.4f, -21.1f, 15.3f, 13.7f, -16.5f, -0.3f},
    {5.0f, 8.2f, 2.9f, -1.4f, -1.1f, -13.3f, 1.1f, 8.9f, -9.3f, -11.9f},
    {-1.9f, -6.2f, -0.1f, 1.7f, -0.9f, 0.6f, -0.9f, 1.9f, 1.4f, -2.4f, -3.9f},
    {3.0f, -1.4f, -2.5f, 2.4f, -0.9f, 0.3f, -0.7f, -0.1f, 1.4f, -0.6f, 0.2f, 3.1f},
    {-2.0f, -0.1f, 0.5f, 1.3f, -1.2f, 0.7f, 0.3f, 0.5f, -0.2f, -0.5f, 0.1f, -1.1f, -0.3f}
};

const float GeomagneticField::GAUSS_COEFFICIENT_H[13][13] = {
    {0.0f},
    {0.0f, 4652.9f},
    {0.0f, -2991.6f, -734.8f},
    {0.0f, -82.2f, 241.8f, -542.9f},
    {0.0f, 282.0f, -158.4f, 199.8f, -350.1f},
    {0.0f, 47.7f, 208.4f, -121.3f, 32.2f, 99.1f},
    {0.0f, -19.1f, 25.0f, 52.7f, -64.4f, 9.0f, 68.1f},
    {0.0f, -51.4f, -16.8f, 2.3f, 23.5f, -2.2f, -27.2f, -1.9f},
    {0.0f, 8.4f, -15.3f, 12.8f, -11.8f, 14.9f, 3.6f, -6.9f, 2.8f},
    {0.0f, -23.3f, 11.1f, 9.8f, -5.1f, -6.2f, 7.8f, 0.4f, -1.5f, 9.7f},
    {0.0f, 3.4f, -0.2f, 3.5f, 4.8f, -8.6f, -0.1f, -4.2f, -3.4f, -0.1f, -8.8f},
    {0.0f, 0.0f, 2.6f, -0.5f, -0.4f, 0.6f, -0.2f, -1.7f, -1.6f, -3.0f, -2.0f, -2.6f},
    {0.0f, -1.2f, 0.5f, 1.3f, -1.8f, 0.1f, 0.7f, -0.1f, 0.6f, 0.2f, -0.9f, 0.0f, 0.5f}
};

const float GeomagneticField::DELTA_GAUSS_COEFFICIENT_G[13][13] = {
    {0.0f},
    {6.7f, 7.7f},
    {-11.5f, -7.1f, -2.2f},
    {2.8f, -6.2f, 3.4f, -12.2f},
    {-1.1f, -1.6f, -6.0f, 5.4f, -5.5f},
    {-0.3f, 0.6f, -0.7f, 0.1f, 1.2f, 1.0f},
    {-0.6f, -0.4f, 0.5f, 1.4f, -1.4f, 0.0f, 0.8f},
    {-0.1f, -0.3f, -0.1f, 0.7f, 0.2f, -0.5f, -0.8f, 1.0f},
    {-0.1f, 0.1f, -0.1f, 0.5f, -0.1f, 0.4f, 0.5f, 0.0f, 0.4f},
    {-0.1f, -0.2f, 0.0f, 0.4f, -0.3f, 0.0f, 0.3f, 0.0f, 0.0f, -0.4f},
    {0.0f, 0.0f, 0.0f, 0.2f, -0.1f, -0.2f, 0.0f, -0.1f, -0.2f, -0.1f, 0.0f},
    {0.0f, -0.1f, 0.0f, 0.0f, 0.0f, -0.1f, 0.0f, 0.0f, -0.1f, -0.1f, -0.1f, -0.1f},
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -0.1f}
};

const float GeomagneticField::DELTA_GAUSS_COEFFICIENT_H[13][13] = {
    {0.0f},
    {0.0f, -25.1f},
    {0.0f, -30.2f, -23.9f},
    {0.0f, 5.7f, -1.0f, 1.1f},
    {0.0f, 0.2f, 6.9f, 3.7f, -5.6f},
    {0.0f, 0.1f, 2.5f, -0.9f, 3.0f, 0.5f},
    {0.0f, 0.1f, -1.8f, -1.4f, 0.9f, 0.1f, 1.0f},
    {0.0f, 0.5f, 0.6f, -0.7f, -0.2f, -1.2f, 0.2f, 0.3f},
    {0.0f, -0.3f, 0.7f, -0.2f, 0.5f, -0.3f, -0.5f, 0.4f, 0.1f},
    {0.0f, -0.3f, 0.2f, -0.4f, 0.4f, 0.1f, 0.0f, -0.2f, 0.5f, 0.2f},
    {0.0f, 0.0f, 0.1f, -0.3f, 0.1f, -0.2f, 0.1f, 0.0f, -0.1f, 0.2f, 0.0f},
    {0.0f, 0.0f, 0.1f, 0.0f, 0.2f, 0.0f, 0.0f, 0.1f, 0.0f, -0.1f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, -0.1f, 0.1f, 0.0f, 0.0f, 0.0f, 0.1f, 0.0f, 0.0f, 0.0f, -0.1f}
};

const int32_t GeomagneticField::GAUSSIAN_COEFFICIENT_DIMENSION =
    sizeof(GAUSS_COEFFICIENT_G) / sizeof(GAUSS_COEFFICIENT_G[0]);
std::vector<std::vector<float>> GeomagneticField::schmidtQuasiNormalFactors;
std::vector<float> GeomagneticField::relativeRadiusPower(GAUSSIAN_COEFFICIENT_DIMENSION + 2);
std::vector<float> GeomagneticField::sinMLongitude(GAUSSIAN_COEFFICIENT_DIMENSION);
std::vector<float> GeomagneticField::cosMLongitude(GAUSSIAN_COEFFICIENT_DIMENSION);
std::vector<std::vector<float>> GeomagneticField::polynomials(GAUSSIAN_COEFFICIENT_DIMENSION);
std::vector<std::vector<float>> GeomagneticField::polynomialsDerivative(GAUSSIAN_COEFFICIENT_DIMENSION);
float GeomagneticField::geocentricLatitude;
float GeomagneticField::geocentricLongitude;
float GeomagneticField::geocentricRadius;

GeomagneticField::GeomagneticField(float latitude, float longitude, float altitude, int64_t timeMillis)
{
    schmidtQuasiNormalFactors = getSchmidtQuasiNormalFactors(GAUSSIAN_COEFFICIENT_DIMENSION);
    float gcLatitude = fmax(LATITUDE_MIN + PRECISION, fmin(LATITUDE_MAX - PRECISION, latitude));
    calibrateGeocentricCoordinates(gcLatitude, longitude, altitude);
    initLegendreTable(GAUSSIAN_COEFFICIENT_DIMENSION - 1, static_cast<float>(M_PI / 2.0 - geocentricLatitude));
    getRelativeRadiusPower();
    double latDiffRad = toRadians(gcLatitude) - geocentricLatitude;
    calculateGeomagneticComponent(latDiffRad, timeMillis);
}

std::vector<std::vector<float>> GeomagneticField::getSchmidtQuasiNormalFactors(int32_t expansionDegree)
{
    std::vector<std::vector<float>> schmidtQuasiNormFactors(expansionDegree + 1);
    schmidtQuasiNormFactors[0].resize(1);
    schmidtQuasiNormFactors[0][0] = 1.0f;
    for (int32_t rowIndex = 1; rowIndex <= expansionDegree; rowIndex++) {
        schmidtQuasiNormFactors[rowIndex].resize(rowIndex + 1);
        schmidtQuasiNormFactors[rowIndex][0] =
            schmidtQuasiNormFactors[rowIndex - 1][0] * (2 * rowIndex - 1) / static_cast<float>(rowIndex);
        for (int32_t columnIndex = 1; columnIndex <= rowIndex; columnIndex++) {
            schmidtQuasiNormFactors[rowIndex][columnIndex] = schmidtQuasiNormFactors[rowIndex][columnIndex - 1]
                * static_cast<float>(sqrt((rowIndex - columnIndex + 1) * ((columnIndex == 1) ? 2 : 1)
                / static_cast<float>(rowIndex + columnIndex)));
        }
    }
    return schmidtQuasiNormFactors;
}

void GeomagneticField::calculateGeomagneticComponent(double latDiffRad, int64_t timeMillis)
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    float yearsSinceBase = (timeMillis - WMM_BASE_TIME) / (365.0f * 24.0f * 60.0f * 60.0f * 1000.0f);
    float inverseCosLatitude = DERIVATIVE_FACTOR / static_cast<float>(cos(geocentricLatitude));
    getLongitudeTrigonometric();
    float gcX = 0.0f;
    float gcY = 0.0f;
    float gcZ = 0.0f;
    for (int32_t rowIndex = 1; rowIndex < GAUSSIAN_COEFFICIENT_DIMENSION; rowIndex++) {
        for (int32_t columnIndex = 0; columnIndex <= rowIndex; columnIndex++) {
            float g = GAUSS_COEFFICIENT_G[rowIndex][columnIndex] + yearsSinceBase
                * DELTA_GAUSS_COEFFICIENT_G[rowIndex][columnIndex];
            float h = GAUSS_COEFFICIENT_H[rowIndex][columnIndex] + yearsSinceBase
                * DELTA_GAUSS_COEFFICIENT_H[rowIndex][columnIndex];
            gcX += relativeRadiusPower[rowIndex + 2]
                * (g * cosMLongitude[columnIndex] + h * sinMLongitude[columnIndex])
                * polynomialsDerivative[rowIndex][columnIndex]
                * schmidtQuasiNormalFactors[rowIndex][columnIndex];
            gcY += relativeRadiusPower[rowIndex + 2] * columnIndex
                * (g * sinMLongitude[columnIndex] - h * cosMLongitude[columnIndex])
                * polynomials[rowIndex][columnIndex]
                * schmidtQuasiNormalFactors[rowIndex][columnIndex]
                * inverseCosLatitude;
            gcZ -= (rowIndex + 1) * relativeRadiusPower[rowIndex + 2]
                * (g * cosMLongitude[columnIndex] + h * sinMLongitude[columnIndex])
                * polynomials[rowIndex][columnIndex]
                * schmidtQuasiNormalFactors[rowIndex][columnIndex];
        }
        northComponent = static_cast<float>(gcX * cos(latDiffRad) + gcZ * sin(latDiffRad));
        eastComponent = gcY;
        downComponent = static_cast<float>(-gcX * sin(latDiffRad) + gcZ * cos(latDiffRad));
    }
}

void GeomagneticField::getLongitudeTrigonometric()
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    sinMLongitude[0] = 0.0f;
    cosMLongitude[0] = 1.0f;
    sinMLongitude[1] = static_cast<float>(sin(geocentricLongitude));
    cosMLongitude[1] = static_cast<float>(cos(geocentricLongitude));
    for (uint32_t index = 2; index < GAUSSIAN_COEFFICIENT_DIMENSION; ++index) {
        int32_t x = index >> 1;
        sinMLongitude[index] = (sinMLongitude[index - x] * cosMLongitude[x]
            + cosMLongitude[index - x] * sinMLongitude[x]);
        cosMLongitude[index] = (cosMLongitude[index - x] * cosMLongitude[x]
            - sinMLongitude[index - x] * sinMLongitude[x]);
    }
}

void GeomagneticField::getRelativeRadiusPower()
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    relativeRadiusPower[0] = 1.0f;
    relativeRadiusPower[1] = EARTH_REFERENCE_RADIUS / geocentricRadius;
    for (int32_t index = 2; index < relativeRadiusPower.size(); ++index) {
        relativeRadiusPower[index] = relativeRadiusPower[index - 1] * relativeRadiusPower[1];
    }
}

void GeomagneticField::calibrateGeocentricCoordinates(float latitude, float longitude, float altitude)
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    float altitudeKm = altitude / CONVERSION_FACTOR;
    float a2 = EARTH_MAJOR_AXIS_RADIUS * EARTH_MAJOR_AXIS_RADIUS;
    float b2 = EARTH_MINOR_AXIS_RADIUS * EARTH_MINOR_AXIS_RADIUS;
    double gdLatRad = toRadians(latitude);
    float clat = static_cast<float>(cos(gdLatRad));
    float slat = static_cast<float>(sin(gdLatRad));
    float tlat = slat / clat;
    float latRad = static_cast<float>(sqrt(a2 * clat * clat + b2 * slat * slat));
    geocentricLatitude = static_cast<float>(atan(tlat * (latRad * altitudeKm + b2)
        / (latRad * altitudeKm + a2)));
    geocentricLongitude = static_cast<float>(toRadians(longitude));

    float radSq = altitudeKm * altitudeKm + 2 * altitudeKm
        * latRad + (a2 * a2 * clat * clat + b2 * b2 * slat * slat)
        / (a2 * clat * clat + b2 * slat * slat);
    geocentricRadius = static_cast<float>(sqrt(radSq));
}

void GeomagneticField::initLegendreTable(int32_t expansionDegree, float thetaRad)
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    polynomials[0].resize(1);
    polynomials[0][0] = 1.0f;
    polynomialsDerivative[0].resize(1);
    polynomialsDerivative[0][0] = 0.0f;
    float cosValue = static_cast<float>(cos(thetaRad));
    float sinValue = static_cast<float>(sin(thetaRad));
    for (int32_t rowIndex = 1; rowIndex <= expansionDegree; rowIndex++) {
        polynomials[rowIndex].resize(rowIndex + 1);
        polynomialsDerivative[rowIndex].resize(rowIndex + 1);
        for (int32_t columnIndex = 0; columnIndex <= rowIndex; columnIndex++) {
            if (rowIndex == columnIndex) {
                polynomials[rowIndex][columnIndex] = sinValue * polynomials[rowIndex - 1][columnIndex - 1];
                polynomialsDerivative[rowIndex][columnIndex] = cosValue * polynomials[rowIndex - 1][columnIndex - 1]
                    + sinValue * polynomialsDerivative[rowIndex - 1][columnIndex - 1];
            } else if (rowIndex == 1 || columnIndex == rowIndex - 1) {
                polynomials[rowIndex][columnIndex] = cosValue * polynomials[rowIndex - 1][columnIndex];
                polynomialsDerivative[rowIndex][columnIndex] = -sinValue * polynomials[rowIndex - 1][columnIndex]
                    + cosValue * polynomialsDerivative[rowIndex - 1][columnIndex];
            } else {
                float k = ((rowIndex - 1) * (rowIndex - 1) - columnIndex * columnIndex)
                    / static_cast<float>((2 * rowIndex - 1) * (2 * rowIndex - 3));
                polynomials[rowIndex][columnIndex] = cosValue * polynomials[rowIndex - 1][columnIndex]
                    - k * polynomials[rowIndex - 2][columnIndex];
                polynomialsDerivative[rowIndex][columnIndex] = -sinValue * polynomials[rowIndex - 1][columnIndex]
                    + cosValue * polynomialsDerivative[rowIndex - 1][columnIndex]
                    - k * polynomialsDerivative[rowIndex - 2][columnIndex];
            }
        }
    }
}

float GeomagneticField::obtainX()
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    return northComponent;
}

float GeomagneticField::obtainY()
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    return eastComponent;
}

float GeomagneticField::obtainZ()
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    return downComponent;
}

float GeomagneticField::obtainGeomagneticDip()
{
    return static_cast<float>(toDegrees(atan2(downComponent, obtainLevelIntensity())));
}

double GeomagneticField::toDegrees(double angrad)
{
    return angrad * 180.0 / M_PI;
}

double GeomagneticField::toRadians(double angdeg)
{
    return angdeg / 180.0 * M_PI;
}

float GeomagneticField::obtainDeflectionAngle()
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    return static_cast<float>(toDegrees(atan2(eastComponent, northComponent)));
}

float GeomagneticField::obtainLevelIntensity()
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    float horizontalIntensity = hypot(northComponent, eastComponent);
    return horizontalIntensity;
}

float GeomagneticField::obtainTotalIntensity()
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    float sumOfSquares = northComponent * northComponent + eastComponent * eastComponent
        + downComponent * downComponent;
    float totalIntensity = static_cast<float>(sqrt(sumOfSquares));
    return totalIntensity;
}
