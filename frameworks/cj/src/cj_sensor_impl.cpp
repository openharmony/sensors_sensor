/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "cj_sensor_impl.h"

#include "cj_lambda.h"
#include "geomagnetic_field.h"
#include "sensor_agent.h"
#include "sensor_algorithm.h"
#include "sensor_errors.h"

namespace OHOS {
namespace Sensors {
namespace {
constexpr int32_t ROTATION_VECTOR_LENGTH = 3;
constexpr int32_t QUATERNION_LENGTH = 4;
constexpr int32_t THREE_DIMENSIONAL_MATRIX_LENGTH = 9;
constexpr int32_t DATA_LENGTH = 16;
} // namespace

CJSensorImpl::CJSensorImpl() {}
CJSensorImpl::~CJSensorImpl() {}

void CJSensorImpl::CJDataCallbackImpl(SensorEvent *event)
{
    CHKPV(event);
    CJ_SENSOR_IMPL->EmitCallBack(event);
}

int32_t CJSensorImpl::SubscribeSensorImpl(int32_t sensorId, int64_t interval)
{
    CALL_LOG_ENTER;
    int32_t ret = SubscribeSensor(sensorId, &cjUser_);
    if (ret != ERR_OK) {
        SEN_HILOGE("SubscribeSensor failed");
        return ret;
    }
    ret = SetBatch(sensorId, &cjUser_, interval, 0);
    if (ret != ERR_OK) {
        SEN_HILOGE("SetBatch failed");
        return ret;
    }

    return ActivateSensor(sensorId, &cjUser_);
}

int32_t CJSensorImpl::UnsubscribeSensorImpl(int32_t sensorTypeId)
{
    CALL_LOG_ENTER;
    int32_t ret = DeactivateSensor(sensorTypeId, &cjUser_);
    if (ret != ERR_OK) {
        SEN_HILOGE("DeactivateSensor failed");
        return ret;
    }
    return UnsubscribeSensor(sensorTypeId, &cjUser_);
}

char *CJSensorImpl::MallocCString(const std::string origin)
{
    if (origin.empty()) {
        SEN_HILOGD("String is empty.");
        return nullptr;
    }
    auto len = origin.length() + 1;
    char *res = static_cast<char *>(malloc(sizeof(char) * len));
    if (res == nullptr) {
        SEN_HILOGE("Malloc failed.");
        return nullptr;
    }
    return std::char_traits<char>::copy(res, origin.c_str(), len);
}

void CJSensorImpl::Transform2CSensor(const SensorInfo &in, CSensor &out)
{
    out.sensorName = MallocCString(in.sensorName);
    out.vendorName = MallocCString(in.vendorName);
    out.firmwareVersion = MallocCString(in.firmwareVersion);
    out.hardwareVersion = MallocCString(in.hardwareVersion);

    out.sensorTypeId = in.sensorTypeId;
    out.maxRange = in.maxRange;
    out.minSamplePeriod = in.minSamplePeriod;
    out.maxSamplePeriod = in.maxSamplePeriod;
    out.precision = in.precision;
    out.power = in.power;
}

std::vector<float> CJSensorImpl::ConvertCArr2Vector(const CArrFloat32 &in)
{
    std::vector<float> res;
    if (in.head == nullptr || in.size <= 0) {
        SEN_HILOGD("head is nullptr or size is zero.");
        return res;
    }

    for (int64_t i = 0; i < in.size; i++) {
        res.push_back(in.head[i]);
    }

    return res;
}

CArrFloat32 CJSensorImpl::ConvertVector2CArr(const std::vector<float> &in)
{
    CArrFloat32 res = {NULL, 0};
    if (in.empty()) {
        SEN_HILOGD("vector is empty.");
        return res;
    }

    res.head = static_cast<float *>(malloc(sizeof(float) * in.size()));
    if (res.head == nullptr) {
        SEN_HILOGE("Malloc failed.");
        return res;
    }
    size_t i = 0;
    for (; i < in.size(); ++i) {
        res.head[i] = in[i];
    }
    res.size = static_cast<int64_t>(i);
    return res;
}

int32_t CJSensorImpl::OnSensorChange(int32_t sensorId, int64_t interval, void (*callback)(SensorEvent *event))
{
    CALL_LOG_ENTER;
    int32_t ret = SubscribeSensorImpl(sensorId, interval);
    if (ret != ERR_OK) {
        SEN_HILOGE("subscribe sensor failed, %{public}d.", sensorId);
        return ret;
    }

    AddCallback2Map(sensorId, CJLambda::Create(callback));
    return ERR_OK;
}

int32_t CJSensorImpl::OffSensorChange(int32_t sensorId)
{
    CALL_LOG_ENTER;
    int32_t ret = UnsubscribeSensorImpl(sensorId);
    if (ret != ERR_OK) {
        SEN_HILOGE("unsubscribe sensor failed, %{public}d.", sensorId);
        return ret;
    }

    DelCallback(sensorId);
    return ERR_OK;
}

void CJSensorImpl::EmitCallBack(SensorEvent *event)
{
    auto callback = FindCallback(event->sensorTypeId);
    if (callback == std::nullopt) {
        SEN_HILOGE("EmitCallBack failed, %{public}d not find.", event->sensorTypeId);
        return;
    }

    callback.value()(event);
}

void CJSensorImpl::AddCallback2Map(int32_t type, SensorCallbackType callback)
{
    std::lock_guard<std::mutex> mutex(mutex_);
    eventMap_[type] = callback;
}

void CJSensorImpl::DelCallback(int32_t type)
{
    std::lock_guard<std::mutex> mutex(mutex_);
    eventMap_.erase(type);
}

std::optional<SensorCallbackType> CJSensorImpl::FindCallback(int32_t type)
{
    std::lock_guard<std::mutex> mutex(mutex_);
    auto iter = eventMap_.find(type);
    if (iter != eventMap_.end()) {
        return iter->second;
    }

    return std::nullopt;
}

CGeomagneticData CJSensorImpl::GetGeomagneticInfo(CLocationOptions location, int64_t timeMillis)
{
    GeomagneticField geomagneticField(location.latitude, location.longitude, location.altitude, timeMillis);
    CGeomagneticData res = {
        .x = geomagneticField.ObtainX(),
        .y = geomagneticField.ObtainY(),
        .z = geomagneticField.ObtainZ(),
        .geomagneticDip = geomagneticField.ObtainGeomagneticDip(),
        .deflectionAngle = geomagneticField.ObtainDeflectionAngle(),
        .levelIntensity = geomagneticField.ObtainLevelIntensity(),
        .totalIntensity = geomagneticField.ObtainTotalIntensity(),
    };
    return res;
}

int32_t CJSensorImpl::GetAltitude(float seaPressure, float currentPressure, float *altitude)
{
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.GetAltitude(seaPressure, currentPressure, altitude);
    if (ret != ERR_OK) {
        SEN_HILOGE("Get altitude failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t CJSensorImpl::GetGeomagneticDip(CArrFloat32 inclinationMatrix, float *geomagneticDip)
{
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.GetGeomagneticDip(ConvertCArr2Vector(inclinationMatrix), geomagneticDip);
    if (ret != ERR_OK) {
        SEN_HILOGE("Get geomagnetic dip failed, ret:%{public}d", ret);
    }

    return ret;
}

int32_t CJSensorImpl::GetAngleModify(const CArrFloat32 &curRotationMatrix, const CArrFloat32 &preRotationMatrix,
                                     CArrFloat32 *angleChangeOut)
{
    if (angleChangeOut == nullptr) {
        SEN_HILOGE("Invalid parameter, angleChangeOut is nullptr!");
        return PARAMETER_ERROR;
    }
    std::vector<float> angleChange(ROTATION_VECTOR_LENGTH);
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.GetAngleModify(ConvertCArr2Vector(curRotationMatrix),
                                                 ConvertCArr2Vector(preRotationMatrix), angleChange);
    if (ret != ERR_OK) {
        SEN_HILOGE("Get angle change failed, ret:%{public}d", ret);
        return ret;
    }

    *angleChangeOut = ConvertVector2CArr(angleChange);
    return ret;
}

int32_t CJSensorImpl::GetRotationMatrix(const CArrFloat32 &rotationCArr, CArrFloat32 &rotation)
{
    std::vector<float> rotationMatrix(THREE_DIMENSIONAL_MATRIX_LENGTH);
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.CreateRotationMatrix(ConvertCArr2Vector(rotationCArr), rotationMatrix);
    if (ret != ERR_OK) {
        SEN_HILOGE("Get rotation matrix failed, ret:%{public}d", ret);
        return ret;
    }

    rotation = ConvertVector2CArr(rotationMatrix);
    return ret;
}

int32_t CJSensorImpl::TransformRotationMatrix(const CArrFloat32 &rotationCArr, int32_t axisX, int32_t axisY,
                                              CArrFloat32 &outRotationMatrix)
{
    int64_t length = rotationCArr.size;
    if ((length != DATA_LENGTH) && (length != THREE_DIMENSIONAL_MATRIX_LENGTH)) {
        SEN_HILOGE("Transform rotation mastrix failed, invalid parameter.");
        return PARAMETER_ERROR;
    }
    std::vector<float> outRotationVector(length);
    SensorAlgorithm sensorAlgorithm;
    int32_t ret =
        sensorAlgorithm.TransformCoordinateSystem(ConvertCArr2Vector(rotationCArr), axisX, axisY, outRotationVector);
    if (ret != ERR_OK) {
        SEN_HILOGE("Transform coordinate system failed, ret:%{public}d", ret);
        return ret;
    }

    outRotationMatrix = ConvertVector2CArr(outRotationVector);
    return ret;
}

int32_t CJSensorImpl::GetQuaternion(const CArrFloat32 &rotationVector, CArrFloat32 &quaternionOut)
{
    std::vector<float> quaternion(QUATERNION_LENGTH);
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.CreateQuaternion(ConvertCArr2Vector(rotationVector), quaternion);
    if (ret != ERR_OK) {
        SEN_HILOGE("Get quaternion failed, ret:%{public}d", ret);
        return ret;
    }

    quaternionOut = ConvertVector2CArr(quaternion);
    return ret;
}

int32_t CJSensorImpl::GetOrientation(const CArrFloat32 &rotationMatrix, CArrFloat32 &rotationAngleOut)
{
    std::vector<float> rotationAngle(ROTATION_VECTOR_LENGTH);
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.GetDirection(ConvertCArr2Vector(rotationMatrix), rotationAngle);
    if (ret != OHOS::ERR_OK) {
        SEN_HILOGE("Get direction failed, ret:%{public}d", ret);
        return ret;
    }

    rotationAngleOut = ConvertVector2CArr(rotationAngle);
    return ret;
}

int32_t CJSensorImpl::GetRotationMatrixByGraityAndGeomagnetic(const CArrFloat32 gravity, const CArrFloat32 geomagnetic,
                                                              CArrFloat32 &rotationMatrix,
                                                              CArrFloat32 &inclinationMatrix)
{
    std::vector<float> rotation(THREE_DIMENSIONAL_MATRIX_LENGTH);
    std::vector<float> inclination(THREE_DIMENSIONAL_MATRIX_LENGTH);
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.CreateRotationAndInclination(ConvertCArr2Vector(gravity),
                                                               ConvertCArr2Vector(geomagnetic), rotation, inclination);
    if (ret != OHOS::ERR_OK) {
        SEN_HILOGE("Create rotation and inclination failed, ret:%{public}d", ret);
        return ret;
    }

    rotationMatrix = ConvertVector2CArr(rotation);
    inclinationMatrix = ConvertVector2CArr(inclination);

    return ret;
}

int32_t CJSensorImpl::GetAllSensorList(CSensorArray &sensorList)
{
    SensorInfo *sensorInfos = nullptr;
    int32_t count = 0;
    int32_t ret = GetAllSensors(&sensorInfos, &count);
    if (ret != OHOS::ERR_OK) {
        SEN_HILOGE("Get all sensors failed, ret:%{public}d", ret);
        return ret;
    }

    if (count == 0) {
        return ERR_OK;
    }

    sensorList.head = static_cast<CSensor *>(malloc(sizeof(CSensor) * count));
    if (sensorList.head == nullptr) {
        SEN_HILOGE("Malloc failed.");
        return ERR_OK;
    }
    int32_t i = 0;
    for (; i < count; ++i) {
        if ((sensorInfos[i].sensorTypeId == SENSOR_TYPE_ID_AMBIENT_LIGHT1) ||
            (sensorInfos[i].sensorTypeId == SENSOR_TYPE_ID_PROXIMITY1) ||
            (sensorInfos[i].sensorTypeId > GL_SENSOR_TYPE_PRIVATE_MIN_VALUE)) {
            SEN_HILOGD("This sensor is secondary ambient light");
            continue;
        }
        Transform2CSensor(sensorInfos[i], sensorList.head[i]);
    }
    sensorList.size = i;

    return ERR_OK;
}
} // namespace Sensors
} // namespace OHOS