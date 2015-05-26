#pragma once

#include "stdafx.h"
#include <thread>
#include "pxcsensemanager.h"
#include "pxcmetadata.h"
#include "libpxcimu_internal.h"


namespace RealityCap
{
    class SensorDataReceiver
    {
    protected:
        SensorDataReceiver() {};

    public:
        virtual void OnColorFrame(PXCImage* colorImage) {};
        virtual void OnAmeterSample(imu_sample_t* sample) {};
        virtual void OnGyroSample(imu_sample_t* sample) {};
    };

    class SensorManager : SensorDataReceiver
    {
    public:
        SensorManager(PXCSenseManager* senseMan);
        ~SensorManager();
        bool StartSensors(); // returns true if sensors started successfully
        void StopSensors();
        bool StartPlayback(const wchar_t *filename);
        bool SetRecording(const wchar_t *filename);
        bool isVideoStreaming();
        void SetReceiver(SensorDataReceiver* sensorReceiver) { _sensorReceiver = sensorReceiver; };
        PXCSenseManager* GetSenseManager() {  return _senseMan; };

    protected:
        virtual void OnColorFrame(PXCImage* colorImage) override;
        virtual void OnAmeterSample(imu_sample_t* sample) override;
        virtual void OnGyroSample(imu_sample_t* sample) override;

    private:
        PXCSenseManager* _senseMan;
        std::thread videoThread;
        bool _isVideoStreaming;
        SensorDataReceiver* _sensorReceiver;

        void PollForFrames();
    };
}