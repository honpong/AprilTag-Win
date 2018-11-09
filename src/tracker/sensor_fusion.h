//
//  sensor_fusion.h
//  RC3DK
//
//  Created by Eagle Jones on 3/2/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#ifndef __RC3DK__sensor_fusion__
#define __RC3DK__sensor_fusion__

#include <string>
#include <vector>
#include "transformation.h"
#include "platform/sensor_clock.h"
#include "sensor_data.h"
#include "sensor_fusion_queue.h"
#include "RCSensorFusionInternals.h"
#include "filter.h"
#include "sensor.h"
#include <mutex>

class sensor_fusion
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    struct feature_point
    {
        featureid id;
        f_t x, y;
        f_t original_depth;
        f_t stdev;
        f_t worldx, worldy, worldz;
        bool initialized;
    };
    
    std::function<void(const sensor_data *)> data_callback;
    std::function<bool()> status_callback;
    std::function<void(rc_Timestamp)> stage_callback;
    std::function<void(const rc_Relocalization&)> relocalization_callback;
    
    sensor_fusion(fusion_queue::latency_strategy strategy);
    virtual ~sensor_fusion();
    
    /** Sets the current location of the device.
     
     The device's current location (including altitude) is used to account for differences in gravity across the earth. If location is unavailable, results may be less accurate. This should be called before starting sensor fusion or calibration.
     
     @param latitude_degrees Latitude in degrees, with positive north and negative south.
     @param longitude_degrees Longitude in degrees, with positive east and negative west.
     @param altitude_meters Altitude above sea level, in meters.
     */
    void set_location(double latitude_degrees, double longitude_degrees, double altitude_meters);

    bool set_stage(const char *name, const rc_Pose &G_world_stage);
    bool get_stage(bool next, const char *name, mapper::stage::output &stage);

    //These may all run async
    
    /** Prepares the object to receive video and inertial data, and starts sensor fusion updates.
     
     This method should be called when you are ready to begin receiving sensor fusion updates and the user is aware to point the camera at an appropriate visual scene. After you call this method you should immediately begin passing video, accelerometer, and gyro data using receive_image, receive_accelerometer, and receive_gyro respectively. Full processing will not begin until the user has held the device steady for a brief initialization period (this occurs concurrently with focusing the camera). The device does not need to be perfectly still; minor shake from the device being held in hand is acceptable.
     */
    void start(bool threaded, bool fast_path, bool estimate_extrinsics);

    void pause_and_reset_position();
    void unpause();
    
    void start_buffering();
    bool started();

    /** Stops the processing of video and inertial data. */
    void stop();

    /**
     Resets system, clearing all history and state, and sets initial pose and time.
     System will be stopped until one of the start_ functions is called.
     */
    void reset(sensor_clock::time_point time);
    
    /** Once sensor fusion has started, all data is passed via
     * std::move through receive_data
     @param data The image/depth/gyro/accelerometer data
     */
    void receive_data(sensor_data && data);
    
    void start_mapping(bool relocalize, bool save_map, bool allow_jumps);
    void destroy_mapping();

    void save_map(rc_SaveCallback write, void *handle);

    bool load_map(rc_LoadCallback read, void *handle);


    //*************Not yet implemented:
    
    
    /** Determine if valid saved calibration data exists from a previous run.
     
     @return If false, it is strongly recommended to perform the calibration procedure by calling start_calibration.
     @note In some cases, calibration data may become invalid or go out of date, in which case this will return false even if it previously returned true. It is recommended to check hasCalibrationData before each use, even if calibration has previously been run successfully.
     */
//    bool has_calibration_data();

    /** Sets the 3DK license key. Call this once before starting sensor fusion. In most cases, this should be done when your app starts.
     
     @param key A 30 character string. Obtain a license key by contacting RealityCap.
     */
//    void set_license_key(const std::string &key);
    
    
    
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
    //void start_qr_detection(const std::string& data, float size, bool align_gravity);
    
    /** Stops searching for QR codes.
     */
    //void stop_qr_detection();
    
    std::string get_timing_stats();
    fusion_queue queue;
    void queue_receive_data(sensor_data &&data, bool catchup = true);
    void queue_receive_data_fast(sensor_data &data);

    //public for now
    filter sfm;

    //Gets the current transformation, moving from filter-internal to external coordinates
    //Adjusts for camera vs accel centered and QR offset
    transformation get_transformation() const;
    void set_transformation(const transformation &pose_m);

private:
    friend class replay; //Allow replay to access queue directly so it can send the obsolete start measuring signal, which we don't expose elsewhere
    void update_status();
    void update_data(const sensor_data * data);
    void update_stages(rc_Timestamp time_us);
    void update_relocalizations(const map_relocalization_info & info);
    void update_relocalizations(const std::vector<nodeid>& brought_back_groups, rc_Timestamp time_us);
    void stop_threads();
    void stop_mapping_threads();
    std::atomic<bool> isProcessingVideo, isSensorFusionRunning;
    bool threaded;
    bool fast_path = false;

    void flush_and_reset();
    void fast_path_catchup();

    bool buffering = true;

    std::recursive_mutex mini_mutex;
    std::thread save_map_thread;
};

#endif /* defined(__RC3DK__sensor_fusion__) */
