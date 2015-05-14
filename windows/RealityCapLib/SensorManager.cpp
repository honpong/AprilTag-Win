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

typedef struct { int metaDataID; char sensorName[20]; PXCCapture::Device::Property deviceProperty; float fMinReportInterval; } pxcIMUsensors;
pxcIMUsensors selectedSensors[] = {
	{ METADATA_IMU_LINEAR_ACCELERATION, "LinAccel", static_cast<PXCCapture::Device::Property>(PROPERTY_SENSORS_LINEAR_ACCELERATION), 0 },
	{ METADATA_IMU_ANGULAR_VELOCITY,    "AngVel",   static_cast<PXCCapture::Device::Property>(PROPERTY_SENSORS_ANGULAR_VELOCITY),    0 }
	//	{METADATA_IMU_TILT,				   "Tilt",     static_cast<PXCCapture::Device::Property>(PROPERTY_SENSORS_TILT),                0},
	//	{METADATA_IMU_GRAVITY,	           "Gravity",  static_cast<PXCCapture::Device::Property>(PROPERTY_SENSORS_GRAVITY),             0}
};
const int NUM_OF_REQUESTED_SENSORS = sizeof(selectedSensors) / sizeof(pxcIMUsensors);

SensorManager::SensorManager() : _isVideoStreaming(false)
{
	senseMan = PXCSenseManager::CreateInstance();
	if (!senseMan) Debug::Log(L"Unable to create the PXCSenseManager\n");
}

SensorManager::~SensorManager()
{
	senseMan->Release();
}

bool SensorManager::StartSensors()
{
	if (isVideoStreaming()) return true;
	if (!senseMan) return false;

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

	status = senseMan->EnableStreams(&desc);

	if (status < PXC_STATUS_NO_ERROR)
	{
		Debug::Log(L"Failed to enable stream(s)\n");
		return false;
	}

	status = senseMan->Init();
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

void SensorManager::StopSensors()
{
	if (!isVideoStreaming()) return;
	_isVideoStreaming = false;
	videoThread.join();
	if (senseMan) senseMan->Close();
}

bool SensorManager::isVideoStreaming()
{
	return _isVideoStreaming;
}

void SensorManager::PollForFrames()
{
	imu_sample_t samples[NUM_OF_REQUESTED_SENSORS][IMU_RING_BUFFER_SAMPLE_COUNT];
	__int64 last_timestamp[NUM_OF_REQUESTED_SENSORS] = {};

	while (isVideoStreaming() && senseMan->AcquireFrame(true) == PXC_STATUS_NO_ERROR)
	{
		PXCCapture::Sample *cameraSample = senseMan->QuerySample();
		PXCImage* depthImage = cameraSample->depth;
		PXCImage* colorImage = cameraSample->color;

		Debug::Log(L"%lli color sample\n", colorImage->QueryTimeStamp() - 6370); // Taking care of 637 Micro seconds blank interval-> 637000 nanoseconds -> 6370 (one hundred nanoseconds)

		//if (sample) sampleHandler->OnNewSample(senseMan->CUID, cameraFrame);

		// Get the IMU data for each sensor type
		PXCMetadata* metadata = (PXCMetadata *)depthImage->QueryInstance(PXCMetadata::CUID);
		for (int sensorNum = 0; sensorNum < NUM_OF_REQUESTED_SENSORS; sensorNum++) 
		{
			pxcStatus result = metadata->QueryBuffer(selectedSensors[sensorNum].metaDataID, (pxcBYTE*)samples[sensorNum], IMU_RING_BUFFER_SAMPLE_COUNT*sizeof(imu_sample_t));
			if (result >= PXC_STATUS_NO_ERROR)
			{
				// Look for new samples
				int sampleNum;
				for (sampleNum = 0; sampleNum < IMU_RING_BUFFER_SAMPLE_COUNT; sampleNum++)
				{
					// break when we find the first old sample
					if (samples[sensorNum][sampleNum].coordinatedUniversalTime100ns == last_timestamp[sensorNum]) break;
				}
				// sampleNum now represents the number of new samples available
				for (; sampleNum--;)
				{
					if (samples[sensorNum][sampleNum].coordinatedUniversalTime100ns) // If the sample is valid
					{
						Debug::Log(L"%lli %s %0.3f, %0.3f, %0.3f\n", samples[sensorNum][sampleNum].coordinatedUniversalTime100ns, selectedSensors[sensorNum].sensorName, samples[sensorNum][sampleNum].data[0], samples[sensorNum][sampleNum].data[1], samples[sensorNum][sampleNum].data[2]);
						
						// Update the last_timestamp for this sensor type
						last_timestamp[sensorNum] = samples[sensorNum][sampleNum].coordinatedUniversalTime100ns;
					}
				}
			}
			else
			{
				Debug::Log(L"%s data not available\n", selectedSensors[sensorNum].sensorName);
			}
		}

		senseMan->ReleaseFrame();
	}
}

