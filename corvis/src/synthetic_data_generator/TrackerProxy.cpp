#include <thread>
#include <mutex>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include "rc_compat.h"
#include "TrackerProxy.h"
#include <fstream>

rc_ExtendedCameraIntrinsics::rc_ExtendedCameraIntrinsics(uint8_t uIndex, const rc_ImageFormat& fmt, const rc_CameraIntrinsics& ex)
{
    index = uIndex;
    format = fmt;
    cameraintrinsics = ex;
}

TrackerProxy::TrackerProxy()
{
    m_spTracker = std::shared_ptr<rc_Tracker>(rc_create(), rc_destroy);
}

bool TrackerProxy::ReadFile(std::string filename, std::string * const contents) const
{
    std::ifstream f(filename);
    if (!f.is_open())
        return false;
    (*contents).assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return true;
}

std::shared_ptr<rc_Tracker> TrackerProxy::getTracker() const
{
    return m_spTracker;
}

int TrackerProxy::ReadCalibrationFile(std::string inputFile, std::string * const pFileContents)
{
    int res = 1;
    int i = 0;
    int j = 0;
    std::string contents;
    const char *pJson = nullptr;
    rc_Extrinsics rce = {};
    rc_AccelerometerIntrinsics rca = {};
    rc_GyroscopeIntrinsics rcg = {};
    rc_ThermometerIntrinsics rct = {};

    ReadFile(inputFile, &contents);
    if (!rc_setCalibration(m_spTracker.get(), contents.c_str()))
    { 
        std::cout << "rc_setCalibration failed." << std::endl;
        goto END;
    }

    if (0 != (res = DescribeCameraHelper(rc_FORMAT_GRAY8)) )
    {
        goto END;
    }

    if (0 != (res = DescribeCameraHelper(rc_FORMAT_DEPTH16)))
    {
        goto END;
    }

    for (i = 0, j = 0; rc_describeAccelerometer(m_spTracker.get(), i, &rce, &rca); i++)
    { 
        if (!rc_configureAccelerometer(m_spTracker.get(), j++, &rce, &rca))
        {
            std::cout << "rc_configureAccelerometer failed. i:" << i << ".j:" << j << std::endl;
            goto END;
        }
        m_ex.push_back(rce);
        m_ain.push_back(rca);
    }

    memset(&rce, 0, sizeof(rc_Extrinsics));
    for (i = 0, j = 0; rc_describeGyroscope(m_spTracker.get(), i, &rce, &rcg); i++) 
    { 
        if (!rc_configureGyroscope(m_spTracker.get(), j++, &rce, &rcg))
        {
            std::cout << "rc_configureGyroscope failed. i:" << i << ".j:" << j << std::endl;
            goto END;
        }
        m_ex.push_back(rce);
        m_win.push_back(rcg);
    }

    memset(&rce, 0, sizeof(rc_Extrinsics));
    for (i = 0, j = 0; rc_describeThermometer(m_spTracker.get(), i, &rce, &rct); i++) 
    { 
        if (!rc_configureThermometer(m_spTracker.get(), j++, &rce, &rct))
        {
            std::cout << "rc_configureThermometer failed. i:" << i << ".j:" << j << std::endl;
            goto END;
        }
        m_ex.push_back(rce);
        m_tin.push_back(rct);
    }

    if(0 == rc_getCalibration(m_spTracker.get(), &pJson)) 
    { 
        std::cout << "rc_getCalibration failed." << std::endl; 
        goto END;
    }
    *pFileContents = pJson;
    res = 0;
END:
    return res;
}

int TrackerProxy::DescribeCameraHelper(rc_ImageFormat format)
{
    int res = 0;
    int i = 0;
    int j = 0;
    rc_Extrinsics rce = {};
    rc_CameraIntrinsics rcc;
    memset(&rcc, 0, sizeof(rc_CameraIntrinsics));

    for (i = 0, j = 0; rc_describeCamera(m_spTracker.get(), i, format, &rce, &rcc); i++)
    {
        if (!rc_configureCamera(m_spTracker.get(), j++, format, &rce, &rcc))
        {
            std::cout << "rc_configureCamera failed. i:" << i << ".j:" << j << ".Format:" << format << std::endl;
            res = 1;
        }

        m_ex.push_back(rce);
        m_cin.push_back(rc_ExtendedCameraIntrinsics(i,format,rcc));
    }
    return res;
}

rc_Extrinsics operator*(rc_Pose a, rc_Extrinsics b)
{
    transformation a_b = to_transformation(a) * to_transformation(b.pose_m);
    b.pose_m = to_rc_Pose(a_b);
    return b;
}

std::vector<rc_Extrinsics> TrackerProxy::getCamerasExtrinsincs() const
{
    return m_ex;
}

std::vector<rc_ExtendedCameraIntrinsics> TrackerProxy::getCamerasIntrincs() const
{
    return m_cin;

}

std::vector<rc_GyroscopeIntrinsics> TrackerProxy::getGyroscopeIntrincs() const
{
    return m_win;
}

std::vector<rc_ThermometerIntrinsics> TrackerProxy::getThermometerIntrinsics() const
{
    return m_tin;
}

std::vector<rc_AccelerometerIntrinsics> TrackerProxy::getAccelerometerIntrinsics() const
{
    return m_ain;
}

