#include "device_parameters.h"
#include "calibration_json_store.h"
#include "cpprest/json.h"
#include "cpprest/details/basic_types.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>

using namespace RealityCap;
using namespace web;
using namespace web::json;
using namespace utility;
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
#define KEY_SHUTTER_DELAY "shutterDelay"
#define KEY_SHUTTER_PERIOD "shutterPeriod"
#define KEY_CALIBRATION_VERSION "calibrationVersion"

calibration_json_store::calibration_json_store()
{
}


calibration_json_store::~calibration_json_store()
{
}

bool FileExists(string filename) 
{
    struct stat fileInfo;
    return stat(filename.c_str(), &fileInfo) == 0;
}

string GetCalibrationDefaultsFileName(corvis_device_type deviceType)
{
    string deviceTypeString = get_device_type_string(deviceType);
    return deviceTypeString.append(".json");
}

void CopyJsonToStruct(value &json, corvis_device_parameters &cal)
{
    cal.version = json[U(KEY_CALIBRATION_VERSION)].as_number().to_uint32();
    cal.Fx = (float)json[U(KEY_FX)].as_number().to_double();
    cal.Fy = (float)json[U(KEY_FY)].as_number().to_double();
    cal.Cx = (float)json[U(KEY_CX)].as_number().to_double();
    cal.Cy = (float)json[U(KEY_CY)].as_number().to_double();
    cal.px = (float)json[U(KEY_PX)].as_number().to_double();
    cal.py = (float)json[U(KEY_PY)].as_number().to_double();
    cal.K[0] = (float)json[U(KEY_K0)].as_number().to_double();
    cal.K[1] = (float)json[U(KEY_K1)].as_number().to_double();
    cal.K[2] = (float)json[U(KEY_K2)].as_number().to_double();
    cal.a_bias[0] = (float)json[U(KEY_ABIAS0)].as_number().to_double();
    cal.a_bias[1] = (float)json[U(KEY_ABIAS1)].as_number().to_double();
    cal.a_bias[2] = (float)json[U(KEY_ABIAS2)].as_number().to_double();
    cal.w_bias[0] = (float)json[U(KEY_WBIAS0)].as_number().to_double();
    cal.w_bias[1] = (float)json[U(KEY_WBIAS1)].as_number().to_double();
    cal.w_bias[2] = (float)json[U(KEY_WBIAS2)].as_number().to_double();
    cal.Tc[0] = (float)json[U(KEY_TC0)].as_number().to_double();
    cal.Tc[1] = (float)json[U(KEY_TC1)].as_number().to_double();
    cal.Tc[2] = (float)json[U(KEY_TC2)].as_number().to_double();
    cal.Wc[0] = (float)json[U(KEY_WC0)].as_number().to_double();
    cal.Wc[1] = (float)json[U(KEY_WC1)].as_number().to_double();
    cal.Wc[2] = (float)json[U(KEY_WC2)].as_number().to_double();
    cal.a_bias_var[0] = (float)json[U(KEY_ABIASVAR0)].as_number().to_double();
    cal.a_bias_var[1] = (float)json[U(KEY_ABIASVAR1)].as_number().to_double();
    cal.a_bias_var[2] = (float)json[U(KEY_ABIASVAR2)].as_number().to_double();
    cal.w_bias_var[0] = (float)json[U(KEY_WBIASVAR0)].as_number().to_double();
    cal.w_bias_var[1] = (float)json[U(KEY_WBIASVAR1)].as_number().to_double();
    cal.w_bias_var[2] = (float)json[U(KEY_WBIASVAR2)].as_number().to_double();
    cal.Tc_var[0] = (float)json[U(KEY_TCVAR0)].as_number().to_double();
    cal.Tc_var[1] = (float)json[U(KEY_TCVAR1)].as_number().to_double();
    cal.Tc_var[2] = (float)json[U(KEY_TCVAR2)].as_number().to_double();
    cal.Wc_var[0] = (float)json[U(KEY_WCVAR0)].as_number().to_double();
    cal.Wc_var[1] = (float)json[U(KEY_WCVAR1)].as_number().to_double();
    cal.Wc_var[2] = (float)json[U(KEY_WCVAR2)].as_number().to_double();
    cal.w_meas_var = (float)json[U(KEY_WMEASVAR)].as_number().to_double();
    cal.a_meas_var = (float)json[U(KEY_AMEASVAR)].as_number().to_double();
    cal.image_width = json[U(KEY_IMAGE_WIDTH)].as_number().to_int32();
    cal.image_height = json[U(KEY_IMAGE_HEIGHT)].as_number().to_int32();
    cal.shutter_delay = std::chrono::microseconds(json[U(KEY_SHUTTER_DELAY)].as_integer());
    cal.shutter_period = std::chrono::microseconds(json[U(KEY_SHUTTER_PERIOD)].as_integer());
}

