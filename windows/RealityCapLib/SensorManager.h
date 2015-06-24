#pragma once

#include "stdafx.h"
#include <thread>
#include <atomic>
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
        virtual void OnColorFrame(PXCImage* colorImage) {}; // TODO: is this still needed?
		virtual void OnColorFrameWithDepth(PXCImage* colorImage, PXCImage * depthImage) {};
        virtual void OnAmeterSample(imu_sample_t* sample) {};
        virtual void OnGyroSample(imu_sample_t* sample) {};
    };

    class SensorManager : SensorDataReceiver
    {
    public:
        SensorManager(PXCSenseManager* senseMan);
        virtual ~SensorManager();
        bool StartSensors(); // returns true if sensors started successfully
        void StopSensors();
        bool StartPlayback(const wchar_t *filename, bool realtime);
        bool StartRecording(const wchar_t *absFileName);
        bool isVideoStreaming();
        void WaitUntilFinished();
        void SetReceiver(SensorDataReceiver* sensorReceiver) { _sensorReceiver = sensorReceiver; };
        PXCSenseManager* GetSenseManager() {  return _senseMan; };

    protected:
        virtual void OnColorFrame(PXCImage* colorImage) override;
		virtual void OnColorFrameWithDepth(PXCImage* colorImage, PXCImage * depthImage) override;
        virtual void OnAmeterSample(imu_sample_t* sample) override;
        virtual void OnGyroSample(imu_sample_t* sample) override;

    private:
        PXCSenseManager* _senseMan;
        std::thread videoThread;
        std::atomic<bool> _isVideoStreaming;
        SensorDataReceiver* _sensorReceiver;

        void PollForFrames();
    };
}