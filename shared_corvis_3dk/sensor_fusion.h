//
//  sensor_fusion.h
//  RC3DK
//
//  Created by Eagle Jones on 3/2/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#ifndef __RC3DK__sensor_fusion__
#define __RC3DK__sensor_fusion__

#include <stdint.h>
#include <string>
#include "camera_control_interface.h"
#include "transformation.h"

enum class camera
{
    depth,
    rgb
};


/**
 Timestamp, in microseconds
 */
typedef uint64_t timestamp_t;

class sensor_fusion
{
    //These functions all run synchronously; assume that the start_ functions have not been called yet
    /**
     @param source_camera The camera to configure
     @param image_width Image width in pixels
     @param image_height Image height in pixels
     @param focal_length Focal length in pixels
     @param principal_x Horizontal principal point in pixels
     @param principal_y Vertical principal point in pixels
     @param extrinsics Position and orientation of camera relative to reference point
     */
    void configure_camera(camera source_camera, int image_width, int image_height, float focal_length, float principal_x, float principal_y, const transformation &extrinsics);
    
    /** Sets the 3DK license key. Call this once before starting sensor fusion. In most cases, this should be done when your app starts.
     
     @param key A 30 character string. Obtain a license key by contacting RealityCap.
     */
    void set_license_key(const std::string &key);
    
    void configure_accelerometer(float bias_x, float bias_y, float bias_z, float noise_variance, const transformation &extrinsics);
    
    void configure_gyroscope(float bias_x, float bias_y, float bias_z, float noise_variance, const transformation &extrinsics);
    
    /** Sets the current location of the device.
     
     The device's current location (including altitude) is used to account for differences in gravity across the earth. If location is unavailable, results may be less accurate. This should be called before starting sensor fusion or calibration.
     
     @param latitude_degrees Latitude in degrees, with positive north and negative south.
     @param longitude_degrees Longitude in degrees, with positive east and negative west.
     @param altitude_meters Altitude above sea level, in meters.
     */
    void set_location(double latitude_degrees, double longitude_degrees, double altitude_meters);
    
    /** Determine if valid saved calibration data exists from a previous run.
     
     @return If false, it is strongly recommended to perform the calibration procedure by calling start_calibration.
     @note In some cases, calibration data may become invalid or go out of date, in which case this will return false even if it previously returned true. It is recommended to check hasCalibrationData before each use, even if calibration has previously been run successfully.
     */
    bool has_calibration_data();
    
    
    //These may all run async
    
    /**
     Resets system, clearing all history and state, and sets initial pose and time.
     System will be stopped until one of the start_ functions is called.
     */
    void reset(transformation initial_pose, timestamp_t initial_time);
    void start_calibration();
    void start_full_processing();
    
    /** Starts to search for a QR code and once detected reports future transformations relative to the observed QR code.
     
     They sensor fusion system will attempt to detect a QR code until one is found or stop_qr_detection is called. Once the code has been detected, sensor_fusion_data.origin_qr_code will be set to the payload of the QR code, and future instances of sensor_fusion_data.transformation and sensor_fusion_data.camera_transformation will be modified with the origin fixed to the center of the QR code. If align_gravity is false, then positive x will point toward the canonical "right" side of the QR code, positive y will point toward the canonical "top" of the QR code, and positive z will point out of the plane of the QR code. If alignGravity is true (recommended), the coordinates will be rotated so that the positive z axis points opposite to gravity.
     
     [ ]  ^+y [ ]
          |
          o--->+x
     
     [ ]
     
     @param data The expected payload of the QR code. If nullptr is passed, the first detected QR code will be used.
     @param size The size of the QR code (width = height) in meters.
     @param align_gravity If true (recommended), the z axis will be aligned with gravity; if false the z axis will be perpindicular to the QR code.
     */
    void start_qr_detection(const std::string *data, float size, bool align_gravity);
    
    /** Stops searching for QR codes.
     */
    void stop_qr_detection();

    
    /** Prepares the object to receive video and inertial data, and starts sensor fusion updates.
     
     This method should be called when you are ready to begin receiving sensor fusion updates and the user is aware to point the camera at an appropriate visual scene. After you call this method you should immediately begin passing video, accelerometer, and gyro data using receive_image, receive_accelerometer, and receive_gyro respectively. Full processing will not begin until the user has held the device steady for a brief initialization period (this occurs concurrently with focusing the camera). The device does not need to be perfectly still; minor shake from the device being held in hand is acceptable. If the user moves during this time, the timer will start again. The progress of this timer is provided as a float between 0 and 1 in sensor_fusion_status.progress.
     
     @param device The camera_control_interface to be used for capture. This function will lock the focus on the camera device (if the device is capable of focusing) before starting video processing. No other modifications to the camera settings are made.
     */
    void start_sensor_fusion(camera_control_interface &device);
    
    /** Prepares the object to receive video and inertial data, and starts sensor fusion updates.
     
     This method may be called when you are ready to begin receiving sensor fusion updates and the user is aware to point the camera at an appropriate visual scene. After you call this method you should immediately begin passing video, accelerometer, and gyro data using receive_image, receive_accelerometer, and receive_gyro respectively. It is strongly recommended to call start_sensor_fusion rather than this function, unless it is absolutely impossible for the device to be held steady while initializing (for example, in a moving vehicle). There will be a delay after calling this function before video processing begins, while the camera is focused and sensor fusion is initialized.
     
     @param device The camera_control_interface to be used for capture. This function will lock the focus on the camera device (if the device is capable of focusing) before starting video processing. No other modifications to the camera settings are made.
     @note It is strongly recommended to call start_sensor_fusion rather than this function
     */
    void start_sensor_fusion_unstable(camera_control_interface &device);
    
    /** Stops the processing of video and inertial data. */
    void stop_sensor_fusion();
    
    /** Once sensor fusion has started, video frames should be passed in as they are received from the camera.
     @param which_camera Specifies which camera generated this image.
     @param image The image data
     @param pose Optional estimate of the pose of the device, if available. nullptr if not available.
     @param time Time image was captured, in microseconds.
     */
    void receive_image(camera which_camera, uint8_t *image, float *pose, timestamp_t time);
    
    /** Once sensor fusion has started, acceleration data should be passed in as it's received from the accelerometer.
     @param x Acceleration along the x axis, in m/s^2
     @param y Acceleration along the y axis, in m/s^2
     @param z Acceleration along the z axis, in m/s^2
     */
    void receive_accelerometer(float x, float y, float z, timestamp_t time);
    
    /** Once sensor fusion has started, angular velocity data should be passed in as it's received from the gyro.
     @param x Angular velocity around the x axis, in rad/s
     @param y Angular velocity around the y axis, in rad/s
     @param z Angular velocity around the z axis, in rad/s
     */
    void receive_gyro(float x, float y, float z, timestamp_t time);
    
    //These two can be run at the same time or independently
    void start_mapping_from_pose_estimates();
    void start_inertial_only();
    
    void save_state();
    void load_state();
    void log_output();
    
    
};

#endif /* defined(__RC3DK__sensor_fusion__) */
