#include "stdafx.h"
#include "SensorManager.h"
#include "Debug.h"
#include "pxcsensemanager.h"
#include "pxcmetadata.h"
#include "libpxcimu_internal.h"

using namespace RealityCap;

static const int DEPTH_IMAGE_WIDTH = 320;
static const int DEPTH_IMAGE_HEIGHT = 240;
static const int COLOR_IMAGE_WIDTH = 640;
static const int COLOR_IMAGE_HEIGHT = 480;
static const int FPS = 30;

typedef struct { int metaDataID; char sensorName[20]; PXCCapture::Device::Property deviceProperty; float fMinReportInterval; } pxcIMUsensor;
pxcIMUsensor selectedSensors[] = {
    { METADATA_IMU_LINEAR_ACCELERATION, "LinAccel", static_cast<PXCCapture::Device::Property>(PROPERTY_SENSORS_LINEAR_ACCELERATION), 0 },
    { METADATA_IMU_ANGULAR_VELOCITY,    "AngVel",   static_cast<PXCCapture::Device::Property>(PROPERTY_SENSORS_ANGULAR_VELOCITY),    0 }
    //	{METADATA_IMU_TILT,				   "Tilt",     static_cast<PXCCapture::Device::Property>(PROPERTY_SENSORS_TILT),                0},
    //	{METADATA_IMU_GRAVITY,	           "Gravity",  static_cast<PXCCapture::Device::Property>(PROPERTY_SENSORS_GRAVITY),             0}
};
const int NUM_OF_REQUESTED_SENSORS = sizeof(selectedSensors) / sizeof(pxcIMUsensor);

SensorManager::SensorManager(PXCSenseManager* senseMan) : _isVideoStreaming(false), _sensorReceiver(NULL)
{
    _senseMan = senseMan;
}

SensorManager::~SensorManager()
{
    StopSensors();
}

bool SensorManager::StartSensors()
{
    if (isVideoStreaming()) return true;
    if (!_senseMan) return false;

    pxcStatus status;

    PXCVideoModule::DataDesc desc = { 0 };
    desc.streams.depth.sizeMin.width = DEPTH_IMAGE_WIDTH;
    desc.streams.depth.sizeMin.height = DEPTH_IMAGE_HEIGHT;
    desc.streams.depth.frameRate.min = FPS;

    desc.streams.depth.sizeMax.width = DEPTH_IMAGE_WIDTH;
    desc.streams.depth.sizeMax.height = DEPTH_IMAGE_HEIGHT;
    desc.streams.depth.frameRate.max = FPS;

    desc.streams.color.sizeMin.width = COLOR_IMAGE_WIDTH;
    desc.streams.color.sizeMin.height = COLOR_IMAGE_HEIGHT;
    desc.streams.color.frameRate.min = FPS;

    desc.streams.color.sizeMax.width = COLOR_IMAGE_WIDTH;
    desc.streams.color.sizeMax.height = COLOR_IMAGE_HEIGHT;
    desc.streams.color.frameRate.max = FPS;

    for (int i = 0; i < NUM_OF_REQUESTED_SENSORS; ++i)
    {
        desc.devCaps[i].label = selectedSensors[i].deviceProperty;
        desc.devCaps[i].value = selectedSensors[i].fMinReportInterval;
    }

    status = _senseMan->EnableStreams(&desc);

    if (status < PXC_STATUS_NO_ERROR)
    {
        Debug::Log(L"Failed to enable stream(s)\n");
        return false;
    }

    status = _senseMan->Init();
    if (status < PXC_STATUS_NO_ERROR)
    {
        Debug::Log(L"Failed to initialize video pipeline\n");
        return false;
    }

    _isVideoStreaming = true;

    // poll for frames in a separate thread
    videoThread = std::thread(&SensorManager::PollForFrames, this);

    return true;
}

bool SensorManager::StartRecording(const wchar_t *absFileName)
{
    PXCCaptureManager *captureMgr = _senseMan->QueryCaptureManager();
    auto status = captureMgr->SetFileName(absFileName, true);
    if (status < PXC_STATUS_NO_ERROR)
    {
        Debug::Log(L"Failed to set recording file name\n");
        return false;
    }
    if (!StartSensors()) return false;
    return true;
}

bool SensorManager::StartPlayback(const wchar_t *filename, bool realtime)
{
    if (_isVideoStreaming) return false;
    PXCCaptureManager *captureMgr = _senseMan->QueryCaptureManager();
    auto status = captureMgr->SetFileName(filename, false);
    if (status < PXC_STATUS_NO_ERROR)
    {
        Debug::Log(L"Failed to set playback file name\n");
        return false;
    }

    captureMgr->SetRealtime(realtime);

    PXCVideoModule::DataDesc desc = { 0 };
    captureMgr->QueryCapture()->QueryDeviceInfo(0, &desc.deviceInfo);
    status = _senseMan->EnableStreams(&desc);

    if (status < PXC_STATUS_NO_ERROR)
    {
        Debug::Log(L"Failed to enable stream(s)\n");
        return false;
    }

    status = _senseMan->Init();
    if (status < PXC_STATUS_NO_ERROR)
    {
        Debug::Log(L"Failed to initialize video pipeline\n");
        return false;
    }

    _isVideoStreaming = true;

    // poll for frames in a separate thread
    videoThread = std::thread(&SensorManager::PollForFrames, this);

    return true;
}

