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

typedef enum
{
    rc_EGRAY8,
    rc_EDEPTH16,
} rc_Camera;

typedef struct
{
    float x,y,z;
} rc_Vector;

typedef struct {
    float x,y;
    int id;
} rc_Feature;

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
void rc_destroy(rc_Tracker * tracker);

/**
 Resets system, clearing all history and state, and sets initial pose and time.
 System will be stopped until one of the rc_start_ functions is called.
 */
void rc_reset(rc_Tracker * tracker, rc_Timestamp initialTime_100_ns, const rc_Pose initialPose_m);

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
void rc_configureCamera(rc_Tracker * tracker, rc_Camera camera, const rc_Pose pose_m,
                        int width_px, int height_px, float center_x_px, float center_y_px,
                        float focal_length_x_px, float focal_length_xy_px, float focal_length_y_px);
void rc_configureAccelerometer(rc_Tracker * tracker, const rc_Pose pose_m, const rc_Vector bias_m__s2, float noiseVariance_m2__s4);
void rc_configureGyroscope(rc_Tracker * tracker, const rc_Pose pose_m, const rc_Vector bias_rad__s, float noiseVariance_rad2__s2);
void rc_configureLocation(rc_Tracker * tracker, double latitude_deg, double longitude_deg, double altitude_m);

//TODO: Define calibration interface
//void rc_startCalibration(rc_Tracker * tracker);
//bool rc_getCalibration(rc_Tracker * tracker, char **buffer, int *length);

/**
 Starts processing inertial data to estimate the orientation of the device so that initialization of the full tracker can happen more quickly.
 */
void rc_startInertialOnly(rc_Tracker * tracker);

/**
 Starts the tracker.
 */
void rc_startTracker(rc_Tracker * tracker);
    
void rc_stopTracker(rc_Tracker * tracker);
    
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
void rc_receiveImage(rc_Tracker * tracker, rc_Camera camera, rc_Timestamp time_100_ns, rc_Timestamp shutter_time_100_ns, const rc_Pose poseEstimate_m, bool force_recognition, int stride, const void *image);
void rc_receiveAccelerometer(rc_Tracker * tracker, rc_Timestamp time_100_ns, const rc_Vector acceleration_m__s2);
void rc_receiveGyro(rc_Tracker * tracker, rc_Timestamp time_100_ns, const rc_Vector angular_velocity_rad__s);

void rc_getPose(const rc_Tracker * tracker, rc_Pose pose_m);
int rc_getFeatures(const rc_Tracker * tracker, rc_Feature **features_px);

/**
 @param tracker The active rc_Tracker instance
 @param log The function to call with output
 @param stream If true, log every calculated output pose
 @param period_100_ns If non-zero, log each calculated pose when it has been period_100_ns or more since the last pose was logged
 @param handle Token to pass to log callback
 */
void rc_setLog(rc_Tracker * tracker, void (*log)(void *handle, char *buffer_utf8, size_t length), bool stream, rc_Timestamp period_100_ns, void *handle);

/**
 Immediately outputs the last calculated pose
 */
void rc_triggerLog(const rc_Tracker * tracker);

/*
 Not yet implemented (depend on loop closure):

//Starts processing inertial data and some image data to build environment representation and enable relocalization / loop closure. Does not run full tracker; instead uses the provided poseEstimates with each image)
void rc_startMappingFromKnownPoses(rc_Tracker * tracker);
void rc_saveMap(rc_Tracker * tracker,  void (*write)(void *handle, void *buffer, size_t length), void *handle);
void rc_loadMap(rc_Tracker * tracker, size_t (*read)(void *handle, void *buffer, size_t length), void *handle);
*/
    
#ifdef __cplusplus
}
#endif

#endif
