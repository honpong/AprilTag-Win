/********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
terms of a license agreement or nondisclosure agreement with Intel Corporation
and may not be copied or disclosed except in accordance with the terms of that
agreement.
Copyright(c) 2011-2016 Intel Corporation. All Rights Reserved.

*********************************************************************************/

#ifndef SP_TYPE_H
#define SP_TYPE_H

#include <stdint.h>

/// status of API function calls.
typedef enum SP_STATUS : int
{
    SP_STATUS_SUCCESS,
    SP_STATUS_ERROR,
    SP_STATUS_WARNING,
    SP_STATUS_INVALIDARG,
    SP_STATUS_NOT_CONFIGURED,
    SP_STATUS_PLATFORM_NOT_SUPPORTED
} SP_STATUS;

/// status indicating tracking accuracy
typedef enum SP_TRACKING_ACCURACY : int
{
    LOW,
    MED,
    HIGH,
    FAILED
} SP_TRACKING_ACCURACY;

/// A struct to represent a sample of inertial sensor units.
typedef struct
{
    /// @param timestamp
    /// Time stamp of the IMU sample in microseconds.
    /// If the value is zero, the sample should be
    /// considered as invalid.
    int64_t    timestamp;

    /// @param data
    /// Three dimensional sample data in x, y, z
    /// or yaw, pitch, roll.
    float      data[3];
}
SP_ImuSample;


const unsigned SP_IMU_RING_BUFFER_SAMPLE_COUNT = 16; // The number of samples attached (for each sensor type)


/// A struct to represent input samples for Scene Perception.
/// All timestamps must be in the same time system and inputs should be provided
/// with proper timestamps. A timestamp value of 0 is invalid.
typedef struct
{
    int64_t depthTimestamp;                     /*!< time stamp in microseconds for the depth image. */
	unsigned short * pDepthSrc;                 /*!< pointer to a depth source data (required).*/
	int64_t fisheyeOrRGBTimestamp;                   /*!< time stamp in microseconds for the fisheye image. */
    unsigned char*  pFisheyeOrRGBSrc;                /*!< pointer to a fisheye source data (required). */
    SP_ImuSample * pGravitySamples;            // pointer to gravity samples (optional NULL).
    int numberOfGravitySamples;                // number of available gravity samples
    SP_ImuSample * pLinearAccelerationSamples;  /*!< pointer to acceleration samples.*/
    int numberOfLinearAccelerationSamples;      /*!< number of available linear acceleration samples.*/
    SP_ImuSample * pAngularVelocitySamples;     /*!< pointer to angular velocity samples.*/
    int numberOfAngularVelocitySamples;         /*!< number of available angular velocity samples.*/
}
SP_InputStream;


/// Camera pose is represented in a 3 by 4 matrix format [R | T], where
/// R = [ r11 r12 r13 ]
///     [ r21 r22 r23 ]
///     [ r31 r32 r33 ]
/// is a rotation matrix and
/// T = [tx ty tz], is a translation vector whose values are in meter.
/// SP_Pose is represented by a 12-element array in row-major order:
/// [r11 r12 r13 tx r21 r22 r23 ty r31 r32 r33 tz]
typedef float SP_Pose[12];

/// A struct to represent feature point that is being tracked.
/// It includes world coordinate values and also image coordinate values.
typedef struct
{
    uint64_t id;                     /*!< id of the feature, applicable during a tracking session (until the next configuration/reset event). */
    float world_x, world_y, world_z; /*!< world coordinate values in meter. */
    float image_x, image_y;          /*!< image coordinate values in pixel. */
} SP_Feature;

/// Tracking status that includes camera pose, accuracy and timestamp of tracking, and whether
/// the pose is a result of re-localization.
typedef struct
{
    float pose[12]; /*!< camera pose output of tracking. */
    SP_TRACKING_ACCURACY accuracy; /*!< accuracy of tracking. */
    int isRelocalizationPose; /*!< indicates if there is a re-localization (value 1). */
    int64_t timeStamp; /*!< timestamp of tracking results that is in the same time system as of inputs' timestamps. */
}
SP_TrackingStatus;

/// <summary>
/// TrackingCallback defines type of callback function to receives tracking result update.
/// </summary>
///
/// <param name="trackingResult"> : a struct that carries details of tracking results. Tracking results
/// include:
/// - camera pose values. Values and memory of the pose struct member are ONLY guaranteed to be valid within
/// the scope of the function call. Pose values are valid only when tracking accuracy is not FAILED.
/// - accuracy of tracking result and can equal one of the following values:
/// MED: Indicates medium confidence in camera pose estimation.
/// LOW: Indicates low confidence in camera pose estimation.
/// FAILED: Indicates tracking failure where camera pose values are not valid result.
/// - whether tracking result is a re-localization result.
/// - timestamp of tracking result.
/// </param>
///
typedef void(*TrackingCallback)
(
    SP_TrackingStatus trackingResult
);

typedef struct
{
    float v[3][3];
} SP_Matrix;

