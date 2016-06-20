//
//  rc_tracker.h
//
//  Created by Eagle Jones on 1/29/15.
//  Copyright (c) 2015 Intel. All rights reserved.
//

#ifndef rc_tracker_h
#define rc_tracker_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#ifndef _WIN32
#include <stdbool.h>
#endif

#ifdef _WIN32
#  ifdef RCTRACKER_API_EXPORTS
#    define RCTRACKER_API __declspec(dllexport)
#  else
#    define RCTRACKER_API __declspec(dllimport)
#  endif
#else
#  define RCTRACKER_API __attribute__ ((visibility("default")))
#endif

typedef enum rc_ImageFormat {
    rc_FORMAT_GRAY8,
    rc_FORMAT_DEPTH16,
} rc_ImageFormat;

typedef enum rc_TrackerState
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
    rc_E_LANDSCAPE_CALIBRATION = 6,
    /** rc_Tracker is running in inertial only mode. Orientation will be tracked. */
    rc_E_INERTIAL_ONLY
} rc_TrackerState;

typedef enum rc_TrackerError
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

typedef enum rc_TrackerConfidence
{
    /** rc_Tracker is not currently running (possibly due to failure). */
    rc_E_CONFIDENCE_NONE = 0,
    /** The data has low confidence. This occurs if no visual features could be detected or tracked. */
    rc_E_CONFIDENCE_LOW = 1,
    /** The data has medium confidence. This occurs when rc_Tracker has recently been initialized, or has recovered from having few usable visual features, and continues until the user has moved sufficiently to produce reliable measurements. If the user moves too slowly or features are unreliable, this will not switch to rc_E_CONFIDENCE_HIGH, and measurements may be unreliable. */
    rc_E_CONFIDENCE_MEDIUM = 2,
    /** Sufficient visual features and motion have been detected that measurements are likely to be accurate. */
    rc_E_CONFIDENCE_HIGH = 3
} rc_TrackerConfidence;

typedef struct rc_Vector {
    float x,y,z;
} rc_Vector;

/**
 Flat array of 12 floats, corresponding to 3x4 transformation matrix in row-major order:
 
 [R00 R01 R02 T0]
 [R10 R11 R12 T1]
 [R20 R21 R22 T2]
 */
typedef float rc_Pose[12];

typedef struct {
    rc_Pose alignment_and_bias_m__s2;
    float noiseVariance_m2__s4;
} rc_AccelerometerIntrinsics;

typedef struct {
    rc_Pose alignment_and_bias_rad__s;
    float noiseVariance_rad2__s2;
} rc_GyroscopeIntrinsics;

static const rc_Pose rc_POSE_IDENTITY = { 1.f, 0.f, 0.f, 0.f,
                                          0.f, 1.f, 0.f, 0.f,
                                          0.f, 0.f, 1.f, 0.f };

/**
 Timestamp, in microseconds
 */
typedef int64_t rc_Timestamp;

typedef uint16_t rc_Sensor;

typedef struct rc_Feature
{
    uint64_t id;
    rc_Vector world;
    float image_x, image_y;
    bool initialized;
    float stdev;
} rc_Feature;

typedef enum rc_MessageLevel
{
    rc_MESSAGE_NONE = 0, /* no output */
    rc_MESSAGE_ERROR = 1, /* only errors */
    rc_MESSAGE_WARN = 2, /* errors + warnings */
    rc_MESSAGE_INFO = 3, /* errors + warnings + info */
    rc_MESSAGE_DEBUG = 4, /* errors + warnings + info + debug */
    rc_MESSAGE_TRACE = 5, /* everything */
} rc_MessageLevel;

typedef void(*rc_DataCallback)(void *handle, rc_Timestamp time, rc_Pose pose, rc_Feature *features, size_t feature_count);
typedef void(*rc_StatusCallback)(void *handle, rc_TrackerState state, rc_TrackerError error, rc_TrackerConfidence confidence, float progress);
typedef void(*rc_MessageCallback)(void *handle, rc_MessageLevel message_level, const char * message, size_t len);

typedef struct rc_Tracker rc_Tracker;

RCTRACKER_API const char *rc_version();
RCTRACKER_API rc_Tracker * rc_create();
RCTRACKER_API void rc_destroy(rc_Tracker *tracker);

/**
 Resets system, clearing all history and state, and sets initial pose and time.
 System will be stopped until one of the rc_start_ functions is called.
 @param initialPose_m is deprecated, always pass the identity and use rc_setPose() after convergence
 */
RCTRACKER_API void rc_reset(rc_Tracker *tracker, rc_Timestamp initialTime_us, const rc_Pose initialPose_m);

