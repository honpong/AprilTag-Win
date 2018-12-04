/*
* INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement
* or nondisclosure agreement with Intel Corporation and may not be
* copied or disclosed except in accordance with the terms of that
* agreement.
* Copyright(c) 2014-2018 Intel Corporation. All Rights Reserved.
*/

#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

#include "librealsense2/rs.hpp"
#include "librealsense2/rs_advanced_mode.hpp"

#include "rs2-custom-calibration-mm.h"

using namespace std;
using namespace rs2;
using namespace crw;


// example use only for print a buffer content to console
void dump_to_console(uint8_t* buffer, uint32_t size)
{
    // save original settings
    ios_base::fmtflags origFlags = cout.flags();
    streamsize         origPrec = cout.precision();
    char               origFill = cout.fill();

    cout << "buffer size: " << size << " bytes" << endl;
    cout << hex << std::uppercase;

    // print header
    cout << endl << "Offset (h)";

    for (int i = 0; i < 16; i++)
    {
        cout << " " << noshowbase << setw(2) << setfill('0') << (int)i;
    }

    cout << endl;

    for (int i = 0; i < 58; i++)
    {
        cout << "-";
    }

    uint16_t c = 0;
    uint16_t more = size;

    // print the buffer content in hex, byte by byte, 16 bytes per line
    while (more)
    {
        uint16_t offset = c - c % 16;

        if (c % 16 == 0)
        {
            cout << endl << setw(6) << setfill('0') << (int)offset << " ==>";
        }

        cout << " " << noshowbase << setw(2) << setfill('0') << (int)buffer[c];

        c++;
        more--;
    }

    cout << endl << endl;

    // restore original settings
    cout.flags(origFlags);
    cout.precision(origPrec);
    cout.fill(origFill);
}

//
// new D435i devices include IMU but its not calibrated, so user will need to calibrate the IMU before
// using it. A custom calibration r/w API is provided to write/read user calibration data to the device
// this example demos usage of the IMU custom calibration data read/write APIs
//

int main(int argc, char * argv[]) try
{
    cout << "simple imu custom data r/w sample for Intel RealSense D400, Version: " << DS_CUSTOM_CALIBRATION_VERSION << endl << endl;

    // rs context
    rs2::context *ctx = new context();

    // query devices on host
    auto devices = ctx->query_devices();

    if (devices.size() == 0)
    {
        std::cout << "No Intel RealSense device connected." << std::endl;
        return 0;
    }

    // only the first device got picked, so connect only one device when try this example 
    rs2::device mydev = devices[0];

    // to use the custom calibration read/write api, the device should be in advanced mode
    if (mydev.is<rs400::advanced_mode>())
    {
        rs400::advanced_mode advanced = mydev.as<rs400::advanced_mode>();
        if (!advanced.is_enabled())
        {
            advanced.toggle_advanced_mode(true);
            delete ctx;
            ctx = new context();
            devices = ctx->query_devices();
        }
    }

    mydev = devices[0];

    // print a few basic information about the device
    cout << "  Device PID: " << mydev.get_info(RS2_CAMERA_INFO_PRODUCT_ID) << endl;
    cout << "  Device name: " << mydev.get_info(RS2_CAMERA_INFO_NAME) << endl;
    cout << "  Serial number: " << mydev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER) << endl;
    cout << "  Firmware version: " << mydev.get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION) << endl;
    cout << endl;


    // initialize custom calibration R/W API
    MMCalibrationRWAPI *m_dcApi = new MMCalibrationRWAPI();

    // mydev should be a pointer to rs2::device from librealsense
    int ret = m_dcApi->Initialize(&mydev);
    if (ret != DC_MM_SUCCESS)
    {
        delete m_dcApi;
        return DC_MM_ERROR_FAIL;
    }

    int status = DC_MM_SUCCESS;
    uint8_t mmCustom[DC_MM_CUSTOM_DATA_SIZE];

    // the custom data area has DC_MM_CUSTOM_DATA_SIZE (504 bytes) of storage space. User can 
    // define their data own format and write to the device and retrieve it later from
    // the device

    // on a new device, the storage is not initialzed, so should write data before perform
    // read. reading before writing any data will fail.

    memset((void*)&mmCustom, 0xFF, sizeof(mmCustom));

    mmCustom[160] = 0xB;
    mmCustom[180] = 0x8;

    ret = m_dcApi->WriteMMCustomData((uint8_t*) &mmCustom);

    if (ret == DC_MM_SUCCESS)
    {
        cout << "imu custom data successfully write to device " << endl;
	   status = EXIT_SUCCESS;
    }
    else
    {
        cout << "Failed to write imu custom data to device: error " << ret << endl;
        status = EXIT_FAILURE;
    }

    // read the custom data back from the device
    ret = m_dcApi->ReadMMCustomData(mmCustom);

    if (ret == DC_MM_SUCCESS)
    {
        dump_to_console(mmCustom, sizeof(mmCustom));
        status = EXIT_SUCCESS;
    }
    else
    {
        cout << "Failed to read motion module custom data." << endl;
        status = EXIT_FAILURE;
    }

    delete m_dcApi;

    return status;
}
catch (const rs2::error & e)
{
    std::cerr << "error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
