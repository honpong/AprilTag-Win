#include "calibration_json.h"
#include "json_keys.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>

using namespace rapidjson;
using namespace std;

static void CopyJsonToRSStruct(Document &json, rcCalibration &cal, const rcCalibration *defaults)
{
    if (json.HasMember(KEY_DEVICE_NAME))
    {
        std::string deviceName = json[KEY_DEVICE_NAME].GetString();
        snprintf(cal.deviceName, sizeof(cal.deviceName), "%s", deviceName.c_str());
    }
    else
    {
        snprintf(cal.deviceName, sizeof(cal.deviceName), "%s", defaults->deviceName);
    }

    cal.calibrationVersion = json.HasMember(KEY_CALIBRATION_VERSION) ? json[KEY_CALIBRATION_VERSION].GetInt() : defaults->calibrationVersion;
    cal.Fx = json.HasMember(KEY_FX) ? (float)json[KEY_FX].GetDouble() : defaults->Fx;
    cal.Fy = json.HasMember(KEY_FY) ? (float)json[KEY_FY].GetDouble() : defaults->Fy;
    cal.Cx = json.HasMember(KEY_CX) ? (float)json[KEY_CX].GetDouble() : defaults->Cx;
    cal.Cy = json.HasMember(KEY_CY) ? (float)json[KEY_CY].GetDouble() : defaults->Cy;
    cal.px = json.HasMember(KEY_PX) ? (float)json[KEY_PX].GetDouble() : defaults->px;
    cal.py = json.HasMember(KEY_PY) ? (float)json[KEY_PY].GetDouble() : defaults->py;
    cal.K0 = json.HasMember(KEY_K0) ? (float)json[KEY_K0].GetDouble() : defaults->K0;
    cal.K1 = json.HasMember(KEY_K1) ? (float)json[KEY_K1].GetDouble() : defaults->K1;
    cal.K2 = json.HasMember(KEY_K2) ? (float)json[KEY_K2].GetDouble() : defaults->K2;
    cal.Kw = json.HasMember(KEY_KW) ? (float)json[KEY_KW].GetDouble() : defaults->Kw;
    cal.a_bias[0] = json.HasMember(KEY_ABIAS0) ? (float)json[KEY_ABIAS0].GetDouble() : defaults->a_bias[0];
    cal.a_bias[1] = json.HasMember(KEY_ABIAS1) ? (float)json[KEY_ABIAS1].GetDouble() : defaults->a_bias[1];
    cal.a_bias[2] = json.HasMember(KEY_ABIAS2) ? (float)json[KEY_ABIAS2].GetDouble() : defaults->a_bias[2];
    cal.w_bias[0] = json.HasMember(KEY_WBIAS0) ? (float)json[KEY_WBIAS0].GetDouble() : defaults->w_bias[0];
    cal.w_bias[1] = json.HasMember(KEY_WBIAS1) ? (float)json[KEY_WBIAS1].GetDouble() : defaults->w_bias[1];
    cal.w_bias[2] = json.HasMember(KEY_WBIAS2) ? (float)json[KEY_WBIAS2].GetDouble() : defaults->w_bias[2];
    cal.Tc[0] = json.HasMember(KEY_TC0) ? (float)json[KEY_TC0].GetDouble() : defaults->Tc[0];
    cal.Tc[1] = json.HasMember(KEY_TC1) ? (float)json[KEY_TC1].GetDouble() : defaults->Tc[1];
    cal.Tc[2] = json.HasMember(KEY_TC2) ? (float)json[KEY_TC2].GetDouble() : defaults->Tc[2];
    cal.Wc[0] = json.HasMember(KEY_WC0) ? (float)json[KEY_WC0].GetDouble() : defaults->Wc[0];
    cal.Wc[1] = json.HasMember(KEY_WC1) ? (float)json[KEY_WC1].GetDouble() : defaults->Wc[1];
    cal.Wc[2] = json.HasMember(KEY_WC2) ? (float)json[KEY_WC2].GetDouble() : defaults->Wc[2];
    cal.a_bias_var[0] = json.HasMember(KEY_ABIASVAR0) ? (float)json[KEY_ABIASVAR0].GetDouble() : defaults->a_bias_var[0];
    cal.a_bias_var[1] = json.HasMember(KEY_ABIASVAR1) ? (float)json[KEY_ABIASVAR1].GetDouble() : defaults->a_bias_var[1];
    cal.a_bias_var[2] = json.HasMember(KEY_ABIASVAR2) ? (float)json[KEY_ABIASVAR2].GetDouble() : defaults->a_bias_var[2];
    cal.w_bias_var[0] = json.HasMember(KEY_WBIASVAR0) ? (float)json[KEY_WBIASVAR0].GetDouble() : defaults->w_bias_var[0];
    cal.w_bias_var[1] = json.HasMember(KEY_WBIASVAR1) ? (float)json[KEY_WBIASVAR1].GetDouble() : defaults->w_bias_var[1];
    cal.w_bias_var[2] = json.HasMember(KEY_WBIASVAR2) ? (float)json[KEY_WBIASVAR2].GetDouble() : defaults->w_bias_var[2];
    cal.Tc_var[0] = json.HasMember(KEY_TCVAR0) ? (float)json[KEY_TCVAR0].GetDouble() : defaults->Tc_var[0];
    cal.Tc_var[1] = json.HasMember(KEY_TCVAR1) ? (float)json[KEY_TCVAR1].GetDouble() : defaults->Tc_var[1];
    cal.Tc_var[2] = json.HasMember(KEY_TCVAR2) ? (float)json[KEY_TCVAR2].GetDouble() : defaults->Tc_var[2];
    cal.Wc_var[0] = json.HasMember(KEY_WCVAR0) ? (float)json[KEY_WCVAR0].GetDouble() : defaults->Wc_var[0];
    cal.Wc_var[1] = json.HasMember(KEY_WCVAR1) ? (float)json[KEY_WCVAR1].GetDouble() : defaults->Wc_var[1];
    cal.Wc_var[2] = json.HasMember(KEY_WCVAR2) ? (float)json[KEY_WCVAR2].GetDouble() : defaults->Wc_var[2];
    cal.w_meas_var = json.HasMember(KEY_WMEASVAR) ? (float)json[KEY_WMEASVAR].GetDouble() : defaults->w_meas_var;
    cal.a_meas_var = json.HasMember(KEY_AMEASVAR) ? (float)json[KEY_AMEASVAR].GetDouble() : defaults->a_meas_var;
    cal.image_width = json.HasMember(KEY_IMAGE_WIDTH) ? json[KEY_IMAGE_WIDTH].GetInt() : defaults->image_width;
    cal.image_height = json.HasMember(KEY_IMAGE_HEIGHT) ? json[KEY_IMAGE_HEIGHT].GetInt() : defaults->image_height;
    cal.distortionModel = json.HasMember(KEY_DISTORTION_MODEL) ? json[KEY_DISTORTION_MODEL].GetInt() : defaults->distortionModel;
    cal.shutterDelay = json.HasMember(KEY_SHUTTER_DELAY) ? (float)json[KEY_SHUTTER_DELAY].GetDouble() : defaults->shutterDelay;
    cal.shutterPeriod = json.HasMember(KEY_SHUTTER_PERIOD) ? (float)json[KEY_SHUTTER_PERIOD].GetDouble() : defaults->shutterPeriod;
    cal.timeStampOffset = json.HasMember(KEY_TIMESTAMP_OFFSET) ? (float)json[KEY_TIMESTAMP_OFFSET].GetDouble() : defaults->timeStampOffset;

    int i = 0;
    SizeType destArrSize = SizeType(sizeof(cal.accelerometerTransform) / sizeof(cal.accelerometerTransform[0]));
    if (json.HasMember(KEY_ACCEL_TRANSFORM) && json[KEY_ACCEL_TRANSFORM].IsArray())
    {
        SizeType arrSize = json[KEY_ACCEL_TRANSFORM].Size() > destArrSize ? destArrSize : json[KEY_ACCEL_TRANSFORM].Size();
        for (; i < arrSize; i++)
        {
            cal.accelerometerTransform[i] = (float)json[KEY_ACCEL_TRANSFORM][i].GetDouble();
        }
    }
    // set defaults for any remaining array members
    for (; i < destArrSize; i++)
    {
        cal.accelerometerTransform[i] = defaults->accelerometerTransform[i]; // we can be certain that these arrays are the same size
    }

    i = 0;
    destArrSize = SizeType(sizeof(cal.gyroscopeTransform) / sizeof(cal.gyroscopeTransform[0]));
    if (json.HasMember(KEY_GYRO_TRANSFORM) && json[KEY_GYRO_TRANSFORM].IsArray())
    {
        SizeType arrSize = json[KEY_GYRO_TRANSFORM].Size() > destArrSize ? destArrSize : json[KEY_GYRO_TRANSFORM].Size();
        for (; i < arrSize; i++)
        {
            cal.gyroscopeTransform[i] = (float)json[KEY_GYRO_TRANSFORM][i].GetDouble();
        }
    }
    // set defaults for any remaining array members
    for (; i < destArrSize; i++)
    {
        cal.gyroscopeTransform[i] = defaults->gyroscopeTransform[i]; // we can be certain that these arrays are the same size
    }
}

