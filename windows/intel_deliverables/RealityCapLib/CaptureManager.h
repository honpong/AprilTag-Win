#pragma once

#include "SensorManager.h"
//#include "..\..\corvis\src\filter\capture.h"

namespace RealityCap
{
    class CaptureManager : virtual public SensorManager
    {
    public:
        CaptureManager(PXCSenseManager* senseMan);
        bool StartCapture();
        void StopCapture();
        bool isCapturing();

    private:
        bool _isCapturing;
    };
}
