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
#define KEY_DEVICE_NAME "deviceName"
//#define KEY_SHUTTER_DELAY "shutterDelay"
//#define KEY_SHUTTER_PERIOD "shutterPeriod"
//#define KEY_TIMESTAMP_OFFSET "timeStampOffset"

// these are meaningless. they are only used to initialize the json document.
static const char *CALIBRATION_DEFAULTS = R"(
{
    "deviceName" : "unknown",
    "calibrationVersion": 1,
    "Fx": 600,
    "Fy": 600,
    "Cx": 319.5,
    "Cy": 239.5,
    "px": 0,
    "py": 0,
    "K0": 0.20000000298023224,
    "K1": -0.20000000298023224,
    "K2": 0,
    "Kw": 0,
    "abias0": 0,
    "abias1": 0,
    "abias2": 0,
    "wbias0": 0,
    "wbias1": 0,
    "wbias2": 0,
    "Tc0": 0.0099999997764825821,
    "Tc1": 0.029999999329447746,
    "Tc2": 0,
    "Wc0": 2.2214415073394775,
    "Wc1": -2.2214415073394775,
    "Wc2": 0,
    "abiasvar0": 0.0096039995551109314,
    "abiasvar1": 0.0096039995551109314,
    "abiasvar2": 0.0096039995551109314,
    "wbiasvar0": 0.0076154354028403759,
    "wbiasvar1": 0.0076154354028403759,
    "wbiasvar2": 0.0076154354028403759,
    "TcVar0": 9.9999999747524271e-07,
    "TcVar1": 9.9999999747524271e-07,
    "TcVar2": 1.000000013351432e-10,
    "WcVar0": 9.9999999747524271e-07,
    "WcVar1": 9.9999999747524271e-07,
    "WcVar2": 9.9999999747524271e-07,
    "wMeasVar": 1.3707783182326239e-05,
    "aMeasVar": 0.00022821025049779564,
    "imageWidth": 640,
    "imageHeight": 480,
    "distortionModel": 0
}
)";

void CopyJsonToStruct(Document &json, rcCalibration &cal)
{
    std::string deviceName = json[KEY_DEVICE_NAME].GetString();
    int charLen = sizeof(cal.deviceName);
    strncpy(cal.deviceName, deviceName.c_str(), charLen - 1);
    cal.deviceName[charLen - 1] = '\0';

    cal.calibrationVersion = json[KEY_CALIBRATION_VERSION].GetInt();
    cal.Fx = (float)json[KEY_FX].GetDouble();
    cal.Fy = (float)json[KEY_FY].GetDouble();
    cal.Cx = (float)json[KEY_CX].GetDouble();
    cal.Cy = (float)json[KEY_CY].GetDouble();
    cal.px = (float)json[KEY_PX].GetDouble();
    cal.py = (float)json[KEY_PY].GetDouble();
    cal.K0 = (float)json[KEY_K0].GetDouble();
    cal.K1 = (float)json[KEY_K1].GetDouble();
    cal.K2 = (float)json[KEY_K2].GetDouble();
    cal.Kw = (float)json[KEY_KW].GetDouble();
    cal.abias0 = (float)json[KEY_ABIAS0].GetDouble();
    cal.abias1 = (float)json[KEY_ABIAS1].GetDouble();
    cal.abias2 = (float)json[KEY_ABIAS2].GetDouble();
    cal.wbias0 = (float)json[KEY_WBIAS0].GetDouble();
    cal.wbias1 = (float)json[KEY_WBIAS1].GetDouble();
    cal.wbias2 = (float)json[KEY_WBIAS2].GetDouble();
    cal.Tc0 = (float)json[KEY_TC0].GetDouble();
    cal.Tc1 = (float)json[KEY_TC1].GetDouble();
    cal.Tc2 = (float)json[KEY_TC2].GetDouble();
    cal.Wc0 = (float)json[KEY_WC0].GetDouble();
    cal.Wc1 = (float)json[KEY_WC1].GetDouble();
    cal.Wc2 = (float)json[KEY_WC2].GetDouble();
    cal.abiasvar0 = (float)json[KEY_ABIASVAR0].GetDouble();
    cal.abiasvar1 = (float)json[KEY_ABIASVAR1].GetDouble();
    cal.abiasvar2 = (float)json[KEY_ABIASVAR2].GetDouble();
    cal.wbiasvar0 = (float)json[KEY_WBIASVAR0].GetDouble();
    cal.wbiasvar1 = (float)json[KEY_WBIASVAR1].GetDouble();
    cal.wbiasvar2 = (float)json[KEY_WBIASVAR2].GetDouble();
    cal.TcVar0 = (float)json[KEY_TCVAR0].GetDouble();
    cal.TcVar1 = (float)json[KEY_TCVAR1].GetDouble();
    cal.TcVar2 = (float)json[KEY_TCVAR2].GetDouble();
    cal.WcVar0 = (float)json[KEY_WCVAR0].GetDouble();
    cal.WcVar1 = (float)json[KEY_WCVAR1].GetDouble();
    cal.WcVar2 = (float)json[KEY_WCVAR2].GetDouble();
    cal.wMeasVar = (float)json[KEY_WMEASVAR].GetDouble();
    cal.aMeasVar = (float)json[KEY_AMEASVAR].GetDouble();
    cal.imageWidth = json[KEY_IMAGE_WIDTH].GetInt();
    cal.imageHeight = json[KEY_IMAGE_HEIGHT].GetInt();
    cal.distortionModel = json[KEY_DISTORTION_MODEL].GetInt();
//    cal.shutterDelay = (float)json[KEY_SHUTTER_DELAY].GetDouble();
//    cal.shutterPeriod = (float)json[KEY_SHUTTER_PERIOD].GetDouble();
//    cal.timeStampOffset = (float)json[KEY_TIMESTAMP_OFFSET].GetDouble();
}

