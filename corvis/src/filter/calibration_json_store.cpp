#include "device_parameters.h"
#include "json_keys.h"
#include "calibration_json_store.h"
#include "calibration_defaults.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>

using namespace rapidjson;
using namespace std;


void CopyJsonToStruct(Document &json, device_parameters &cal)
{
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
    cal.a_bias[0] = (float)json[KEY_ABIAS0].GetDouble();
    cal.a_bias[1] = (float)json[KEY_ABIAS1].GetDouble();
    cal.a_bias[2] = (float)json[KEY_ABIAS2].GetDouble();
    cal.w_bias[0] = (float)json[KEY_WBIAS0].GetDouble();
    cal.w_bias[1] = (float)json[KEY_WBIAS1].GetDouble();
    cal.w_bias[2] = (float)json[KEY_WBIAS2].GetDouble();
    cal.Tc[0] = (float)json[KEY_TC0].GetDouble();
    cal.Tc[1] = (float)json[KEY_TC1].GetDouble();
    cal.Tc[2] = (float)json[KEY_TC2].GetDouble();
    cal.Wc[0] = (float)json[KEY_WC0].GetDouble();
    cal.Wc[1] = (float)json[KEY_WC1].GetDouble();
    cal.Wc[2] = (float)json[KEY_WC2].GetDouble();
    cal.a_bias_var[0] = (float)json[KEY_ABIASVAR0].GetDouble();
    cal.a_bias_var[1] = (float)json[KEY_ABIASVAR1].GetDouble();
    cal.a_bias_var[2] = (float)json[KEY_ABIASVAR2].GetDouble();
    cal.w_bias_var[0] = (float)json[KEY_WBIASVAR0].GetDouble();
    cal.w_bias_var[1] = (float)json[KEY_WBIASVAR1].GetDouble();
    cal.w_bias_var[2] = (float)json[KEY_WBIASVAR2].GetDouble();
    cal.Tc_var[0] = (float)json[KEY_TCVAR0].GetDouble();
    cal.Tc_var[1] = (float)json[KEY_TCVAR1].GetDouble();
    cal.Tc_var[2] = (float)json[KEY_TCVAR2].GetDouble();
    cal.Wc_var[0] = (float)json[KEY_WCVAR0].GetDouble();
    cal.Wc_var[1] = (float)json[KEY_WCVAR1].GetDouble();
    cal.Wc_var[2] = (float)json[KEY_WCVAR2].GetDouble();
    cal.w_meas_var = (float)json[KEY_WMEASVAR].GetDouble();
    cal.a_meas_var = (float)json[KEY_AMEASVAR].GetDouble();
    cal.image_width = json[KEY_IMAGE_WIDTH].GetInt();
    cal.image_height = json[KEY_IMAGE_HEIGHT].GetInt();
    cal.distortionModel = 0;
}

void CopyStructToJson(const device_parameters &cal, Value &json)
{
    json[KEY_CALIBRATION_VERSION] = CALIBRATION_VERSION;
    json[KEY_FX] = cal.Fx;
    json[KEY_FY] = cal.Fy;
    json[KEY_CX] = cal.Cx;
    json[KEY_CY] = cal.Cy;
    json[KEY_PX] = cal.px;
    json[KEY_PY] = cal.py;
    json[KEY_K0] = cal.K0;
    json[KEY_K1] = cal.K1;
    json[KEY_K2] = cal.K2;
    json[KEY_ABIAS0] = cal.a_bias[0];
    json[KEY_ABIAS1] = cal.a_bias[1];
    json[KEY_ABIAS2] = cal.a_bias[2];
    json[KEY_WBIAS0] = cal.w_bias[0];
    json[KEY_WBIAS1] = cal.w_bias[1];
    json[KEY_WBIAS2] = cal.w_bias[2];
    json[KEY_TC0] = cal.Tc[0];
    json[KEY_TC1] = cal.Tc[1];
    json[KEY_TC2] = cal.Tc[2];
    json[KEY_WC0] = cal.Wc[0];
    json[KEY_WC1] = cal.Wc[1];
    json[KEY_WC2] = cal.Wc[2];
    json[KEY_ABIASVAR0] = cal.a_bias_var[0];
    json[KEY_ABIASVAR1] = cal.a_bias_var[1];
    json[KEY_ABIASVAR2] = cal.a_bias_var[2];
    json[KEY_WBIASVAR0] = cal.w_bias_var[0];
    json[KEY_WBIASVAR1] = cal.w_bias_var[1];
    json[KEY_WBIASVAR2] = cal.w_bias_var[2];
    json[KEY_TCVAR0] = cal.Tc_var[0];
    json[KEY_TCVAR1] = cal.Tc_var[1];
    json[KEY_TCVAR2] = cal.Tc_var[2];
    json[KEY_WCVAR0] = cal.Wc_var[0];
    json[KEY_WCVAR1] = cal.Wc_var[1];
    json[KEY_WCVAR2] = cal.Wc_var[2];
    json[KEY_WMEASVAR] = cal.w_meas_var;
    json[KEY_AMEASVAR] = cal.a_meas_var;
    json[KEY_IMAGE_WIDTH] = cal.image_width;
    json[KEY_IMAGE_HEIGHT] = cal.image_height;
}

bool calibration_serialize(const device_parameters &cal, std::string &jsonString)
{
    Document json; json.Parse(calibration_default_json_for_device_type(DEVICE_TYPE_UNKNOWN));
    CopyStructToJson(cal, json);
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    json.Accept(writer);
    jsonString = buffer.GetString();
    return jsonString.length() > 0;
}

bool calibration_deserialize(const std::string &jsonString, device_parameters &cal)
{
    Document json; json.Parse(jsonString.c_str());
    if (json.HasParseError())
        return false;
    CopyJsonToStruct(json, cal);
    return true;
}

bool calibration_load_defaults(const corvis_device_type deviceType, device_parameters &cal)
{
    bool result = calibration_deserialize(calibration_default_json_for_device_type(deviceType), cal);
    if (result) cal.calibrationVersion = CALIBRATION_VERSION; // need this here to override possibly outdated value in default JSON
    return result;
}
