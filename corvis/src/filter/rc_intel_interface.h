//
//  rc_intel_interface.h
//
//  Created by Eagle Jones on 1/29/15.
//  Copyright (c) 2015 Realitycap. All rights reserved.
//

#ifndef rc_intel_interface_h
#define rc_intel_interface_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

typedef enum {
    rc_EGRAY8,
    rc_EDEPTH16,
} rc_Camera;
    

typedef enum
{
    /** rc_Tracker is inactive. */
    rc_E_INACTIVE = 0,
    /** rc_Tracker is in static calibration mode. The device should not be moved or touched. */
    rc_E_STATIC_CALIBRATION = 1,
    /** startSensorFusionWithDevice: has been called, and rc_Tracker is in the handheld steady initialization phase. */
    rc_E_STEADY_INITIALIZATION = 2,
    /** startSensorFusionUnstableWithDevice: has been called, and rc_Tracker is in the handheld dynamic initialization phase. */
    rc_E_DYNAMIC_INITIALIZATION = 3,
    /** rc_Tracker is active and updates are being provided with all data. */
    rc_E_RUNNING = 4,
    /** rc_Tracker is in handheld portrait calibration mode. The device should be held steady in portrait orientation, perpendicular to the floor. */
    rc_E_PORTRAIT_CALIBRATION = 5,
    /** rc_Tracker is in handheld landscape calibration mode. The device should be held steady in landscape orientation, perpendicular to the floor. */
    rc_E_LANDSCAPE_CALIBRAITON = 6
} rc_TrackerState;

typedef enum
{
    /** No error has occurred. */
    rc_E_ERROR_NONE = 0,
    /** No visual features were detected in the most recent image. This is normal in some circumstances, such as quick motion or if the device temporarily looks at a blank wall. */
    rc_E_ERROR_VISION = 1,
    /** The device moved more rapidly than expected for typical handheld motion. This may indicate that rc_Tracker has failed and is providing invalid data. */
    rc_E_ERROR_SPEED = 2,
    /** A fatal internal error has occured. */
    rc_E_ERROR_OTHER = 3,
} rc_TrackerError;

typedef enum
{
    /** rc_Tracker is not currently running (possibly due to failure). */
    RC_E_CONFIDENCE_NONE = 0,
    /** The data has low confidence. This occurs if no visual features could be detected or tracked. */
    rc_E_CONFIDENCE_LOW = 1,
    /** The data has medium confidence. This occurs when rc_Tracker has recently been initialized, or has recovered from having few usable visual features, and continues until the user has moved sufficiently to produce reliable measurements. If the user moves too slowly or features are unreliable, this will not switch to rc_E_CONFIDENCE_HIGH, and measurements may be unreliable. */
    rc_E_CONFIDENCE_MEDIUM = 2,
    /** Sufficient visual features and motion have been detected that measurements are likely to be accurate. */
    rc_E_CONFIDENCE_HIGH = 3
} rc_TrackerConfidence;


typedef struct {
    float x,y,z;
} rc_Vector;

/**
 Flat array of 12 floats, corresponding to 3x4 transformation matrix in row-major order:
 
 [R00 R01 R02 T0]
 [R10 R11 R12 T1]
 [R20 R21 R22 T2]
 */
typedef float rc_Pose[12];

/**
 Timestamp, in nanoseconds * 100
 */
typedef int64_t rc_Timestamp;

typedef struct
{
    uint64_t id;
    rc_Vector world;
    float image_x, image_y;
} rc_Feature;

typedef struct rc_Tracker rc_Tracker;

rc_Tracker * rc_create();
void rc_destroy(rc_Tracker *tracker);

/**
 Resets system, clearing all history and state, and sets initial pose and time.
 System will be stopped until one of the rc_start_ functions is called.
 */
void rc_reset(rc_Tracker *tracker, rc_Timestamp initialTime_100_ns, const rc_Pose initialPose_m);

