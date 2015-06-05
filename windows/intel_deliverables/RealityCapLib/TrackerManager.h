#pragma once

#include "SensorManager.h"
#include <thread>

#include "rc_intel_interface.h"

namespace RealityCap
{
    class TrackerManagerDelegate
    {
    protected:
        TrackerManagerDelegate() {};
    public:
        virtual void OnProgressUpdated(float progress) {};
        virtual void OnStatusUpdated(int status) {};
        virtual void OnError(int code) {};
        virtual void OnDataUpdated(rc_Timestamp time, rc_Pose pose, rc_Feature *features, size_t feature_count) {}
    };

    class TrackerManager : virtual public SensorManager
    {
    public:
        TrackerManager(PXCSenseManager* senseMan);
        virtual ~TrackerManager();

        bool isRunning();
        void SetDelegate(TrackerManagerDelegate* del);

        bool Start();
        bool StartCalibration();
        bool StartReplay(const std::wstring filename, bool realtime);
        void Stop();

        void ConfigureCameraIntrinsics();
        bool ReadCalibration(std::wstring filename);
        bool LoadDefaultCalibration();
        bool LoadStoredCalibration();
        bool WriteCalibration(std::wstring filename);
        bool StoreCurrentCalibration();
        void SetLog(void(*log)(void * handle, const char * buffer_utf8, size_t length), void *handle)
        {
            rc_setLog(_tracker, log, true, 0, handle);
        }

        void UpdateStatus(rc_TrackerState newState, rc_TrackerError errorCode, rc_TrackerConfidence confidence, float progress);
        void UpdateData(rc_Timestamp time, rc_Pose pose, rc_Feature *features, size_t feature_count);

    protected:
        virtual void OnColorFrame(PXCImage* colorImage) override;
        virtual void OnAmeterSample(imu_sample_t* sample) override;
        virtual void OnGyroSample(imu_sample_t* sample) override;
        std::wstring GetExePath();

    private:
        bool _isRunning;
        TrackerManagerDelegate* _delegate;
        rc_Tracker* _tracker;
        int _trackerState;
        float _progress;
    };
}
