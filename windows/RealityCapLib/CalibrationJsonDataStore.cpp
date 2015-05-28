#include "stdafx.h"
#include "CalibrationJsonDataStore.h"
#include <cpprest\json.h>
#include <cpprest\details\basic_types.h>
#include <iostream>
#include <fstream>
#include <sys/stat.h>

using namespace RealityCap;
using namespace web;
using namespace web::json;
using namespace utility;
using namespace std;

CalibrationJsonDataStore::CalibrationJsonDataStore()
{
}


CalibrationJsonDataStore::~CalibrationJsonDataStore()
{
}

bool FileExists(string filename) 
{
    struct stat fileInfo;
    return stat(filename.c_str(), &fileInfo) == 0;
}

corvis_device_parameters ParseCalibrationFile(string fileName)
{
    if (!FileExists(fileName)) throw std::runtime_error("Calibration file not found.");

    wifstream jsonFile;
    jsonFile.open(fileName);
    if (jsonFile.fail()) throw runtime_error("Failed to open calibraiton file.");
    value json = value::parse(jsonFile);

    corvis_device_parameters cal;
    cal.Fx = json[U(KEY_FX)].as_number().to_double();
    cal.Fy = json[U(KEY_FY)].as_number().to_double();
    cal.Cx = json[U(KEY_CX)].as_number().to_double();
    cal.Cy = json[U(KEY_CY)].as_number().to_double();
    cal.px = json[U(KEY_PX)].as_number().to_double();
    cal.py = json[U(KEY_PY)].as_number().to_double();
    cal.K[0] = json[U(KEY_K0)].as_number().to_double();
    cal.K[1] = json[U(KEY_K1)].as_number().to_double();
    cal.K[2] = json[U(KEY_K2)].as_number().to_double();
    cal.a_bias[0] = json[U(KEY_ABIAS0)].as_number().to_double();
    cal.a_bias[1] = json[U(KEY_ABIAS1)].as_number().to_double();
    cal.a_bias[2] = json[U(KEY_ABIAS2)].as_number().to_double();
    cal.w_bias[0] = json[U(KEY_WBIAS0)].as_number().to_double();
    cal.w_bias[1] = json[U(KEY_WBIAS1)].as_number().to_double();
    cal.w_bias[2] = json[U(KEY_WBIAS2)].as_number().to_double();
    cal.Tc[0] = json[U(KEY_TC0)].as_number().to_double();
    cal.Tc[1] = json[U(KEY_TC1)].as_number().to_double();
    cal.Tc[2] = json[U(KEY_TC2)].as_number().to_double();
    cal.Wc[0] = json[U(KEY_WC0)].as_number().to_double();
    cal.Wc[1] = json[U(KEY_WC1)].as_number().to_double();
    cal.Wc[2] = json[U(KEY_WC2)].as_number().to_double();
    cal.a_bias_var[0] = json[U(KEY_ABIASVAR0)].as_number().to_double();
    cal.a_bias_var[1] = json[U(KEY_ABIASVAR1)].as_number().to_double();
    cal.a_bias_var[2] = json[U(KEY_ABIASVAR2)].as_number().to_double();
    cal.w_bias_var[0] = json[U(KEY_WBIASVAR0)].as_number().to_double();
    cal.w_bias_var[1] = json[U(KEY_WBIASVAR1)].as_number().to_double();
    cal.w_bias_var[2] = json[U(KEY_WBIASVAR2)].as_number().to_double();
    cal.Tc_var[0] = json[U(KEY_TCVAR0)].as_number().to_double();
    cal.Tc_var[1] = json[U(KEY_TCVAR1)].as_number().to_double();
    cal.Tc_var[2] = json[U(KEY_TCVAR2)].as_number().to_double();
    cal.Wc_var[0] = json[U(KEY_WCVAR0)].as_number().to_double();
    cal.Wc_var[1] = json[U(KEY_WCVAR1)].as_number().to_double();
    cal.Wc_var[2] = json[U(KEY_WCVAR2)].as_number().to_double();
    cal.w_meas_var = json[U(KEY_WMEASVAR)].as_number().to_double();
    cal.a_meas_var = json[U(KEY_AMEASVAR)].as_number().to_double();
    cal.image_width = json[U(KEY_IMAGE_WIDTH)].as_number().to_double();
    cal.image_height = json[U(KEY_IMAGE_HEIGHT)].as_number().to_double();
    cal.shutter_delay = std::chrono::microseconds(json[U(KEY_SHUTTER_DELAY)].as_integer());
    cal.shutter_period = std::chrono::microseconds(json[U(KEY_SHUTTER_PERIOD)].as_integer());
    cal.version = json[U(KEY_CALIBRATION_VERSION)].as_number().to_uint32();

    return cal;
}

corvis_device_parameters RealityCap::CalibrationJsonDataStore::GetCalibration()
{
    if (FileExists(CALIBRATION_FILE_NAME))
    {
        corvis_device_parameters cal = ParseCalibrationFile(CALIBRATION_FILE_NAME);
        if (cal.version == CALIBRATION_VERSION)
            return cal;
        else
            return ParseCalibrationFile(CALIBRATION_DEFAULTS_FILE_NAME);
    }
    else
    {
        return ParseCalibrationFile(CALIBRATION_DEFAULTS_FILE_NAME);
    }
}

void CalibrationJsonDataStore::SaveCalibration(corvis_device_parameters cal)
{
    value json = value::object();
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
    json[U(KEY_CALIBRATION_VERSION)] = value::number(CALIBRATION_VERSION);

    wofstream jsonFile;
    jsonFile.open(CALIBRATION_FILE_NAME);
    if (jsonFile.fail()) 
        throw runtime_error("Failed to open calibration file for writing.");
    jsonFile << json;
    jsonFile.close();
}

int RealityCap::CalibrationJsonDataStore::ClearCalibration()
{
    return remove(CALIBRATION_FILE_NAME);
}

bool RealityCap::CalibrationJsonDataStore::HasCalibration()
{
    if (!FileExists(CALIBRATION_FILE_NAME)) return false;
    corvis_device_parameters cal = ParseCalibrationFile(CALIBRATION_FILE_NAME);
    corvis_device_parameters defaults = ParseCalibrationFile(CALIBRATION_DEFAULTS_FILE_NAME);
    return is_calibration_valid(cal, defaults);
}
