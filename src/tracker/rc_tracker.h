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
#include <stdbool.h>

#ifdef _WIN32
#  ifdef RCTRACKER_API_EXPORTS
#    define RCTRACKER_API __declspec(dllexport)
#  else
#    define RCTRACKER_API __declspec(dllimport)
#  endif
#else
#  define RCTRACKER_API __attribute__ ((visibility("default")))
#endif

typedef enum rc_TrackerState
{
    /** rc_Tracker is inactive. */
    rc_E_INACTIVE = 0,
    /** startSensorFusionUnstableWithDevice: has been called, and rc_Tracker is in the handheld dynamic initialization phase. */
    rc_E_DYNAMIC_INITIALIZATION = 3,
    /** rc_Tracker is active and updates are being provided with all data. */
    rc_E_RUNNING = 4,
    /** rc_Tracker is running in inertial only mode. Orientation will be tracked. */
    rc_E_INERTIAL_ONLY = 7,
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

typedef enum rc_TrackerQueueStrategy
{
    rc_QUEUE_MINIMIZE_DROPS = 0,
    rc_QUEUE_MINIMIZE_LATENCY = 1,
} rc_TrackerQueueStrategy;

/**
 Timestamp, in microseconds
 */
typedef int64_t rc_Timestamp;

typedef enum rc_SessionId
{
    rc_SESSION_CURRENT_SESSION = 0,
    rc_SESSION_PREVIOUS_SESSION = 1,
} rc_SessionId;

typedef struct { float v[3][3]; } rc_Matrix;
typedef union { struct { float x,y,z;   }; float v[3]; } rc_Vector;
typedef union { struct { float x,y,z,w; }; float v[4]; } rc_Quaternion;
typedef struct { rc_Quaternion Q; rc_Vector T; rc_Matrix R; } rc_Pose; // both Q and R are always output, the closer to 1 of |Q| and |R| is used for input
typedef struct { rc_Vector W; rc_Vector T; } rc_PoseVelocity; // Q is the spatial (not body) velocity and T the derivative of rc_Pose's T
typedef struct { rc_Vector W; rc_Vector T; } rc_PoseAcceleration; // derivative of rc_PoseVelocity
typedef struct { rc_Vector W; rc_Vector T; } rc_PoseVariance; // this is not the full variance yet
typedef struct { rc_Pose pose_m; rc_Timestamp time_us; } rc_PoseTime;
typedef struct { rc_Timestamp time_us; rc_SessionId session; } rc_Relocalization;

#if __cplusplus
static const rc_Matrix rc_MATRIX_IDENTITY = {
    {{1, 0, 0},
     {0, 1, 0},
     {0, 0, 1}},
};
static const rc_Quaternion rc_QUATERNION_IDENTITY = {
    {0,0,0,1},
};
static const rc_Pose rc_POSE_IDENTITY = {
    {{0,0,0,1}},
    {{0,0,0}},
    {{{1, 0, 0},
      {0, 1, 0},
      {0, 0, 1}}},
};
#endif

typedef uint16_t rc_Sensor;

typedef enum rc_SensorType
{
    rc_SENSOR_TYPE_GYROSCOPE = 0,
    rc_SENSOR_TYPE_ACCELEROMETER = 1,
    rc_SENSOR_TYPE_DEPTH = 2,
    rc_SENSOR_TYPE_IMAGE = 3,
    rc_SENSOR_TYPE_STEREO = 4,
    rc_SENSOR_TYPE_THERMOMETER = 5,
    rc_SENSOR_TYPE_DEBUG = 6,
    rc_SENSOR_TYPE_VELOCIMETER = 7,
} rc_SensorType;

typedef enum rc_DataPath
{
    rc_DATA_PATH_SLOW = 0,
    rc_DATA_PATH_FAST = 1,
} rc_DataPath;

typedef struct rc_Feature
{
    uint64_t id;
    rc_Sensor camera_id; // depth is w.r.t this
    rc_Vector world;
    float image_x, image_y;
    float image_prediction_x, image_prediction_y;
    bool initialized;
    float depth;
    float stdev;
    float innovation_variance_x, innovation_variance_y, innovation_variance_xy;
    bool depth_measured;
    bool recovered;
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

typedef enum rc_ImageFormat {
    rc_FORMAT_GRAY8,
    rc_FORMAT_DEPTH16,
    rc_FORMAT_RGBA8,
} rc_ImageFormat;

typedef struct rc_ImageData
{
    rc_Timestamp shutter_time_us;
    int width, height, stride;
    rc_ImageFormat format;
    const void *image;
    void (*release)(void *handle);
    void *handle;
} rc_ImageData;

typedef struct rc_StereoData
{
    rc_Timestamp shutter_time_us;
    int width, height, stride1, stride2;
    rc_ImageFormat format;
    const void * image1;
    const void * image2;
    void (*release)(void * handle);
    void *handle;
} rc_StereoData;

typedef struct rc_DebugData {
    rc_ImageData image;
    const char *message;
    bool pause;
    bool erase_previous_debug_images;
} rc_DebugData;

typedef struct rc_Data
{
    rc_Sensor id;
    rc_SensorType type;
    rc_Timestamp time_us;
    rc_DataPath path;
    union {
        rc_ImageData image;
        rc_ImageData depth;
        rc_StereoData stereo;
        rc_Vector angular_velocity_rad__s;
        rc_Vector acceleration_m__s2;
        float temperature_C;
        rc_DebugData debug;
        rc_Vector translational_velocity_m__s;
    };
} rc_Data;

typedef struct rc_Stage {
    const char *name;
    rc_Pose pose_m;
} rc_Stage;

typedef struct rc_Tracker rc_Tracker;

/**
  The callbacks are called synchronously with the filter thread
 */
typedef void(*rc_DataCallback)(void *handle, rc_Tracker * tracker, const rc_Data * data);
typedef void(*rc_StatusCallback)(void *handle, rc_TrackerState state, rc_TrackerError error, rc_TrackerConfidence confidence);
typedef void(*rc_MessageCallback)(void *handle, rc_MessageLevel message_level, const char * message, size_t len);
typedef void(*rc_StageCallback)(void *handle, rc_Tracker * tracker, const rc_Stage * stage, rc_Timestamp time_us);
typedef void(*rc_RelocalizationCallback)(void *handle, rc_Tracker * tracker, const rc_Relocalization * relocalization);

RCTRACKER_API const char *rc_version();
RCTRACKER_API rc_Tracker *rc_create();
RCTRACKER_API rc_Tracker *rc_create_at(void *mem, size_t *size);
RCTRACKER_API void rc_destroy(rc_Tracker *tracker);

/**
 Resets system, clearing all history and state, and sets initial time.
 System will be stopped until one of the rc_start*() functions is called.
 */
RCTRACKER_API void rc_reset(rc_Tracker *tracker, rc_Timestamp initial_time_us);

typedef enum rc_CalibrationType {
    rc_CALIBRATION_TYPE_UNKNOWN,        // rd = ???
    rc_CALIBRATION_TYPE_FISHEYE,        // rd = arctan(2 * ru * tan(w / 2)) / w
    rc_CALIBRATION_TYPE_POLYNOMIAL3,    // rd = ru * (k1 * ru^2 + k2 * ru^4 + k3 * ru^6)
    rc_CALIBRATION_TYPE_UNDISTORTED,    // rd = ru
                                        // theta=arctan(ru)
    rc_CALIBRATION_TYPE_KANNALA_BRANDT4 // rd = theta + k1*theta^3 + k2*theta^5 + k3*theta^7 + k4*theta^9
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
    uint32_t width_px, height_px;
    double f_x_px, f_y_px;
    double c_x_px, c_y_px;
    union {
        double distortion[4];
        struct { double k1,k2,k3,k4; };
        double w;
    };
} rc_CameraIntrinsics;

typedef struct {
    rc_Matrix scale_and_alignment;
    rc_Vector bias_m__s2;
    rc_Vector bias_variance_m2__s4;
    float measurement_variance_m2__s4;
    uint32_t decimate_by;
} rc_AccelerometerIntrinsics;

typedef struct {
    rc_Matrix scale_and_alignment;
    rc_Vector bias_rad__s;
    rc_Vector bias_variance_rad2__s2;
    float measurement_variance_rad2__s2;
    uint32_t decimate_by;
} rc_GyroscopeIntrinsics;

typedef struct {
    float scale_and_alignment;
    float bias_C;
    float bias_variance_C2;
    float measurement_variance_C2;
} rc_ThermometerIntrinsics;

typedef struct {
    rc_Matrix scale_and_alignment;
    float measurement_variance_m2__s2;
    uint32_t decimate_by;
} rc_VelocimeterIntrinsics;

/**
 @param T Translation to the origin
 @param W Rotation vector specifying the rotation to the origin
 @param T_variance Variance for the translation
 @param W_variance Variance for the rotation vector
 */
typedef struct rc_Extrinsics {
    rc_Pose pose_m;
    rc_PoseVariance variance_m2;
} rc_Extrinsics;

/**
 Configure or describe a camera or depth camera. When configuring, extrinsics and intrinsics must be set.
 @param tracker The active rc_Tracker instance
 @param camera_id The id of the camera to configure/describe. Camera ids start at 0 and must be sequential.
 @param format Image format controls if you are configuring a camera or depth camera. Depth cameras have their own set of ids starting at 0
 @param extrinsics_wrt_origin_m The transformation from the camera to the origin
 @param intrinsics The intrinsics of the camera

 Must be called before rc_startTracker().
 */
RCTRACKER_API bool rc_configureCamera(rc_Tracker *tracker, rc_Sensor camera_id, rc_ImageFormat format, const rc_Extrinsics *extrinsics_wrt_origin_m, const rc_CameraIntrinsics *intrinsics);
RCTRACKER_API bool rc_describeCamera(rc_Tracker *tracker,  rc_Sensor camera_id, rc_ImageFormat format,       rc_Extrinsics *extrinsics_wrt_origin_m,       rc_CameraIntrinsics *intrinsics);

/* Configures a stereo pair for use with rc_receiveStereo using the
 * calibration from camera_id_0 and camera_id_1. The ID of the stereo
 * pair will be camera_id_0 */
RCTRACKER_API bool rc_configureStereo(rc_Tracker *tracker, rc_Sensor camera_id_0, rc_Sensor camera_id_1);

/**
 Configure or describe an accelerometer. When configuring, extrinsics and intrinsics must be set.
 @param tracker The active rc_Tracker instance
 @param accel_id The id of the accelerometer to configure/describe. Accelerometer ids start at 0 and must be sequential. A gyroscope must be configured for every configured accelerometer.
 @param extrinsics_wrt_origin_m The transformation from the accelerometer to the origin
 @param intrinsics The intrinsics of the accelerometer

 Must be called before rc_startTracker().
 */
RCTRACKER_API bool rc_configureAccelerometer(rc_Tracker *tracker, rc_Sensor accel_id, const rc_Extrinsics *extrinsics_wrt_origin_m, const rc_AccelerometerIntrinsics *intrinsics);
RCTRACKER_API bool rc_describeAccelerometer( rc_Tracker *tracker, rc_Sensor accel_id,       rc_Extrinsics *extrinsics_wrt_origin_m,       rc_AccelerometerIntrinsics *intrinsics);

/**
 Configure or describe an gyroscope. When configuring, extrinsics and intrinsics must be set.
 @param tracker The active rc_Tracker instance
 @param accel_id The id of the accelerometer to configure/describe. Gyroscope ids start at 0 and must be sequential. An accelerometer must be configured for every configured gyroscope.
 @param extrinsics_wrt_origin_m The transformation from the gyroscope to the origin
 @param intrinsics The intrinsics of the gyroscope

 Must be called before rc_startTracker().
 */
RCTRACKER_API bool rc_configureGyroscope(rc_Tracker *tracker, rc_Sensor gyro_id, const rc_Extrinsics *extrinsics_wrt_origin_m, const rc_GyroscopeIntrinsics *intrinsics);
RCTRACKER_API bool rc_describeGyroscope( rc_Tracker *tracker, rc_Sensor gyro_id,       rc_Extrinsics *extrinsics_wrt_origin_m,       rc_GyroscopeIntrinsics *intrinsics);

/**
 Configure or describe an thermometer.
 @param tracker The active rc_Tracker instance
 @param therm_id The id of the temperature sensor to configure/describe. Temperature sensor ids start at 0 and must be sequential.

 Must be called before rc_startTracker().
 */
RCTRACKER_API bool rc_configureThermometer(rc_Tracker *tracker, rc_Sensor therm_id, const rc_Extrinsics *extrinsics_wrt_origin_m, const rc_ThermometerIntrinsics *intrinsics);
RCTRACKER_API bool rc_describeThermometer( rc_Tracker *tracker, rc_Sensor therm_id,       rc_Extrinsics *extrinsics_wrt_origin_m,       rc_ThermometerIntrinsics *intrinsics);

/**
 Configure or describe an velocimeter. When configuring, extrinsics and intrinsics must be set.
 @param tracker The active rc_Tracker instance
 @param velo_id The id of the velocimeter to configure/describe. Velocimeter ids start at 0 and must be sequential.
 @param extrinsics_wrt_origin_m The transformation from the velocimeter to the origin
 @param intrinsics The intrinsics of the velocimeter

 Must be called before rc_startTracker().
 */
RCTRACKER_API bool rc_configureVelocimeter(rc_Tracker *tracker, rc_Sensor velo_id, const rc_Extrinsics *extrinsics_wrt_origin_m, const rc_VelocimeterIntrinsics *intrinsics);
RCTRACKER_API bool rc_describeVelocimeter( rc_Tracker *tracker, rc_Sensor velo_id,       rc_Extrinsics *extrinsics_wrt_origin_m, rc_VelocimeterIntrinsics *intrinsics);

RCTRACKER_API void rc_configureLocation(rc_Tracker *tracker, double latitude_deg, double longitude_deg, double altitude_m);

/**
 World coordinates are defined to be the gravity aligned right handed
 frame coincident with the device and oriented such that the device
 initially looks, modulo elevation, in a known direction
 (world_initial_forward).  rc_getPose() maps points from the body
 fixed reference frame to the world frame defined by this function.
 The extrinsics of rc_configureAccelerometer(),
 rc_configureGyroscope() and rc_configureCamera() all define
 transformations from sensor coordinates to the body fixed reference
 frame.

 @param world_up defines the world up direction.  By default this is
 [0,0,1], i.e. Z is always up.
 @param world_initial_forward will be the initial projection to the
 ground plane of body_forward.  By default this is [0,1,0], i.e. the
 world coordinates are oriented such that the device initially faces
 down the Y axis.
 @param body_forward this defines the "front" of the device. In the
 single camera case, this usually is the direction the camera faces.

 Must be called before rc_startTracker().
 */
RCTRACKER_API void rc_configureWorld(rc_Tracker *tracker, const rc_Vector  world_up, const rc_Vector  world_initial_forward, const rc_Vector  body_forward);
RCTRACKER_API void rc_describeWorld( rc_Tracker *tracker,       rc_Vector *world_up,       rc_Vector *world_initial_forward,       rc_Vector *body_forward);

/* Configures the latency strategy used by the sensor fusion queue.
 * The current default is rc_QUEUE_MINIMIZE_LATENCY, which will use
 * internal constants for each sensor to determine how long to wait
 * until we dispatch data from the queue and potentially drop late
 * arriving data.
 */
RCTRACKER_API bool rc_configureQueueStrategy(rc_Tracker *tracker, rc_TrackerQueueStrategy strategy);
RCTRACKER_API bool rc_describeQueueStrategy(rc_Tracker *tracker, rc_TrackerQueueStrategy * strategy);

/**
 Updates or creates a named stage, an environment fixed coordinate
 frame, relative to tracker world coordinates.

 May only be called when the tracker is running with rc_E_CONFIDENCE_HIGH.

 May be called asynchronously.
 */
RCTRACKER_API bool rc_setStage(rc_Tracker *tracker, const char  *name, const rc_Pose pose_m);

/**
 Returns the pose of a named stage, an environment fixed coordinate
 frame, relative to the current tracker world coordinates.

     if (rc_getStage(tracker, "name", &stage))
         ; // do something with stage

 If name is NULL, then it returns the next stage after stage.name.

     for (rc_Stage stage = {NULL}; rc_getStage(tracker, NULL, &stage); )
         ; // do something with stage

 May only be called from the rc_DataCallback(rc Data *data).
 */
RCTRACKER_API bool rc_getStage(rc_Tracker *tracker, const char *name, rc_Stage *stage);

/**
  WARNING: These callbacks are synchronous with the the filter thread. Don't do significant work in them!
*/
RCTRACKER_API void rc_setDataCallback(rc_Tracker *tracker, rc_DataCallback callback, void *handle);
RCTRACKER_API void rc_setStatusCallback(rc_Tracker *tracker, rc_StatusCallback callback, void *handle);
RCTRACKER_API void rc_setMessageCallback(rc_Tracker *tracker, rc_MessageCallback callback, void *handle, rc_MessageLevel maximum_level);
RCTRACKER_API void rc_setStageCallback(rc_Tracker *tracker, rc_StageCallback callback, void *handle);
RCTRACKER_API void rc_setRelocalizationCallback(rc_Tracker *tracker, rc_RelocalizationCallback callback, void *handle);

typedef enum rc_TrackerRunFlags
{
    /** rc_Tracker should process data on the callers thread. */
    rc_RUN_SYNCHRONOUS  = 0,
    /** rc_Tracker should process data on its own thread, returning immediately from all calls. */
    rc_RUN_ASYNCHRONOUS = 1,
    /** rc_Tracker should process IMU without waiting for image data. */
    rc_RUN_FAST_PATH = 2,
    rc_RUN_NO_FAST_PATH = 0,
    /** rc_Tracker should dynamically estimate extrinsics, etc. for each camera. */
    rc_RUN_DYNAMIC_CALIBRATION = 4,
    rc_RUN_STATIC_CALIBRATION = 0,
} rc_TrackerRunFlags;

#if __cplusplus
}
#include <type_traits>
constexpr rc_TrackerRunFlags operator|(rc_TrackerRunFlags x, rc_TrackerRunFlags y) {
    return static_cast<rc_TrackerRunFlags>(static_cast<typename std::underlying_type<rc_TrackerRunFlags>::type>(x) |
                                           static_cast<typename std::underlying_type<rc_TrackerRunFlags>::type>(y));
}
extern "C" {
#endif

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
RCTRACKER_API bool rc_startBuffering(rc_Tracker *tracker);

/**
 Starts the tracker.
 */
RCTRACKER_API bool rc_startTracker(rc_Tracker *tracker, rc_TrackerRunFlags run_flags);
RCTRACKER_API void rc_stopTracker(rc_Tracker *tracker);

/**
 Receive data, either for procesing (default) or capture (if rc_setOutputLog has been called). Returns false if data is passed for a sensor which has not yet been configured.
 */

/*
 @param tracker The active rc_Tracker instance
 @param camera_id The id of the camera
 @param format The image format of the camera. This determines if the camera is treated as depth or grayscale
 @param time_us Timestamp (in microseconds) when capture of this frame began
 @param shutter_time_us Exposure time (in microseconds). For rolling shutter, this should be the time such that exposure line l takes place at time_us + l/height * shutter_time_us. For global shutter, specify the exposure time, so that the middle of the exposure will be time_us + shutter_time_us / 2.
 @param width The width in pixels of the image
 @param height The height in pixels of the image
 @param stride Number of bytes in each line
 @param image Image data.
 @param completion_callback Function to be called when the frame has been processed and image data is no longer needed. image must remain valid (even after receiveImage has returned) until this function is called.
 @param callback_handle An opaque pointer that will be passed to completion_callback when the frame has been processed and image data is no longer needed. Note: callback_handle must not be null, or completion_callback will not be called.
 */
RCTRACKER_API bool rc_receiveImage(rc_Tracker *tracker, rc_Sensor camera_id, rc_ImageFormat format, rc_Timestamp time_us, rc_Timestamp shutter_time_us, int width, int height, int stride, const void *image, void(*completion_callback)(void *callback_handle), void *callback_handle);

RCTRACKER_API bool rc_receiveStereo(rc_Tracker *tracker, rc_Sensor stereo_id, rc_ImageFormat format, rc_Timestamp time_us, rc_Timestamp shutter_time_us, int width, int height, int stride1, int stride2, const void *image1, const void * image2, void(*completion_callback)(void *callback_handle), void *callback_handle);



/*
 @param tracker The active rc_Tracker instance
 @param velocimeter_id The id of the velocimeter
 @param time_us Timestamp (in microseconds) corresponding to the middle of the velocimeter data integration time
 @param translation_m__s Vector of measured velocity in meters/second
 */
RCTRACKER_API bool rc_receiveVelocimeter(rc_Tracker *tracker, rc_Sensor velocimeter_id, rc_Timestamp time_us, rc_Vector translational_velocity_m__s);


/*
 @param tracker The active rc_Tracker instance
 @param accelerometer_id The id of the accelerometer
 @param time_us Timestamp (in microseconds) corresponding to the middle of the IMU data integration time
 @param acceleration_m__s2 Vector of measured acceleration in meters/second^2
 */
RCTRACKER_API bool rc_receiveAccelerometer(rc_Tracker *tracker, rc_Sensor accelerometer_id, rc_Timestamp time_us, rc_Vector acceleration_m__s2);

/*
 @param tracker The active rc_Tracker instance
 @param accelerometer_id The id of the gyroscope
 @param time_us Timestamp (in microseconds) corresponding to the middle of the IMU data integration time
 @param angular_velocity_rad__s Vector of measured angular velocity in radians/second
 */
RCTRACKER_API bool rc_receiveGyro(rc_Tracker *tracker, rc_Sensor gyro_id, rc_Timestamp time_us, rc_Vector angular_velocity_rad__s);

/*
 @param tracker The active rc_Tracker instance
 @param therm_id The id of the thermometer
 @param time_us Timestamp (in microseconds)
 @param temperature_C the measurement in degrees Celsius
 */
RCTRACKER_API bool rc_receiveTemperature(rc_Tracker *tracker, rc_Sensor therm_id, rc_Timestamp time_us, float temperature_C);

/**
 @param tracker The active rc_Tracker instance
 @param pose_m Position (in meters) relative to the camera reference frame
 Immediately after a call rc_getPose() will return pose_m.  For best
 results, call this once the tracker has converged and the confidence
 is rc_E_CONFIDENCE_MEDIUM or better rc_E_CONFIDENCE_HIGH.
 The following functions should should only be called from one of the callbacks.
 */
RCTRACKER_API void rc_setPose(rc_Tracker *tracker, const rc_Pose pose_m);
/**
 @param tracker The active rc_Tracker instance
 @param velocity Velocity (rad/s, m/s), the rotation components in in body fixed coordinates, the translational is in world coordinates (may be NULL)
 @param acceleration Position (rad/s/s, m/s/s) these are the derivatives of the velocities (may be NULL)
 @param path When RC_RUN_ASYNCHRONOUS, rc_getPose() must only be called from the rc_DataCallback(rc Data *data) with the value of data->path
*/
RCTRACKER_API rc_PoseTime rc_getPose(rc_Tracker *tracker, rc_PoseVelocity *v, rc_PoseAcceleration *a, rc_DataPath path);
RCTRACKER_API int rc_getFeatures(rc_Tracker *tracker, rc_Sensor camera_id, rc_Feature **features_px);
RCTRACKER_API rc_TrackerState rc_getState(const rc_Tracker *tracker);
RCTRACKER_API rc_TrackerConfidence rc_getConfidence(const rc_Tracker *tracker);
RCTRACKER_API rc_TrackerError rc_getError(const rc_Tracker *tracker);
/**
.Returns total length of the travelling path.
@param tracker The active rc_Tracker instance
*/
RCTRACKER_API float rc_getPathLength(rc_Tracker* tracker);

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
RCTRACKER_API bool rc_setCalibrationTM2(rc_Tracker *tracker, const void *table, size_t size);

/**
    Loads a JSON string representing calibration data into the tracker and appends it in case of any existing calibration.
*/
RCTRACKER_API bool rc_appendCalibration(rc_Tracker *tracker, const char *buffer);

/**
 Start/stop the mapping subsystem. When started, the map is completely empty. The map is build synchronously with rc_receive* startMapping must be called before loadMap
 There is only one possible instance of mapping subsystem per tracking instance.
 Subsequent calls to rc_startMapping do not create new instance nor reset mapping but apply other input parameters.
 rc_stopMapping removes existing mapping sub-system.
 */
RCTRACKER_API void rc_startMapping(rc_Tracker *tracker, bool relocalize, bool save_map, bool allow_jumps);
RCTRACKER_API void rc_stopMapping(rc_Tracker *tracker);

typedef void   (*rc_SaveCallback)(void *handle, const void *buffer, size_t length);
typedef size_t (*rc_LoadCallback)(void *handle, void *buffer, size_t length);
/**
 Save/load a map to use.
 */
RCTRACKER_API void rc_saveMap(rc_Tracker *tracker, rc_SaveCallback write, void *handle);
RCTRACKER_API bool rc_loadMap(rc_Tracker *tracker, rc_LoadCallback read, void *handle);

#include <math.h>
#include <float.h>
inline rc_Quaternion rc_quaternionExp(rc_Vector v)
{
    rc_Vector w = {{ v.x/2, v.y/2, v.z/2 }};
    float th2, th = sqrtf(th2 = w.x*w.x + w.y*w.y + w.z*w.z), c = cosf(th), s = th2 < sqrtf(120*FLT_EPSILON) ? 1-th2/6 : sinf(th)/th;
    rc_Quaternion Q = {{ s*w.x, s*w.y, s*w.z, c }};
    return Q;
}

inline rc_Quaternion rc_quaternionMultiply(rc_Quaternion a, rc_Quaternion b)
{
    rc_Quaternion Q = {{
        a.x * b.w + a.w * b.x - a.z * b.y + a.y * b.z,
        a.y * b.w + a.z * b.x + a.w * b.y - a.x * b.z,
        a.z * b.w - a.y * b.x + a.x * b.y + a.w * b.z,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
    }};
    return Q;
}

inline rc_Pose rc_predictPose(rc_Timestamp dt_us, const rc_Pose p, const rc_PoseVelocity v, const rc_PoseAcceleration a)
{
    float dt = dt_us * 1e-6f;
    rc_Pose P = {{{0}}};
    P.T.x = dt * (dt/2 * a.T.x + v.T.x) + p.T.x;
    P.T.y = dt * (dt/2 * a.T.y + v.T.y) + p.T.y;
    P.T.z = dt * (dt/2 * a.T.z + v.T.z) + p.T.z;
    rc_Vector W = {{
            dt * (dt/2 * a.W.x + v.W.x),
            dt * (dt/2 * a.W.y + v.W.y),
            dt * (dt/2 * a.W.z + v.W.z),
    }};
    P.Q = rc_quaternionMultiply(rc_quaternionExp(W), p.Q);
    return P;
}

#ifdef __cplusplus
}
#endif

#endif
