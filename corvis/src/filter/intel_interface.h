//
//  intel_interface.h
//
//  Created by Eagle Jones on 1/29/15.
//  Copyright (c) 2015 Realitycap. All rights reserved.
//

#ifndef intel_interface_h
#define intel_interface_h

enum class camera
{
    depth,
    rgb
};

class intel_interface
{
    
//These functions all run synchronously, assume that the start_ functions have not been called yet
    /**
     @param source_camera The camera to configure
     @param image_width Image width in pixels
     @param image_height Image height in pixels
     @param focal_length Focal length in pixels
     @param principal_x Horizontal principal point in pixels
     @param principal_y Vertical principal point in pixels
     @param extrinsics 4x3 matrix containing 3x3 rotation matrix and 1x3 translation vector (stored as fourth column) in row-major order (first 4 entries = [R00 R01 R02 T0])
     */
    void configure_camera(camera source_camera, int image_width, int image_height, float focal_length, float principal_x, float principal_y, float extrinsics[12]);
    
    void configure_accelerometer(float bias_x, float bias_y, float bias_z, float noise_variance, float extrinsics[12]);
    
    void configure_gyroscope(float bias_X, float bias_y, float bias_z, float noise_variance, float extrinsics[12]);

    void set_location(double latitude_degrees, double longitude_degrees, double altitude_meters);


//These may all run async
    //All timestamps are in microseconds

    /**
     Resets system, clearing all history and state, and sets initial pose and time.
     System will be stopped until one of the start_ functions is called.
     */
    void reset(float *initial_pose, uint64_t initial_timestamp);
    void start_calibration();
    void start_full_processing();

    //These two can be run at the same time or independently
    void start_mapping_from_pose_estimates();
    void start_inertial_only();
    
    void save_state();
    void load_state();
    void log_output();
    
    void receive_image(camera which_camera, uint8_t *image, float *pose, uint64_t timestamp);
    
    //Acceleration is in m/s^2
    void receive_accelerometer(float x, float y, float z, uint64_t timestamp);
    
    //Angular velocity is in rad/s
    void receive_gyro(float x, float y, float z, uint64_t timestamp);
    
};

#endif