void SensorManager::WaitUntilFinished()
{
    if (!isVideoStreaming()) return;
    if (videoThread.joinable())
        videoThread.join();
}

void SensorManager::StopSensors()
{
    if (!isVideoStreaming()) return;
    _isVideoStreaming = false;
    if (videoThread.joinable())
        videoThread.join();
}

bool SensorManager::isVideoStreaming()
{
    return _isVideoStreaming;
}

int CountNewSamples(imu_sample_t samples[], __int64 last_timestamp)
{
    int sampleNum = 0;
    for (; sampleNum < IMU_RING_BUFFER_SAMPLE_COUNT; sampleNum++)
    {
        // break when we find the first old sample
        imu_sample_t sample = samples[sampleNum];
        if (sample.coordinatedUniversalTime100ns == last_timestamp) break;
    }
    return sampleNum;
}

// returns the number of new samples fetched
int FetchNewSamplesForSensor(pxcIMUsensor* sensor, PXCImage* depthImage, imu_sample_t samples[], __int64 last_timestamp)
{
    PXCMetadata* metadata = (PXCMetadata *)depthImage->QueryInstance(PXCMetadata::CUID);

    // fill buffer with samples
    pxcStatus result = metadata->QueryBuffer(sensor->metaDataID, (pxcBYTE*)samples, IMU_RING_BUFFER_SAMPLE_COUNT*sizeof(imu_sample_t));

    if (result >= PXC_STATUS_NO_ERROR)
    {
        // Look for new samples in the buffer
        return CountNewSamples(samples, last_timestamp);
    }
    else
    {
        return 0;
        Debug::Log(L"%s data not available", sensor->sensorName);
    }
}

void SensorManager::PollForFrames()
{
    imu_sample_t sampleBuffer[NUM_OF_REQUESTED_SENSORS][IMU_RING_BUFFER_SAMPLE_COUNT];
    __int64 last_timestamp[NUM_OF_REQUESTED_SENSORS];

    while (isVideoStreaming() && _senseMan->AcquireFrame(true) == PXC_STATUS_NO_ERROR)
    {
        PXCCapture::Sample *cameraSample = _senseMan->QuerySample();
        PXCImage* depthImage = cameraSample->depth;
        PXCImage* colorImage = cameraSample->color;

        // process the IMU data for each sensor type        
        for (int sensorNum = 0; sensorNum < NUM_OF_REQUESTED_SENSORS; sensorNum++)
        {
            pxcIMUsensor* sensor = &selectedSensors[sensorNum];
            const int newSampleCount = FetchNewSamplesForSensor(sensor, depthImage, sampleBuffer[sensorNum], last_timestamp[sensorNum]);

            for (int sampleNum = newSampleCount - 1; sampleNum >= 0; sampleNum--)
            {
                imu_sample_t sample = sampleBuffer[sensorNum][sampleNum];
                if (sample.coordinatedUniversalTime100ns) // If the sample is valid
                {
                    if (sensor->deviceProperty == PROPERTY_SENSORS_LINEAR_ACCELERATION)
                    {
                        OnAmeterSample(&sample);
                    }
                    else if (sensor->deviceProperty == PROPERTY_SENSORS_ANGULAR_VELOCITY)
                    {
                        OnGyroSample(&sample);
                    }

                    // Update the last_timestamp for this sensor type
                    last_timestamp[sensorNum] = sample.coordinatedUniversalTime100ns;
                }
            }
        }

        //Always pass image data AFTER associated inertial data. This will trigger the update
        OnColorFrame(colorImage);
		OnColorFrameWithDepth(colorImage, depthImage);

        _senseMan->ReleaseFrame();
    }

    _isVideoStreaming = false; // in case we're replaying and we reach EoF
    _senseMan->Close();
    Debug::Log(L"Exiting sensor polling thread");
}

void SensorManager::OnColorFrame(PXCImage * colorImage)
{
    if (_sensorReceiver) _sensorReceiver->OnColorFrame(colorImage);
    Debug::Log(L"%lli color sample", colorImage->QueryTimeStamp() - 6370); // Taking care of 637 Micro seconds blank interval-> 637000 nanoseconds -> 6370 (one hundred nanoseconds)
}

void SensorManager::OnColorFrameWithDepth(PXCImage* colorImage, PXCImage * depthImage)
{
	if (_sensorReceiver) _sensorReceiver->OnColorFrameWithDepth(colorImage, depthImage);
	Debug::Log(L"%lli color with depth sample", depthImage->QueryTimeStamp() - 6370); // Taking care of 637 Micro seconds blank interval-> 637000 nanoseconds -> 6370 (one hundred nanoseconds)
}

void SensorManager::OnAmeterSample(imu_sample_t* sample)
{
    if (_sensorReceiver) _sensorReceiver->OnAmeterSample(sample);
    Debug::Log(L"%lli %s\t%0.3f, %0.3f, %0.3f", sample->coordinatedUniversalTime100ns, L"accel", sample->data[0], sample->data[1], sample->data[2]);
}

void SensorManager::OnGyroSample(imu_sample_t* sample)
{
    if (_sensorReceiver) _sensorReceiver->OnGyroSample(sample);
    Debug::Log(L"%lli %s\t%0.3f, %0.3f, %0.3f", sample->coordinatedUniversalTime100ns, L"gyro", sample->data[0], sample->data[1], sample->data[2]);
}

