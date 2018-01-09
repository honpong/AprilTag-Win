#ifndef  TRACKERPROXY_H
#define  TRACKERPROXY_H

#include "rc_tracker.h"

struct rc_ExtendedCameraIntrinsics
{
    uint8_t index;
    rc_ImageFormat format;
    rc_CameraIntrinsics  cameraintrinsics;
    rc_ExtendedCameraIntrinsics(uint8_t uIndex, const rc_ImageFormat& fmt, const rc_CameraIntrinsics& ex);
};

class TrackerProxy
{
public:
    TrackerProxy();
    size_t ReadCalibrationFile(std::string inputFile, std::string * const fileContents);
    std::shared_ptr<rc_Tracker> getTracker() const;
    std::vector<rc_Extrinsics> getCamerasExtrinsincs() const;
    std::vector<rc_ExtendedCameraIntrinsics> getCamerasIntrincs() const;
    std::vector<rc_GyroscopeIntrinsics> getGyroscopeIntrincs() const;
    std::vector<rc_ThermometerIntrinsics> getThermometerIntrinsics() const;
    std::vector<rc_AccelerometerIntrinsics> getAccelerometerIntrinsics() const;

private:
    bool ReadFile(std::string filename, std::string* const contents) const;
    size_t DescribeCameraHelper(rc_ImageFormat format);

    std::vector<rc_Extrinsics> m_ex;
    std::vector<rc_ExtendedCameraIntrinsics> m_cin;
    std::vector<rc_GyroscopeIntrinsics> m_win;
    std::vector<rc_ThermometerIntrinsics> m_tin;
    std::vector<rc_AccelerometerIntrinsics> m_ain;
    std::shared_ptr<rc_Tracker> m_spTracker;
};

#endif // !TRACKERPROXY_H