/*
* INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement
* or nondisclosure agreement with Intel Corporation and may not be
* copied or disclosed except in accordance with the terms of that
* agreement.
* Copyright(c) 2014-2018 Intel Corporation. All Rights Reserved.
*/

#ifndef _D400_MM_CALIBRATION_DATA_RW_API_H_
#define _D400_MM_CALIBRATION_DATA_RW_API_H_

#define DS_CUSTOM_CALIBRATION_VERSION "2.6.8.0"

#include <cstdint>

#ifdef DSDYNCAL_EXPORTS
#if defined(_WIN32) || defined(__WIN32__)
#define DSDYNCAL_API __declspec(dllexport)
#else
#if defined(__GNUC__) && defined(GCC_HASCLASSVISIBILITY)
#define DSDYNCAL_API __attribute__((visibility("default")))
#else
#define DSDYNCAL_API
#endif
#endif
#else
#if defined(DSDYNCAL_IMPPORTS)
#if defined(_WIN32) || defined(__WIN32__)
#define DSDYNCAL_API __declspec(dllimport)
#else
#define DSDYNCAL_API
#endif
#else
#define DSDYNCAL_API
#endif
#endif

#define DC_MM_CUSTOM_DATA_SIZE        504   // 0x1F8
#define DC_MM_FE_CUSTOM_DATA_SIZE     136   // 0x88

// error codes
#define DC_MM_SUCCESS                                              0    // successful

#define DC_MM_ERROR_INVALID_BUFFER                                 1    // invalid or NULL pointer to buffer provided
#define DC_MM_ERROR_INVALID_PARAMETER                              2    // invalid parameter provided
#define DC_MM_ERROR_NOT_INITIALIZED                                3    // library not initialized
#define DC_MM_ERROR_FAIL                                           4    // generic error for failure where error code is not used
#define DC_MM_ERROR_UNKNOWN                                     9999    // other errors for unknown reason

namespace crw
{
    /** interface for custom calibration data r/w for motion modules like IMU intrinsics and extrinsics
     *  the API simply provides interface to the internal storage space and let user to define their own
     *  data format to suit their specific usage
     */
    class DSDYNCAL_API MMCalibrationRWAPI
    {
    public:
        MMCalibrationRWAPI();
       ~MMCalibrationRWAPI();

        /** Initialize library
         *
         *  @param rs2Dev: opaque device handle, currently only librealsense handle rs2::device is supported on both Windows and Linux
         *  @return: returns DC_MM_SUCCESS if no issue, error code otherwise:
         *      DC_MM_ERROR_INVALID_PARAMETER - invalid parameter passed in
         */
        int Initialize(void *rs2Dev);

        /** Get version of dynamic calibration as a string
         *
         * @return: Version of Custom Calibration Data R/W API
         */
        static const char * GetVersion();

        /** Read FE custom data from device into buffer
        *  @param buffer: user allocated buffer in size of DC_MM_FE_CUSTOM_DATA_SIZE
        *  @return When DC_MM_SUCCESS then data read successfully
        *          DC_MM_ERROR_NOT_INITIALIZED - library not initialized
        *          DC_MM_ERROR_INVALID_BUFFER - invalid buffer
        *          DC_MM_ERROR_FAIL - failed to read data from the device
        */
        int ReadFECustomData(uint8_t* buffer);

        /** Write FE custom data buffer to device
        *  @param buffer: user allocated buffer in size of DC_MM_FE_CUSTOM_DATA_SIZE that contains custom data to be written to device
        *  @return When DC_MM_SUCCESS then data written uccessfully
        *          DC_MM_ERROR_NOT_INITIALIZED - library not initialized
        *          DC_MM_ERROR_INVALID_BUFFER - invalid buffer
        *          DC_MM_ERROR_FAIL - failed to write data to device
        */
        int WriteFECustomData(uint8_t* buffer);

        /** Read motion module custom data from device into buffer
        *  @param buffer: user allocated buffer in size of DC_MM_CUSTOM_DATA_SIZE (504 bytes)
        *  @return When DC_MM_SUCCESS then data read successfully
        *          DC_MM_ERROR_NOT_INITIALIZED - library not initialized
        *          DC_MM_ERROR_INVALID_BUFFER - invalid buffer
        *          DC_MM_ERROR_FAIL - failed to read data from the device
        */
        int ReadMMCustomData(uint8_t* buffer);

        /** Write motion module custom data buffer to device
        *  @param buffer: user allocated buffer in size of DC_MM_CUSTOM_DATA_SIZE  (504 bytes) that contains custom data to be written to device
        *  @return When DC_MM_SUCCESS then data written uccessfully
        *          DC_MM_ERROR_NOT_INITIALIZED - library not initialized
        *          DC_MM_ERROR_INVALID_BUFFER - invalid buffer
        *          DC_MM_ERROR_FAIL - failed to write data to device
        */
        int WriteMMCustomData(uint8_t* buffer);

    private:
        void *m_Rs400Device;

        bool HwMonitorCmd_Get(void* rs400Dev, uint8_t* cmd, uint8_t* data, int length);
        bool HwMonitorCmd_Set(void* rs400Dev, uint8_t* cmd, uint8_t* data, int length);
    };
}

#endif //_D400_MM_CALIBRATION_DATA_RW_API_H_