typedef enum rc_CalibrationType {
    rc_CALIBRATION_TYPE_UNKNOWN,     // rd = ???
    rc_CALIBRATION_TYPE_FISHEYE,     // rd = arctan(2 * ru * tan(w / 2)) / w
    rc_CALIBRATION_TYPE_POLYNOMIAL3, // rd = ru * (k1 * ru^2 + k2 * ru^4 + k3 * ru^6)
    rc_CALIBRATION_TYPE_UNDISTORTED, // rd = ru
} rc_CalibrationType;

/**
 @param format Image format
 @param width_px Image width in pixels
 @param height_px Image height in pixels
 @param f_x_px Focal length of camera in pixels
 @param f_y_px Focal length of camera in pixels
 @param c_x_px Horizontal principal point of camera in pixels
 @param c_y_px Veritical principal point of camera in pixels
 @param k1,k2,k3 Polynomial distortion parameters
 @param w Fisheye camera field of view in radians (half-angle FOV)
 */
typedef struct rc_CameraIntrinsics {
    rc_CalibrationType type;
    rc_ImageFormat format;
    uint32_t width_px, height_px;
    double f_x_px, f_y_px;
    double c_x_px, c_y_px;
    union {
        double distortion[5];
        struct { double k1,k2,k3; };
        double w;
    };
} rc_CameraIntrinsics;

/**
 @param tracker The active rc_Tracker instance
 @param camera_id Refers to one of a specific supported predefined set
 @param extrinsics_wrt_accel_m Transformation from the Camera frame to the Accelerometer frame in meters (may be NULL)
 @param intrinsics Camera Intrinsics (may be NULL)
 */
RCTRACKER_API bool rc_describeCamera(rc_Tracker *tracker,  rc_Sensor camera_id,       rc_Pose extrinsics_wrt_origin_m,       rc_CameraIntrinsics *intrinsics);
RCTRACKER_API void rc_configureCamera(rc_Tracker *tracker, rc_Sensor camera_id, const rc_Pose extrinsics_wrt_origin_m, const rc_CameraIntrinsics *intrinsics);
RCTRACKER_API void rc_configureAccelerometer(rc_Tracker *tracker, rc_Sensor accel_id, const rc_Pose extrinsics_wrt_origin_m, const rc_AccelerometerIntrinsics * intrinsics);
RCTRACKER_API void rc_configureGyroscope(rc_Tracker *tracker, rc_Sensor gyro_id, const rc_Pose extrinsics_wrt_origin_m, const rc_GyroscopeIntrinsics * intrinsics);
RCTRACKER_API void rc_configureLocation(rc_Tracker *tracker, double latitude_deg, double longitude_deg, double altitude_m);

/**
  WARNING: These callbacks are synchronous with the the filter thread. Don't do significant work in them!
*/
RCTRACKER_API void rc_setDataCallback(rc_Tracker *tracker, rc_DataCallback callback, void *handle);
RCTRACKER_API void rc_setStatusCallback(rc_Tracker *tracker, rc_StatusCallback callback, void *handle);
RCTRACKER_API void rc_setMessageCallback(rc_Tracker *tracker, rc_MessageCallback callback, void *handle, rc_MessageLevel maximum_level);

typedef enum rc_TrackerRunFlags
{
    /** rc_Tracker should process data on the callers thread. */
    rc_E_SYNCHRONOUS = 0,
    /** rc_Tracker should process data on its own thread, returning immediately from all calls. */
    rc_E_ASYNCHRONOUS = 1,
} rc_TrackerRunFlags;

RCTRACKER_API void rc_startCalibration(rc_Tracker *tracker, rc_TrackerRunFlags run_flags);

/**
 Resets position and pauses the tracker. While paused, the tracker will continue processing inertial measurements and updating orientation. When rc_unpause is called, the tracker will start up again very quickly.
 */
RCTRACKER_API void rc_pauseAndResetPosition(rc_Tracker *tracker);

/**
 Resumes full tracker operation.
 */
RCTRACKER_API void rc_unpause(rc_Tracker *tracker);

/**
 Start buffering data. Currently 6 images and 64 imu samples are retained. Example usage:

 rc_startBuffering(tracker);
 // call receive as many times as needed
 rc_receiveImage(...)
 rc_receiveAccelerometer(...)
 rc_receiveGyro(...)
 // start the tracker and set the position
 rc_startTracker(tracker, rc_E_SYNCHRONOUS);
 rc_setPose(tracker, rc_POSE_IDENTITY);
 */
RCTRACKER_API void rc_startBuffering(rc_Tracker *tracker);

/**
 Starts the tracker.
 */
