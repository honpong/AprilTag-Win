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

#include <stdint.h>

enum rc_CameraType
{
    depth,
    rgb
};

struct rc_Vector
{
    double x,y,z;
}

/**
 This maps to a flat array of 12 floats, corresponding to 3x3 rotation matrix followed by translation vector in row-major order:
 
 [R00 R01 R02]
 [R10 R11 R12]
 [R20 R21 R22]
 [T0 T1 T2]
 */
typedef double rc_Pose[12];

/**
 Timestamp, in nanoseconds
 */
typedef int64_t rc_Timestamp;

enum rc_LogMode {
    stream,  // log input data
    trigger, // call log now
    event,   // call log in future with events
};

struct rc_Tracker;

struct rc_Tracker* rc_create();
void rc_destroy(struct rc_Tracker *tracker);

/**
 Resets system, clearing all history and state, and sets initial pose and time.
 System will be stopped until one of the rc_start_ functions is called.
 */
void rc_reset(struct rc_Tracker *tracker, const struct rc_Pose *initialPose_m, rc_Timestamp initialTime_ns);
void rc_save(struct rc_Tracker *tracker,  void (*write)(void *, void *buffer, size_t length), void *);
void rc_load(struct rc_Tracker *tracker, size_t (*read)(void *, void *buffer, size_t length), void *);

//These functions all run synchronously; assume that the start_ functions have not been called yet
/**
 @param tracker The active rc_Tracker instance
 @param camera The camera to configure
 @param pose_m Position (in meters) and orientation of camera relative to reference point
 @param center_px, Horizontal principal point / Vertical principal point / Focal length all in pixels
 @param image_width_px Image width in pixels
 @param image_height_px Image height in pixels
 */
void rc_configureCamera(struct rc_Tracker *tracker, int index, const struct rc_Pose *pose_m, const struct rc_Vector *center_px,
                         int width_px, int height_px, enum rc_CameraType camera);
void rc_configureAccelerometer(struct rc_Tracker *tracker, int index, const struct rc_Pose *pose_m, struct rc_Vector *bias_m__s_s, float noiseVariance);
void rc_configureGyroscope(struct rc_Tracker *tracker, int index, const struct rc_Pose *pose_m, struct rc_Vector *bias_rad__s, float noiseVariance);

void rc_setLocation(struct rc_Tracker *tracker, double latitude_deg, double longitude_deg, double altitude_m);

void rc_setLog(struct rc_Tracker *tracker, void (*log)(void *, char *buffer_utf8, size_t length), rc_Timestamp period_ns, enum rc_LogMode mode, void *);

//These may all run async
void rc_startCalibration(struct rc_Tracker *tracker);
bool rc_getCalibration(struct rc_Tracker *tracker, char **buffer, int *length);
enum rc_TrackerFlag { rc_start_inertial_only = 1 };
void rc_start(struct rc_Tracker *tracker, enum rc_TrackerFlag flags);

void rc_receiveImage(struct rc_Tracker *tracker, int index, rc_Timestamp time_ns, const rc_Pose *poseEstimate_m, const uint8_t *image);
void rc_receiveAccelerometer(struct rc_Tracker *tracker, int index, rc_Timestamp time_ns, const rc_Pose *poseEstimate_m, const rc_Vector *acceleration_m__s_s);
void rc_receiveGyro(struct rc_Tracker *tracker, int index, rc_Timestamp time_ns, const rc_Pose *poseEstimate_m, const rc_Vector *angular_velocity_rad__s);

struct rc_Feature { int id; float x, y; };
int rc_getFeatures(struct rc_Tracker *tracker, int index, struct rc_Feature **features_px);

enum rc_PerformFlag { rc_loop_closure=1, rc_relocalization=2 };
void rc_perform(struct rc_Tracker *tracker, enum rc_PerformFlag flags);

#ifdef __cplusplus
}
#endif

#endif
