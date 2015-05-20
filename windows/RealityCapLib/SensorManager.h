#pragma once

#include "stdafx.h"
#include <thread>

class PXCSenseManager;
class PXCImage;
struct imu_sample;

namespace RealityCap
{
    class SensorDataReceiver
    {
    protected:
        SensorDataReceiver() {};

    public:
        virtual void OnColorFrame(PXCImage* colorImage) {};
        virtual void OnAmeterSample(struct imu_sample* sample) {};
        virtual void OnGyroSample(struct imu_sample* sample) {};
    };

    class SensorManager : SensorDataReceiver
    {
    public:
        SensorManager(PXCSenseManager* senseMan);
        ~SensorManager();
        bool StartSensors(); // returns true if sensors started successfully
        void StopSensors();
        bool isVideoStreaming();
        void SetReceiver(SensorDataReceiver* sensorReceiver) { _sensorReceiver = sensorReceiver; };
        PXCSenseManager* GetSenseManager() {  return _senseMan; };

    protected:
        virtual void OnColorFrame(PXCImage* colorImage) override;
        virtual void OnAmeterSample(struct imu_sample* sample) override;
        virtual void OnGyroSample(struct imu_sample* sample) override;

    private:
        PXCSenseManager* _senseMan;
        std::thread videoThread;
        bool _isVideoStreaming;
        SensorDataReceiver* _sensorReceiver;

        void PollForFrames();
    };
}