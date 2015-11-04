#include "rs_calibration_json.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>

using namespace rapidjson;
using namespace std;

#define KEY_FX "Fx"
#define KEY_FY "Fy"
#define KEY_CX "Cx"
#define KEY_CY "Cy"
#define KEY_PX "px"
#define KEY_PY "py"
#define KEY_K0 "K0"
#define KEY_K1 "K1"
#define KEY_K2 "K2"
#define KEY_KW "Kw"
#define KEY_ABIAS0 "abias0"
#define KEY_ABIAS1 "abias1"
#define KEY_ABIAS2 "abias2"
#define KEY_WBIAS0 "wbias0"
#define KEY_WBIAS1 "wbias1"
#define KEY_WBIAS2 "wbias2"
#define KEY_TC0 "Tc0"
#define KEY_TC1 "Tc1"
#define KEY_TC2 "Tc2"
#define KEY_WC0 "Wc0"
#define KEY_WC1 "Wc1"
#define KEY_WC2 "Wc2"
#define KEY_ABIASVAR0 "abiasvar0"
#define KEY_ABIASVAR1 "abiasvar1"
#define KEY_ABIASVAR2 "abiasvar2"
#define KEY_WBIASVAR0 "wbiasvar0"
#define KEY_WBIASVAR1 "wbiasvar1"
#define KEY_WBIASVAR2 "wbiasvar2"
#define KEY_TCVAR0 "TcVar0"
#define KEY_TCVAR1 "TcVar1"
#define KEY_TCVAR2 "TcVar2"
#define KEY_WCVAR0 "WcVar0"
#define KEY_WCVAR1 "WcVar1"
#define KEY_WCVAR2 "WcVar2"
#define KEY_WMEASVAR "wMeasVar"
#define KEY_AMEASVAR "aMeasVar"
#define KEY_IMAGE_WIDTH "imageWidth"
#define KEY_IMAGE_HEIGHT "imageHeight"
#define KEY_CALIBRATION_VERSION "calibrationVersion"
#define KEY_DISTORTION_MODEL "distortionModel"
#define KEY_DEVICE_NAME "device"
#define KEY_SHUTTER_DELAY "shutterDelay"
#define KEY_SHUTTER_PERIOD "shutterPeriod"
#define KEY_TIMESTAMP_OFFSET "timeStampOffset"
#define KEY_ACCEL_TRANSFORM "accelerometerTransform"
#define KEY_GYRO_TRANSFORM "gyroscopeTransform"

