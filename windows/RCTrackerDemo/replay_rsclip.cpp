#define _USE_MATH_DEFINES

#include "rc_intel_interface.h"

#include <iostream>
#include <fstream>
#include <string.h>
#include <assert.h>
#include <sstream>
#include <memory>
#include <conio.h>
#include "libpxcimu_internal.h"
#include "pxcsensemanager.h"
#include "stats.h"

using namespace std;

typedef struct { int metaDataID; char sensorName[20]; PXCCapture::Device::Property deviceProperty; float fMinReportInterval; } pxcIMUsensor;

pxcIMUsensor selectedSensors[] = {
    { METADATA_IMU_LINEAR_ACCELERATION, "LinAccel", static_cast<PXCCapture::Device::Property>(PROPERTY_SENSORS_LINEAR_ACCELERATION), 0 },
    { METADATA_IMU_ANGULAR_VELOCITY,    "AngVel",   static_cast<PXCCapture::Device::Property>(PROPERTY_SENSORS_ANGULAR_VELOCITY),    0 },
    //	{METADATA_IMU_TILT,				   "Tilt",     static_cast<PXCCapture::Device::Property>(PROPERTY_SENSORS_TILT),                0},
    //	{METADATA_IMU_GRAVITY,	           "Gravity",  static_cast<PXCCapture::Device::Property>(PROPERTY_SENSORS_GRAVITY),             0}
};

const int NUM_OF_REQUESTED_SENSORS = sizeof(selectedSensors) / sizeof(pxcIMUsensor);
bool verbose = false;
bool run_tracker = true;
pxcCHAR     *m_recordedFile = L"capture.rssdk";
pxcBool        m_bRecord = 0;

const rc_Pose rc_pose_identity = {1, 0, 0, 0, 
                                  0, 1, 0, 0,
                                  0, 0, 1, 0}; 

const rc_Pose camera_pose      = { 0,-1,  0, 0.064f,
                                  -1, 0,  0, -0.017f,
                                   0, 0, -1, 0}; 


void logger(void * handle, const char * buffer_utf8, size_t length)
{
    fprintf((FILE *)handle, "%s\n", buffer_utf8);
}