void CopyStructToJson(const rcCalibration &cal, Document &json)
{
    json[KEY_DEVICE_NAME].SetString(cal.deviceName, json.GetAllocator());
    json[KEY_CALIBRATION_VERSION] = cal.calibrationVersion;
    json[KEY_FX] = cal.Fx;
    json[KEY_FY] = cal.Fy;
    json[KEY_CX] = cal.Cx;
    json[KEY_CY] = cal.Cy;
    json[KEY_PX] = cal.px;
    json[KEY_PY] = cal.py;
    json[KEY_K0] = cal.K0;
    json[KEY_K1] = cal.K1;
    json[KEY_K2] = cal.K2;
    json[KEY_KW] = cal.Kw;
    json[KEY_ABIAS0] = cal.abias0;
    json[KEY_ABIAS1] = cal.abias1;
    json[KEY_ABIAS2] = cal.abias2;
    json[KEY_WBIAS0] = cal.wbias0;
    json[KEY_WBIAS1] = cal.wbias1;
    json[KEY_WBIAS2] = cal.wbias2;
    json[KEY_TC0] = cal.Tc0;
    json[KEY_TC1] = cal.Tc1;
    json[KEY_TC2] = cal.Tc2;
    json[KEY_WC0] = cal.Wc0;
    json[KEY_WC1] = cal.Wc1;
    json[KEY_WC2] = cal.Wc2;
    json[KEY_ABIASVAR0] = cal.abiasvar0;
    json[KEY_ABIASVAR1] = cal.abiasvar1;
    json[KEY_ABIASVAR2] = cal.abiasvar2;
    json[KEY_WBIASVAR0] = cal.wbiasvar0;
    json[KEY_WBIASVAR1] = cal.wbiasvar1;
    json[KEY_WBIASVAR2] = cal.wbiasvar2;
    json[KEY_TCVAR0] = cal.TcVar0;
    json[KEY_TCVAR1] = cal.TcVar1;
    json[KEY_TCVAR2] = cal.TcVar2;
    json[KEY_WCVAR0] = cal.WcVar0;
    json[KEY_WCVAR1] = cal.WcVar1;
    json[KEY_WCVAR2] = cal.WcVar2;
    json[KEY_WMEASVAR] = cal.wMeasVar;
    json[KEY_AMEASVAR] = cal.aMeasVar;
    json[KEY_IMAGE_WIDTH] = cal.imageWidth;
    json[KEY_IMAGE_HEIGHT] = cal.imageHeight;
    json[KEY_DISTORTION_MODEL] = cal.distortionModel;
//    json[KEY_SHUTTER_DELAY] = cal.shutterDelay;
//    json[KEY_SHUTTER_PERIOD] = cal.shutterPeriod;
//    json[KEY_TIMESTAMP_OFFSET] = cal.timeStampOffset;
}

bool calibration_serialize(const rcCalibration &cal, std::string &jsonString)
{
    Document json; json.Parse(CALIBRATION_DEFAULTS);
    CopyStructToJson(cal, json);
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    json.Accept(writer);
    jsonString = buffer.GetString();
    return jsonString.length() > 0;
}

bool calibration_deserialize(const std::string &jsonString, rcCalibration &cal)
{
    Document json; json.Parse(jsonString.c_str());
    if (json.HasParseError())
        return false;
    CopyJsonToStruct(json, cal);
    return true;
}