/**
 @param tracker The active rc_Tracker instance
 @param camera The camera to configure
 @param pose_m Position (in meters) and orientation of camera relative to reference point
 @param image_width_px Image width in pixels
 @param image_height_px Image height in pixels
 @param center_x_px Horizontal principal point of camera in pixels
 @param center_y_px Horizontal principal point of camera in pixels
 @param focal_length_px Focal length of camera in pixels
 */
void rc_configureCamera(rc_Tracker *tracker, rc_Camera camera, const rc_Pose pose_m,
                        int width_px, int height_px, float center_x_px, float center_y_px,
                        float focal_length_x_px, float focal_length_xy_px, float focal_length_y_px);
void rc_configureAccelerometer(rc_Tracker *tracker, const rc_Pose pose_m, const rc_Vector bias_m__s2, float noiseVariance_m2__s4);
void rc_configureGyroscope(rc_Tracker *tracker, const rc_Pose pose_m, const rc_Vector bias_rad__s, float noiseVariance_rad2__s2);
void rc_configureLocation(rc_Tracker *tracker, double latitude_deg, double longitude_deg, double altitude_m);

//TODO: Define calibration interface
//void rc_startCalibration(rc_Tracker *tracker);
//bool rc_getCalibration(rc_Tracker *tracker, char **buffer, int *length);

/** TODO:
 Starts processing inertial data to estimate the orientation of the device so that initialization of the full tracker can happen more quickly.
 */
//void rc_startInertialOnly(rc_Tracker *tracker);

/**
 Starts the tracker.
 */
void rc_startTracker(rc_Tracker *tracker);
void rc_stopTracker(rc_Tracker *tracker);

/**
 @param tracker The active rc_Tracker instance
 @param camera The camera from which this frame was received
 @param time_100_ns Timestamp (in 100ns) when this frame was received
 @param shutter_time Exposure time (in 100ns)
 @param poseEstimate_m Position (in meters) and orientation estimate from external tracking system
 @param force_recognition If true, force the tracker instance to perform relocalization / loop closure immediately.
 @param stride Number of bytes in each line
 @param image Image data.
 */
void rc_receiveImage(rc_Tracker *tracker, rc_Camera camera, rc_Timestamp time_100_ns, rc_Timestamp shutter_time_100_ns, const rc_Pose poseEstimate_m, bool force_recognition, int stride, const void *image);
void rc_receiveAccelerometer(rc_Tracker *tracker, rc_Timestamp time_100_ns, const rc_Vector acceleration_m__s2);
void rc_receiveGyro(rc_Tracker *tracker, rc_Timestamp time_100_ns, const rc_Vector angular_velocity_rad__s);

void rc_getPose(const rc_Tracker *tracker, rc_Pose pose_m);
int rc_getFeatures(const rc_Tracker *tracker, rc_Feature **features_px);
rc_TrackerState rc_getState(const rc_Tracker *tracker);
rc_TrackerConfidence rc_getConfidence(const rc_Tracker *tracker);
rc_TrackerError rc_getError(const rc_Tracker *tracker);

/**
 @param tracker The active rc_Tracker instance
 @param log The function to call with output
 @param stream If true, log every calculated output pose
 @param period_100_ns If non-zero, log each calculated pose when it has been period_100_ns or more since the last pose was logged
 @param handle Token to pass to log callback
 */
void rc_setLog(rc_Tracker *tracker, void (*log)(void *handle, const char *buffer_utf8, size_t length), bool stream, rc_Timestamp period_100_ns, void *handle);

/**
 Immediately outputs the last calculated pose
 */
void rc_triggerLog(const rc_Tracker *tracker);

/*
 Not yet implemented (depend on loop closure):

//Starts processing inertial data and some image data to build environment representation and enable relocalization / loop closure. Does not run full tracker; instead uses the provided poseEstimates with each image)
void rc_startMappingFromKnownPoses(rc_Tracker *tracker);
void rc_saveMap(rc_Tracker *tracker,  void (*write)(void *handle, void *buffer, size_t length), void *handle);
void rc_loadMap(rc_Tracker *tracker, size_t (*read)(void *handle, void *buffer, size_t length), void *handle);
*/

#ifdef __cplusplus
}
#endif

#endif