void CopyStructToJson(const corvis_device_parameters &cal, value &json)
{
    json[U(KEY_CALIBRATION_VERSION)] = value::number(CALIBRATION_VERSION);
    json[U(KEY_FX)] = value::number(cal.Fx);
    json[U(KEY_FY)] = value::number(cal.Fy);
    json[U(KEY_CX)] = value::number(cal.Cx);
    json[U(KEY_CY)] = value::number(cal.Cy);
    json[U(KEY_PX)] = value::number(cal.px);
    json[U(KEY_PY)] = value::number(cal.py);
    json[U(KEY_K0)] = value::number(cal.K[0]);
    json[U(KEY_K1)] = value::number(cal.K[1]);
    json[U(KEY_K2)] = value::number(cal.K[2]);
    json[U(KEY_ABIAS0)] = value::number(cal.a_bias[0]);
    json[U(KEY_ABIAS1)] = value::number(cal.a_bias[1]);
    json[U(KEY_ABIAS2)] = value::number(cal.a_bias[2]);
    json[U(KEY_WBIAS0)] = value::number(cal.w_bias[0]);
    json[U(KEY_WBIAS1)] = value::number(cal.w_bias[1]);
    json[U(KEY_WBIAS2)] = value::number(cal.w_bias[2]);
    json[U(KEY_TC0)] = value::number(cal.Tc[0]);
    json[U(KEY_TC1)] = value::number(cal.Tc[1]);
    json[U(KEY_TC2)] = value::number(cal.Tc[2]);
    json[U(KEY_WC0)] = value::number(cal.Wc[0]);
    json[U(KEY_WC1)] = value::number(cal.Wc[1]);
    json[U(KEY_WC2)] = value::number(cal.Wc[2]);
    json[U(KEY_ABIASVAR0)] = value::number(cal.a_bias_var[0]);
    json[U(KEY_ABIASVAR1)] = value::number(cal.a_bias_var[1]);
    json[U(KEY_ABIASVAR2)] = value::number(cal.a_bias_var[2]);
    json[U(KEY_WBIASVAR0)] = value::number(cal.w_bias_var[0]);
    json[U(KEY_WBIASVAR1)] = value::number(cal.w_bias_var[1]);
    json[U(KEY_WBIASVAR2)] = value::number(cal.w_bias_var[2]);
    json[U(KEY_TCVAR0)] = value::number(cal.Tc_var[0]);
    json[U(KEY_TCVAR1)] = value::number(cal.Tc_var[1]);
    json[U(KEY_TCVAR2)] = value::number(cal.Tc_var[2]);
    json[U(KEY_WCVAR0)] = value::number(cal.Wc_var[0]);
    json[U(KEY_WCVAR1)] = value::number(cal.Wc_var[1]);
    json[U(KEY_WCVAR2)] = value::number(cal.Wc_var[2]);
    json[U(KEY_WMEASVAR)] = value::number(cal.w_meas_var);
    json[U(KEY_AMEASVAR)] = value::number(cal.a_meas_var);
    json[U(KEY_IMAGE_WIDTH)] = value::number(cal.image_width);
    json[U(KEY_IMAGE_HEIGHT)] = value::number(cal.image_height);
    json[U(KEY_SHUTTER_DELAY)] = value::number((double)std::chrono::duration_cast<std::chrono::microseconds>(cal.shutter_delay).count());
    json[U(KEY_SHUTTER_PERIOD)] = value::number((double)std::chrono::duration_cast<std::chrono::microseconds>(cal.shutter_period).count());
}

bool RealityCap::calibration_json_store::SerializeCalibration(const corvis_device_parameters &cal, utility::string_t &jsonString)
{
    try
    {
        value json = value::object();
        CopyStructToJson(cal, json);
        jsonString = json.serialize();
        if (!jsonString.length()) return false;
    }
    catch (json_exception)
    {
        return false;
    }

    return true;
}

bool RealityCap::calibration_json_store::DeserializeCalibration(const utility::string_t &jsonString, corvis_device_parameters &cal)
{
    try
    {
        if (!jsonString.length()) return false;

        value json = value::parse(jsonString);
        CopyJsonToStruct(json, cal);
    }
    catch (json_exception)
    {
        return false;
    }

    return true;
}

void calibration_json_store::ParseCalibrationFile(string fileName, corvis_device_parameters &cal)
{
    string fullName = calibrationLocation + fileName;
    if (!FileExists(fullName)) throw runtime_error("Calibration file not found.");

    utility::ifstream_t jsonFile;
    jsonFile.open(fullName);
    if (jsonFile.fail()) throw runtime_error("Failed to open calibration file.");
    value json = value::parse(jsonFile);

    CopyJsonToStruct(json, cal);
}

bool RealityCap::calibration_json_store::LoadCalibration(corvis_device_parameters &cal)
{
    if (FileExists(calibrationLocation + CALIBRATION_FILE_NAME))
    {
        ParseCalibrationFile(CALIBRATION_FILE_NAME, cal);
        return true;
    }
    else
    {
        return false;
    }
}

bool RealityCap::calibration_json_store::LoadCalibrationDefaults(const corvis_device_type deviceType, corvis_device_parameters &cal)
{
    string fileName = GetCalibrationDefaultsFileName(deviceType);
    if (FileExists(calibrationLocation + fileName))
    {
        ParseCalibrationFile(fileName, cal);
        return true;
    }
    else
    {
        return false;
    }
}

bool calibration_json_store::SaveCalibration(const corvis_device_parameters &cal)
{
    return SaveCalibration(cal, CALIBRATION_FILE_NAME);
}

bool calibration_json_store::SaveCalibration(const corvis_device_parameters &cal, const char* fileName)
{
    try
    {
        value json = value::object();
        CopyStructToJson(cal, json);

        utility::ofstream_t jsonFile;
        jsonFile.open(calibrationLocation + fileName);
        if (jsonFile.fail()) throw runtime_error("Failed to open calibration file for writing.");
        jsonFile << json;
        jsonFile.close();

        return true;
    }
    catch (...)
    {
        // TODO: log error?
        return false;
    }
}

bool RealityCap::calibration_json_store::ClearCalibration()
{
    return remove(CALIBRATION_FILE_NAME) == 0;
}

bool RealityCap::calibration_json_store::HasCalibration()
{
    corvis_device_parameters cal;
    return LoadCalibration(cal);
}