void CopyJsonToRSStruct(Document &json, rcCalibration &cal)
{
    if (json.HasMember(KEY_DEVICE_NAME))
    {
        std::string deviceName = json[KEY_DEVICE_NAME].GetString();
        snprintf(cal.deviceName, sizeof(cal.deviceName), "%s", deviceName.c_str());
    }

    if (json.HasMember(KEY_CALIBRATION_VERSION)) cal.calibrationVersion = json[KEY_CALIBRATION_VERSION].GetInt();
    if (json.HasMember(KEY_FX)) cal.Fx = (float)json[KEY_FX].GetDouble();
    if (json.HasMember(KEY_FY)) cal.Fy = (float)json[KEY_FY].GetDouble();
    if (json.HasMember(KEY_CX)) cal.Cx = (float)json[KEY_CX].GetDouble();
    if (json.HasMember(KEY_CY)) cal.Cy = (float)json[KEY_CY].GetDouble();
    if (json.HasMember(KEY_PX)) cal.px = (float)json[KEY_PX].GetDouble();
    if (json.HasMember(KEY_PY)) cal.py = (float)json[KEY_PY].GetDouble();
    if (json.HasMember(KEY_K0)) cal.K0 = (float)json[KEY_K0].GetDouble();
    if (json.HasMember(KEY_K1)) cal.K1 = (float)json[KEY_K1].GetDouble();
    if (json.HasMember(KEY_K2)) cal.K2 = (float)json[KEY_K2].GetDouble();
    if (json.HasMember(KEY_KW)) cal.Kw = (float)json[KEY_KW].GetDouble();
    if (json.HasMember(KEY_ABIAS0)) cal.a_bias[0] = (float)json[KEY_ABIAS0].GetDouble();
    if (json.HasMember(KEY_ABIAS1)) cal.a_bias[1] = (float)json[KEY_ABIAS1].GetDouble();
    if (json.HasMember(KEY_ABIAS2)) cal.a_bias[2] = (float)json[KEY_ABIAS2].GetDouble();
    if (json.HasMember(KEY_WBIAS0)) cal.w_bias[0] = (float)json[KEY_WBIAS0].GetDouble();
    if (json.HasMember(KEY_WBIAS1)) cal.w_bias[1] = (float)json[KEY_WBIAS1].GetDouble();
    if (json.HasMember(KEY_WBIAS2)) cal.w_bias[2] = (float)json[KEY_WBIAS2].GetDouble();
    if (json.HasMember(KEY_TC0)) cal.Tc[0] = (float)json[KEY_TC0].GetDouble();
    if (json.HasMember(KEY_TC1)) cal.Tc[1] = (float)json[KEY_TC1].GetDouble();
    if (json.HasMember(KEY_TC2)) cal.Tc[2] = (float)json[KEY_TC2].GetDouble();
    if (json.HasMember(KEY_WC0)) cal.Wc[0] = (float)json[KEY_WC0].GetDouble();
    if (json.HasMember(KEY_WC1)) cal.Wc[1] = (float)json[KEY_WC1].GetDouble();
    if (json.HasMember(KEY_WC2)) cal.Wc[2] = (float)json[KEY_WC2].GetDouble();
    if (json.HasMember(KEY_ABIASVAR0)) cal.a_bias_var[0] = (float)json[KEY_ABIASVAR0].GetDouble();
    if (json.HasMember(KEY_ABIASVAR1)) cal.a_bias_var[1] = (float)json[KEY_ABIASVAR1].GetDouble();
    if (json.HasMember(KEY_ABIASVAR2)) cal.a_bias_var[2] = (float)json[KEY_ABIASVAR2].GetDouble();
    if (json.HasMember(KEY_WBIASVAR0)) cal.w_bias_var[0] = (float)json[KEY_WBIASVAR0].GetDouble();
    if (json.HasMember(KEY_WBIASVAR1)) cal.w_bias_var[1] = (float)json[KEY_WBIASVAR1].GetDouble();
    if (json.HasMember(KEY_WBIASVAR2)) cal.w_bias_var[2] = (float)json[KEY_WBIASVAR2].GetDouble();
    if (json.HasMember(KEY_TCVAR0)) cal.Tc_var[0] = (float)json[KEY_TCVAR0].GetDouble();
    if (json.HasMember(KEY_TCVAR1)) cal.Tc_var[1] = (float)json[KEY_TCVAR1].GetDouble();
    if (json.HasMember(KEY_TCVAR2)) cal.Tc_var[2] = (float)json[KEY_TCVAR2].GetDouble();
    if (json.HasMember(KEY_WCVAR0)) cal.Wc_var[0] = (float)json[KEY_WCVAR0].GetDouble();
    if (json.HasMember(KEY_WCVAR1)) cal.Wc_var[1] = (float)json[KEY_WCVAR1].GetDouble();
    if (json.HasMember(KEY_WCVAR2)) cal.Wc_var[2] = (float)json[KEY_WCVAR2].GetDouble();
    if (json.HasMember(KEY_WMEASVAR)) cal.w_meas_var = (float)json[KEY_WMEASVAR].GetDouble();
    if (json.HasMember(KEY_AMEASVAR)) cal.a_meas_var = (float)json[KEY_AMEASVAR].GetDouble();
    if (json.HasMember(KEY_IMAGE_WIDTH)) cal.image_width = json[KEY_IMAGE_WIDTH].GetInt();
    if (json.HasMember(KEY_IMAGE_HEIGHT)) cal.image_height = json[KEY_IMAGE_HEIGHT].GetInt();
    if (json.HasMember(KEY_DISTORTION_MODEL)) cal.distortionModel = json[KEY_DISTORTION_MODEL].GetInt();
    if (json.HasMember(KEY_SHUTTER_DELAY)) cal.shutterDelay = (float)json[KEY_SHUTTER_DELAY].GetDouble();
    if (json.HasMember(KEY_SHUTTER_PERIOD)) cal.shutterPeriod = (float)json[KEY_SHUTTER_PERIOD].GetDouble();
    if (json.HasMember(KEY_TIMESTAMP_OFFSET)) cal.timeStampOffset = (float)json[KEY_TIMESTAMP_OFFSET].GetDouble();

    if (json.HasMember(KEY_ACCEL_TRANSFORM) && json[KEY_ACCEL_TRANSFORM].IsArray())
    {
        int arrSize = sizeof(cal.accelerometerTransform) / sizeof(cal.accelerometerTransform[0]);
        for (int i = 0; i < arrSize; i++)
        {
            cal.accelerometerTransform[i] = (float)json[KEY_ACCEL_TRANSFORM][i].GetDouble();
        }
    }
    if (json.HasMember(KEY_GYRO_TRANSFORM) && json[KEY_GYRO_TRANSFORM].IsArray())
    {
        int arrSize = sizeof(cal.gyroscopeTransform) / sizeof(cal.gyroscopeTransform[0]);
        for (int i = 0; i < arrSize; i++)
        {
            cal.gyroscopeTransform[i] = (float)json[KEY_GYRO_TRANSFORM][i].GetDouble();
        }
    }
}

void CopyRSStructToJson(const rcCalibration &cal, Document &json)
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

bool rs_calibration_serialize(const rcCalibration &cal, std::string &jsonString)
{
    Document json; 
    
    try
    {
        json.SetObject();

        CopyRSStructToJson(cal, json);

        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        json.Accept(writer);
        jsonString = buffer.GetString();
        return jsonString.length() > 0;
    }
    catch (std::exception e)
    {
        return false;
    }   
}

bool rs_calibration_deserialize(const std::string &jsonString, rcCalibration &cal)
{
    Document json;

    try
    {
        json.Parse(jsonString.c_str());
        if (json.HasParseError()) return false;
        CopyJsonToRSStruct(json, cal);
    }
    catch (std::exception e)
    {
        return false;
    }

    return true;
}
