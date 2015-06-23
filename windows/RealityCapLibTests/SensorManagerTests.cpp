#include "stdafx.h"
#include "../../gmock/gtest/gtest.h"
#include "../../gmock/gmock/gmock.h"
#include "SensorManager.h"
#include "PXCSenseManagerFake.h"

using namespace RealityCap;

TEST(SensorManagerTests, NewDelete)
{
    SensorManager* sensorMan = new SensorManager(NULL);
    delete sensorMan;
}

TEST(SensorManagerTests, StartStop)
{
    PXCSenseManagerFake* senseMan = new PXCSenseManagerFake();

    SensorManager sensorMan(senseMan);
    EXPECT_FALSE(sensorMan.isVideoStreaming());

    EXPECT_TRUE(sensorMan.StartSensors());
    EXPECT_TRUE(sensorMan.isVideoStreaming());

    sensorMan.StopSensors();
    EXPECT_FALSE(sensorMan.isVideoStreaming());
}

TEST(SensorManagerTests, StartFailed)
{
    PXCSenseManagerFake* senseMan = new PXCSenseManagerFake();

    SensorManager sensorMan(senseMan);
    senseMan->fakeStatus = PXC_STATUS_ALLOC_FAILED; // causes EnableStreams to return failure
    EXPECT_FALSE(sensorMan.StartSensors());
    EXPECT_FALSE(sensorMan.isVideoStreaming());
}

TEST(SensorManagerTests, NullSenseMan)
{
    SensorManager sensorMan(NULL);
    EXPECT_FALSE(sensorMan.StartSensors());
    EXPECT_FALSE(sensorMan.isVideoStreaming());
}

class FakeReceiver : public SensorDataReceiver
{
public:
    int colorFramesReceived = 0;
    int ameterSamplesReceived = 0;
    int gyroSamplesReceived = 0;
    const int maxSamples = 3;

    virtual void OnColorFrame(PXCImage* colorFrame)
    {
        colorFramesReceived++;
    }
    virtual void OnAmeterSample(imu_sample_t * sample)
    {
        ameterSamplesReceived++;
    }
    virtual void OnGyroSample(imu_sample_t * sample)
    {
        gyroSamplesReceived++;
    }
    bool isTestComplete()
    {
        if (colorFramesReceived > 0 && ameterSamplesReceived == maxSamples && gyroSamplesReceived == maxSamples)
            return true;
        else
            return false;
    }
};

TEST(SensorManagerTests, SensorData)
{
    PXCSenseManagerFake* senseMan = new PXCSenseManagerFake();
    FakeReceiver receiver;

    SensorManager sensorMan(senseMan);
    sensorMan.SetReceiver(&receiver);
    sensorMan.StartSensors();

    int ticks = 0;
    while (!receiver.isTestComplete() && ticks < 10)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        ticks++;
    }

    sensorMan.StopSensors();

    EXPECT_LT(ticks, 10);

    delete senseMan;
}