static void CopyRSStructToJson(const rcCalibration &cal, Document &json)
{
    Value name(kStringType);
    name.SetString(cal.deviceName, json.GetAllocator());
    json.AddMember(KEY_DEVICE_NAME, name, json.GetAllocator());
    json.AddMember(KEY_CALIBRATION_VERSION, cal.calibrationVersion, json.GetAllocator());
    json.AddMember(KEY_FX, cal.Fx, json.GetAllocator());
    json.AddMember(KEY_FY, cal.Fy, json.GetAllocator());
    json.AddMember(KEY_CX, cal.Cx, json.GetAllocator());
    json.AddMember(KEY_CY, cal.Cy, json.GetAllocator());
    json.AddMember(KEY_PX, cal.px, json.GetAllocator());
    json.AddMember(KEY_PY, cal.py, json.GetAllocator());
    json.AddMember(KEY_K0, cal.K0, json.GetAllocator());
    json.AddMember(KEY_K1, cal.K1, json.GetAllocator());
    json.AddMember(KEY_K2, cal.K2, json.GetAllocator());
    json.AddMember(KEY_KW, cal.Kw, json.GetAllocator());
    json.AddMember(KEY_ABIAS0, cal.a_bias[0], json.GetAllocator());
    json.AddMember(KEY_ABIAS1, cal.a_bias[1], json.GetAllocator());
    json.AddMember(KEY_ABIAS2, cal.a_bias[2], json.GetAllocator());
    json.AddMember(KEY_WBIAS0, cal.w_bias[0], json.GetAllocator());
    json.AddMember(KEY_WBIAS1, cal.w_bias[1], json.GetAllocator());
    json.AddMember(KEY_WBIAS2, cal.w_bias[2], json.GetAllocator());
    json.AddMember(KEY_TC0, cal.Tc[0], json.GetAllocator());
    json.AddMember(KEY_TC1, cal.Tc[1], json.GetAllocator());
    json.AddMember(KEY_TC2, cal.Tc[2], json.GetAllocator());
    json.AddMember(KEY_WC0, cal.Wc[0], json.GetAllocator());
    json.AddMember(KEY_WC1, cal.Wc[1], json.GetAllocator());
    json.AddMember(KEY_WC2, cal.Wc[2], json.GetAllocator());
    json.AddMember(KEY_ABIASVAR0, cal.a_bias_var[0], json.GetAllocator());
    json.AddMember(KEY_ABIASVAR1, cal.a_bias_var[1], json.GetAllocator());
    json.AddMember(KEY_ABIASVAR2, cal.a_bias_var[2], json.GetAllocator());
    json.AddMember(KEY_WBIASVAR0, cal.w_bias_var[0], json.GetAllocator());
    json.AddMember(KEY_WBIASVAR1, cal.w_bias_var[1], json.GetAllocator());
    json.AddMember(KEY_WBIASVAR2, cal.w_bias_var[2], json.GetAllocator());
    json.AddMember(KEY_TCVAR0, cal.Tc_var[0], json.GetAllocator());
    json.AddMember(KEY_TCVAR1, cal.Tc_var[1], json.GetAllocator());
    json.AddMember(KEY_TCVAR2, cal.Tc_var[2], json.GetAllocator());
    json.AddMember(KEY_WCVAR0, cal.Wc_var[0], json.GetAllocator());
    json.AddMember(KEY_WCVAR1, cal.Wc_var[1], json.GetAllocator());
    json.AddMember(KEY_WCVAR2, cal.Wc_var[2], json.GetAllocator());
    json.AddMember(KEY_WMEASVAR, cal.w_meas_var, json.GetAllocator());
    json.AddMember(KEY_AMEASVAR, cal.a_meas_var, json.GetAllocator());
    json.AddMember(KEY_IMAGE_WIDTH, cal.image_width, json.GetAllocator());
    json.AddMember(KEY_IMAGE_HEIGHT, cal.image_height, json.GetAllocator());
    json.AddMember(KEY_DISTORTION_MODEL, cal.distortionModel, json.GetAllocator());
    json.AddMember(KEY_SHUTTER_DELAY, cal.shutterDelay, json.GetAllocator());
    json.AddMember(KEY_SHUTTER_PERIOD, cal.shutterPeriod, json.GetAllocator());
    json.AddMember(KEY_TIMESTAMP_OFFSET, cal.timeStampOffset, json.GetAllocator());
    
    int arrSize = sizeof(cal.accelerometerTransform) / sizeof(cal.accelerometerTransform[0]);
    Value accelArray(kArrayType);
    for (int i = 0; i < arrSize; i++)
    {
        accelArray.PushBack(cal.accelerometerTransform[i], json.GetAllocator());
    }
    json.AddMember(KEY_ACCEL_TRANSFORM, accelArray, json.GetAllocator());

    arrSize = sizeof(cal.gyroscopeTransform) / sizeof(cal.gyroscopeTransform[0]);
    Value gyroArray(kArrayType);
    for (int i = 0; i < arrSize; i++)
    {
        gyroArray.PushBack(cal.gyroscopeTransform[i], json.GetAllocator());
    }
    json.AddMember(KEY_GYRO_TRANSFORM, gyroArray, json.GetAllocator());
}

bool calibration_serialize(const rcCalibration &cal, std::string &jsonString)
{
    Document json;
    json.SetObject();
    CopyRSStructToJson(cal, json);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    json.Accept(writer);
    jsonString = buffer.GetString();
    return jsonString.length() > 0;
}

bool calibration_deserialize(const std::string &jsonString, rcCalibration &cal, const rcCalibration *defaults)
{
    Document json;
    if (json.Parse(jsonString.c_str()).HasParseError())
        return false;
    CopyJsonToRSStruct(json, cal, defaults);
    return true;
}
