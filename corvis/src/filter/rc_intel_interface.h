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

enum rc_camera
{
    depth,
    rgb
};

struct rc_vector
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
struct rc_pose
{
    double R[3][3];
    rc_vector T;
};

/**
 Timestamp, in microseconds
 */
typedef uint64_t rc_timestamp;

struct rc_tracker;

struct rc_data;

struct rc_tracker* rc_create();
void rc_destroy(s)truct rc_tracker *tracker);

//These functions all run synchronously; assume that the start_ functions have not been called yet
/**
 @param tracker The active rc_tracker instance
 @param camera The camera to configure
 @param pose_m Position (in meters) and orientation of camera relative to reference point
 @param image_width_px Image width in pixels
 @param image_height_px Image height in pixels
 @param focal_length_px Focal length in pixels
 @param principal_x_px Horizontal principal point in pixels
 @param principal_y_px Vertical principal point in pixels
 */
void rc_configure_camera(struct rc_tracker *tracker, enum rc_camera camera,
                         const struct rc_pose *pose_m,
                         int width_px, int height_px,
                         int principal_x_px, int principal_y_px, int focal_length_px);
void rc_configure_accelerometer(struct rc_tracker *tracker, const struct rc_pose *pose_m, struct rc_vector *bias_m__s_s, float noise_variance);
void rc_configure_gyroscope(struct rc_tracker *tracker, const struct rc_pose *pose_m, struct rc_vector *bias_rad__s, float noise_variance);

void rc_set_location(struct rc_tracker *tracker, double latitude_degrees, double longitude_degrees, double altitude_meters);
//TBD: how to trigger logging for various log modes (events vs stream vs single)
void rc_set_log(struct rc_tracker *tracker, void (*log)(char *buffer_utf8, size_t length));

//These may all run async
void rc_start_calibration(struct rc_tracker *tracker);
void rc_start_inertial_only(struct rc_tracker *tracker);
void rc_start_full_tracker(struct rc_tracker *tracker);

void rc_receive_image(struct rc_tracker *tracker, rc_camera camera, const rc_pose *pose_estimate_m, rc_timestamp time_us, const uint8_t *image);
//Acceleration is in m/s^2
void rc_receive_accelerometer(struct rc_tracker *tracker, rc_timestamp time_us, const rc_vector *acceleration_m__s_s);
//Angular velocity is in rad/s
void rc_receive_gyro(struct rc_tracker *tracker, rc_timestamp time_us, const rc_vector *angular_velocity_rad__s);

/**
 Resets system, clearing all history and state, and sets initial pose and time.
 System will be stopped until one of the start_ functions is called.
 */
void reset(struct rc_tracker *tracker, const struct rc_pose *initial_pose_m, rc_timestamp initial_time_us);

/* To be implemented with location recognition / loop closure
void rc_start_mapping_from_pose_estimates(struct rc_tracker *tracker);
void rc_save_tracker(struct rc_tracker *tracker);
void rc_load_tracker(struct rc_tracker *tracker);
*/

#ifdef __cplusplus
}
#endif

#endif