int main(int c, char **v)
{
    if(c < 1)
    {
        fprintf(stderr, "specify file name\n");
        return 0;
    }
    const char *name = v[1];
    int width_px = 640;
    int height_px = 480;
    float center_x_px = (width_px-1) / 2.f;
    float center_y_px = (height_px-1) / 2.f;
    bool force_recognition = false;
    double a_bias_stdev = .02 * 9.8 / 2.;
    double w_bias_stdev = 10. / 180. * M_PI / 2.;

    rc_Vector bias_m__s2 = {0.f, 0.f, 0.f};
    float noiseVariance_m2__s4 = a_bias_stdev * a_bias_stdev;
    rc_Vector bias_rad__s = {0.f, 0.f, 0.f};
    float noiseVariance_rad2__s2 = w_bias_stdev * w_bias_stdev;
    rc_Camera camera = rc_EGRAY8;
    int focal_length_x_px = 627;
    int focal_length_y_px = 627;
    int focal_length_xy_px = 0;
    uint64_t shutter_time_100_ns = 333330;


    rc_Tracker * tracker = rc_create();
    rc_setLog(tracker, logger, false, 0, stderr);
    rc_configureCamera(tracker, camera, camera_pose, width_px, height_px, center_x_px, center_y_px, focal_length_x_px, focal_length_xy_px, focal_length_y_px);
    rc_configureAccelerometer(tracker, rc_pose_identity, bias_m__s2, noiseVariance_m2__s4);
    rc_configureGyroscope(tracker, rc_pose_identity, bias_rad__s, noiseVariance_rad2__s2);
    rc_reset(tracker, 0, rc_pose_identity);

    if(run_tracker) rc_startTracker(tracker);

    bool first_packet = false;
    bool is_running = true;
    int packets_dispatched = 0;

    PXCSenseManager* pSenseManager = PXCSenseManager::CreateInstance();
    if (pSenseManager == NULL)
    {
        wcout << L"Sense Manager Failed" << endl;
        return -1;
    }
    pxcStatus result;
    //live mode with or w/o recording
    if (m_recordedFile == NULL || (m_recordedFile != NULL && m_bRecord))
    {
        result = pSenseManager->QuerySession()->SetCoordinateSystem(PXCSession::COORDINATE_SYSTEM_REAR_OPENCV);
        if (m_bRecord)
        {
            PXCCaptureManager *captureMgr = pSenseManager->QueryCaptureManager();
            result = captureMgr->SetFileName(m_recordedFile, m_bRecord);
        }
        PXCVideoModule::DataDesc desc = { 0 };
        desc.streams.color.sizeMin.width = 640;
        desc.streams.color.sizeMin.height = 480;
        desc.streams.color.frameRate.min = 30;

        desc.streams.depth.sizeMax.width = 640;
        desc.streams.depth.sizeMax.height = 480;
        desc.streams.depth.frameRate.max = 30;

        for (int i = 0; i < NUM_OF_REQUESTED_SENSORS; ++i)
        {
            desc.devCaps[i].label = selectedSensors[i].deviceProperty;
            desc.devCaps[i].value = selectedSensors[i].fMinReportInterval;
        }
        result = pSenseManager->EnableStreams(&desc);
        if (result < PXC_STATUS_NO_ERROR)
        {
            wcout << L"Enable Stream Failed" << endl;
            return -1;
        }
    }
    else
    {
        //playback mode
        PXCCaptureManager *captureMgr = pSenseManager->QueryCaptureManager();
        result = captureMgr->SetFileName(m_recordedFile, 0);
        captureMgr->SetRealtime(0);

        PXCVideoModule::DataDesc desc = { 0 };
        captureMgr->QueryCapture()->QueryDeviceInfo(0, &desc.deviceInfo);
        pSenseManager->EnableStreams(&desc);
    }
    result = pSenseManager->Init();

    if (result < PXC_STATUS_NO_ERROR)
    {
        wcout << L"Test aborted due to pSenseManager->Init() failure: " << result << endl;
        return -1;
    }

    imu_sample_t samples[NUM_OF_REQUESTED_SENSORS][IMU_RING_BUFFER_SAMPLE_COUNT];
    // Keep track of the lastest IMU sample for each sample type
    __int64 last_timestamp[NUM_OF_REQUESTED_SENSORS] = {};

    float_stats stats[NUM_OF_REQUESTED_SENSORS];
    int frame = 0;
    // Enter the main loop
    wcout << L"Running...\n";
    while (pSenseManager->AcquireFrame(true) == PXC_STATUS_NO_ERROR) //this will fail if all you been through all samples in playback mode or if camera is unplugged
    {
        PXCCapture::Sample* pSample = pSenseManager->QuerySample();
        if (!pSample || !pSample->depth || !pSample->color)
            break;
        ++frame;
        PXCImage* depth = pSample->depth;

        // Report the frame number and timestamp
        if (verbose)
            wcout << L"Depth frame " << frame << "           (ts=" << depth->QueryTimeStamp() - 6370 << ")" << endl; //Taking care of 637 Micro seconds blank interval-> 637000 nanoseconds -> 6370 (one hundred nanoseconds)

        PXCImage* depthImage = pSample->depth;
        PXCImage* colorImage = pSample->color;
        PXCImage::ImageData data;
        pxcStatus result = colorImage->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::PIXEL_FORMAT_Y8, &data);
        if (result != PXC_STATUS_NO_ERROR || !data.planes[0]) throw std::runtime_error("PXCImage->AcquireAccess failed!");
        uint8_t *image = data.planes[0];
        int stride = data.pitches[0];
        if(run_tracker) rc_receiveImage(tracker, rc_EGRAY8, colorImage->QueryTimeStamp() - 6370 - 333330, 333330, NULL, false, stride, image);
        
        // Get the IMU data for each sensor type
        PXCMetadata* metadata = (PXCMetadata *)depth->QueryInstance(PXCMetadata::CUID);
        // process the IMU data for each sensor type        
        for (int sensor = 0; sensor < NUM_OF_REQUESTED_SENSORS; ++sensor)
        {
            pxcIMUsensor* inertial_sensor = &selectedSensors[sensor];
            pxcStatus result = metadata->QueryBuffer(selectedSensors[sensor].metaDataID, (pxcBYTE*)samples[sensor],
                IMU_RING_BUFFER_SAMPLE_COUNT*sizeof(imu_sample_t));
            if (result >= PXC_STATUS_NO_ERROR)
            {
                // Look for new samples
                int sample;
                for (sample = 0; sample < IMU_RING_BUFFER_SAMPLE_COUNT; sample++)
                {
                    if (samples[sensor][sample].coordinatedUniversalTime100ns == last_timestamp[sensor]) break;
                } for (; sample--;)
                {
                    if (samples[sensor][sample].coordinatedUniversalTime100ns) // If the sample is valid
                    {
                        imu_sample_t *s = &samples[sensor][sample];
                        // Report the new sample
                        if (verbose)
                            wcout << L"Depth frame " << frame << L" (" <<
                            selectedSensors[sensor].sensorName << ", ts=" << s->coordinatedUniversalTime100ns
                            << ") " << s->data[0] << ", " << s->data[1] << ", " <<
                            s->data[2] << endl;

                        // If this isn't the first sample of this sensor type...
                        if (last_timestamp[sensor])
                        {
                            // ...accumulate the report interval measurement into the statistics
                            stats[sensor].add_sample((float)
                                (samples[sensor][sample].coordinatedUniversalTime100ns - last_timestamp[sensor]) / 10000.0f);
                        }

                        // Update the last_timestamp for this sensor type
                        last_timestamp[sensor] = samples[sensor][sample].coordinatedUniversalTime100ns;

                        if (inertial_sensor->deviceProperty == PROPERTY_SENSORS_LINEAR_ACCELERATION)
                        {
                            //windows gives acceleration in g-units, so multiply by standard gravity in m/s^2
                            rc_Vector acceleration_m__s2;
                            acceleration_m__s2.x = -s->data[1] * 9.80665;
                            acceleration_m__s2.y = s->data[0] * 9.80665;
                            acceleration_m__s2.z = -s->data[2] * 9.80665;
                            if(run_tracker) rc_receiveAccelerometer(tracker, s->coordinatedUniversalTime100ns, acceleration_m__s2);
                        }
                        else if (inertial_sensor->deviceProperty == PROPERTY_SENSORS_ANGULAR_VELOCITY)
                        {
                            rc_Vector angular_velocity_rad__s;
                            //windows gives angular velocity in degrees per second
                            angular_velocity_rad__s.x = s->data[0] * M_PI / 180.;
                            angular_velocity_rad__s.y = s->data[1] * M_PI / 180.;
                            angular_velocity_rad__s.z = s->data[2] * M_PI / 180.;
                            if(run_tracker) rc_receiveGyro(tracker, s->coordinatedUniversalTime100ns, angular_velocity_rad__s);
                        }
                    }
                }
            }
            else
            {
                wcout << L"Depth frame " << frame << " - " << selectedSensors[sensor].sensorName << L" data not available " << endl;
            }
        }

        colorImage->ReleaseAccess(&data);

        pSenseManager->ReleaseFrame();

        packets_dispatched++;

        if(run_tracker && packets_dispatched%10 == 0) rc_triggerLog(tracker);


        if (_kbhit())
        {
            int c = _getch() & 255;
            if (c == 27 || c == 'q' || c == 'Q')
                break;
        }
    }

    pSenseManager->Close();
    pSenseManager->Release();
    if(run_tracker) rc_stopTracker(tracker);
    if(run_tracker) rc_triggerLog(tracker);

    rc_destroy(tracker);


    // Report the statistics for each sensor type
    for (int sensor = 0; sensor < NUM_OF_REQUESTED_SENSORS; sensor++)
    {
        wcout << "(" << stats[sensor].count << " " << selectedSensors[sensor].sensorName << " intervals measured at " << stats[sensor].mean << " (ms) +/- "
            << stats[sensor].coff_var()*100.0f << "%%" << endl;
    }

    wcout << L"Exiting" << endl;;

    return 0;

}