RCTRACKER_API void rc_startTracker(rc_Tracker *tracker, rc_TrackerRunFlags run_flags);
RCTRACKER_API void rc_stopTracker(rc_Tracker *tracker);

/**
 @param tracker The active rc_Tracker instance
 @param time_us Timestamp (in microseconds) when capture of this frame began
 @param shutter_time_us Exposure time (in microseconds). For rolling shutter, this should be the time such that exposure line l takes place at time_us + l/height * shutter_time_us. For global shutter, specify the exposure time, so that the middle of the exposure will be time_us + shutter_time_us / 2.
 @param stride Number of bytes in each line
 @param image Image data.
 @param completion_callback Function to be called when the frame has been processed and image data is no longer needed. image must remain valid (even after receiveImage has returned) until this function is called.
 @param callback_handle An opaque pointer that will be passed to completion_callback when the frame has been processed and image data is no longer needed.
 */
RCTRACKER_API void rc_receiveImage(rc_Tracker *tracker, rc_Sensor camera_id, rc_ImageFormat format, rc_Timestamp time_us, rc_Timestamp shutter_time_us, int width, int height, int stride, const void *image, void(*completion_callback)(void *callback_handle), void *callback_handle);
RCTRACKER_API void rc_receiveAccelerometer(rc_Tracker *tracker, rc_Sensor accelerometer_id, rc_Timestamp time_us, const rc_Vector acceleration_m__s2);
RCTRACKER_API void rc_receiveGyro(rc_Tracker *tracker, rc_Sensor gyro_id, rc_Timestamp time_us, const rc_Vector angular_velocity_rad__s);

/**
 @param tracker The active rc_Tracker instance
 @param pose_m Position (in meters) relative to the camera reference frame
 Immediately after a call rc_getPose() will return pose_m.  For best
 results, call this once the tracker has converged and the confidence
 is rc_E_CONFIDENCE_MEDIUM or better rc_E_CONFIDENCE_HIGH.
 */
RCTRACKER_API void rc_setPose(rc_Tracker *tracker, const rc_Pose pose_m);
RCTRACKER_API void rc_getPose(const rc_Tracker *tracker, rc_Pose pose_m);
RCTRACKER_API int rc_getFeatures(rc_Tracker *tracker, rc_Feature **features_px);
RCTRACKER_API rc_TrackerState rc_getState(const rc_Tracker *tracker);
RCTRACKER_API rc_TrackerConfidence rc_getConfidence(const rc_Tracker *tracker);
RCTRACKER_API rc_TrackerError rc_getError(const rc_Tracker *tracker);
RCTRACKER_API float rc_getProgress(const rc_Tracker *tracker);

/**
 @param tracker The active rc_Tracker instance
 @param log The function to call with output
 @param stream If true, log every calculated output pose
 @param period_us If non-zero, log each calculated pose when it has been period_us microseconds or more since the last pose was logged
 @param handle Token to pass to log callback
 */
RCTRACKER_API void rc_setLog(rc_Tracker *tracker, void (*log)(void *handle, const char *buffer_utf8, size_t length), bool stream, rc_Timestamp period_us, void *handle);

/**
 Immediately outputs the last calculated pose
 */
RCTRACKER_API void rc_triggerLog(const rc_Tracker *tracker);

/**
 Returns a string with statistics on sensor timing
 */
RCTRACKER_API const char *rc_getTimingStats(rc_Tracker *tracker);

/**
 If this is set, writes a log file in Realitycap's internal format to the filename specified
 */
RCTRACKER_API bool rc_setOutputLog(rc_Tracker *tracker, const char *filename, rc_TrackerRunFlags run_flags);

/**
    Yields a JSON string that represents the tracker's current a calibration data.
*/
RCTRACKER_API size_t rc_getCalibration(rc_Tracker *tracker, const char **buffer);
/**
    Loads a JSON string representing calibration data into the tracker.
*/
RCTRACKER_API bool rc_setCalibration(rc_Tracker *tracker, const char *buffer);

/**
 Start/stop the mapping subsystem. When started, the map is completely empty. The map is build synchronously with rc_receive* startMapping must be called before loadMap
 */
RCTRACKER_API void rc_startMapping(rc_Tracker *tracker);
RCTRACKER_API void rc_stopMapping(rc_Tracker *tracker);

/**
 Save/load a map to use.
 */
RCTRACKER_API void rc_saveMap(rc_Tracker *tracker,  void (*write)(void *handle, const void *buffer, size_t length), void *handle);
RCTRACKER_API bool rc_loadMap(rc_Tracker *tracker, size_t (*read)(void *handle, void *buffer, size_t length), void *handle);

#ifdef __cplusplus
}
#endif

#endif