typedef union
{
    struct
    {
        float x, y, z;
    };
    float v[3];
} SP_Vector;

/// SP_DISTORTION_MODEL specifies distortion model for image.
typedef enum SP_DISTORTION_MODEL : int
{
    SP_DISTORTION_UNKNOWN, /**< Unknown distortion model.*/
    SP_DISTORTION_FISHEYE, /**< Distortion model for fisheye camera.*/
    SP_DISTORTION_POLYNOMIAL3, /**< Polynomial distortion model. */
    SP_UNDISTORTED /**< Image is not distorted.*/
}
SP_DISTORTION_MODEL;

/// A struct to represent intrinsic parameters of camera sensor.
typedef struct
{
    SP_DISTORTION_MODEL distortionType; /*!< distortionType : type of distortion of camera images. */
    unsigned int imageWidth; /*!< imageWidth : width of camera images. */
    unsigned int imageHeight; /*!< imageHeight : height of camera images. */
    float focalLengthHorizontal; /*!< focalLengthHorizontal : focal length along x axis of Camera Sensor in pixels. */
    float focalLengthVertical; /*!< focalLengthVertical : focal length along y axis of Camera Sensor in pixels. */
    float principalPointCoordU; /*!< principalPointCoordU : x-component of principle point of Camera Sensor in pixels. */
    float principalPointCoordV; /*!< principalPointCoordV : y-component of principle point of Camera Sensor in pixels. */
    union
    {
        float distortion[3]; /*!< camera distortion parameter values. */
        struct
        {
            float k1, k2, k3;
        }; /*!< polynomial distortion parameters. */
        float w; /*!< Fisheye camera field of view in radians (half-angle FOV).*/
    };
}
SP_CameraIntrinsics;

/// A struct to represent camera extrinsic parameters that include rotation and translation
/// extrinsic parameters.
typedef struct
{
    SP_Vector T; /*!< translation extrinsic parameters. */
    SP_Matrix R; /*!< rotation extrinsic parameters. */
} SP_CameraMatrix;

/// A struct to represent camera extrinsic parameters that include rotation and translation
/// extrinsic parameters as well as their variances.
typedef struct
{
    SP_CameraMatrix pose; /*!< translation and rotation extrinsic parameters. */
    SP_CameraMatrix variance; /*!< variance of translation and rotation extrinsic parameters. */
}
SP_CameraExtrinsics;

/// A struct to represent calibration data of IMU sensor, including value transformation, biases,
/// variances of biases, and noise variance.
typedef struct
{
    SP_Matrix scaleAndAlign; /*!< includes scale, cross axis alignment/cross axis sensitivity in row major order:
                                 ScaleX  Y_over_X  Z_over_X
                                 X_over_Y  ScaleY  Z_over_Y
                                 Y_over_Z  X_over_Z  ScaleZ */
    SP_Vector bias; /*!< biases in x, y and z axes and in radian per second or meter per second square for gyroscope and accelerometer
                    samples respectively. */
    SP_Vector biasVariance; /*!< variances of biases in x, y and z axes. */
    float noiseVariance; /*!< noise variance, applicable to x, y and z axes and in unit of rad^2 / s^2 and m^2 / s^4 for
                         gyroscope and accelerometer samples respectively. */
}
SP_IMUIntrinsics;

/// The number of sensors in the calibration configuration per sensor type.
/// The value could be changed in the future.
const int SP_NUM_SENSORS = 1;

/// A struct to represent calibration data, including depth and fisheye cameras' intrinsic and
/// extrinsic parameters to accelerometer sensor, as well as calibration data of accelerometer
/// and gyroscope sensors such as scales, cross axis alignment values, biases, variances of biases,
/// and noise variance.
typedef struct
{
    SP_CameraIntrinsics pFisheyeIntrinsics[SP_NUM_SENSORS]; /*!< intrinsic parameters for fisheye camera. */
    SP_CameraExtrinsics pFisheyeExtrinsics[SP_NUM_SENSORS]; /*!< extrinsic parameters for fisheye camera w.r.t IMU sensor. */
    SP_CameraIntrinsics pDepthIntrinsics[SP_NUM_SENSORS];       /*!< intrinsic parameters for depth camera. */
    SP_CameraExtrinsics pDepthExtrinsics[SP_NUM_SENSORS]; /*!< extrinsic parameters for depth camera  w.r.t IMU sensor. */
    SP_IMUIntrinsics pAccelerometerIntrinsics[SP_NUM_SENSORS]; /*!< calibration data for accelerometer. */
    SP_IMUIntrinsics pGyroscopeIntrinsics[SP_NUM_SENSORS]; /*!< calibration data for gyroscope. */
}
SP_CalibrationConfiguration;

typedef enum SLAM_MODE_FLAGS
{
    SLAM_MODE_SYNCHRONOUS,
    SLAM_MODE_ASYNCHRONOUS,
} SLAM_MODE_FLAGS;
#endif //SP_TYPE_H