#ifndef SP_CALIBRATION_H
#define SP_CALIBRATION_H

typedef struct SP_Calibration
{
	/// @param[out] deviceName
	/// Device name/model
	char deviceName[256];

	/// @param[out] calibrationVersion
	/// Version of the calibration file
	int calibrationVersion;

	/// @param[out] imageWidth
	/// Color camera's image width in pixels
	int image_width;

	/// @param[out] imageHeight
	/// Color camera's image height in pixels
	int image_height;

	/// @param[out] Fx
	/// Color camera's horizontal focal length in pixels
	float Fx;
	/// @param[out] Fy
	/// Color camera's vertical focal length in pixels
	float Fy;

	/// @param[out] Cx
	/// Optical center of camera, x-dimension in pixels
	float Cx;
	/// @param[out] Cy
	/// Optical center of camera, y-dimension in pixels
	float Cy;
	/// @param[out] px
	/// camera tangential distortion parameters in x-dimension
	float px;
	/// @param[out] py
	/// camera tangential distortion parameters in y-dimension
	float py;

	/// @param[out] distortionModel
	/// Default value is 0. For fisheye, value of distortion model is 1
	int distortionModel; 

	/// @param[out] K0
	/// radial distortion params
	float K0; 
	/// @param[out] K1
	/// radial distortion params
	float K1;
	/// @param[out] K2
	/// radial distortion params
	float K2;
	/// @param[out] Kw
	/// fisheye distortion one-parameter model
	float Kw;

    /// @param[out] a_bias
    /// Accelerometer bias
    float a_bias[3];
    
    /// @param[out] a_bias_var
    /// Variance of accelerometer bias
    float a_bias_var[3];
    
    /// @param[out] w_bias
    /// Gyroscope bias
    float w_bias[3];

    /// @param[out] w_bias_var
    /// Variance of gyroscope bias
    float w_bias_var[3];

	/// @param[out] Tc
    /// translation between camera and accelerometer in meters
    float Tc[3];
    /// @param[out] TcVar
    /// Variances of Tc
    float Tc_var[3];

    /// @param[out] Wc
    /// Rotation between camera and accelerometer, in radians
    float Wc[3];

    /// @param[out] WcVar
    /// Variances of Wc
    float Wc_var[3];

	/// @param[out] aMeasVar
	/// Accelerometer measurement noise variance
	float a_meas_var;

	/// @param[out] wMeasVar
	/// Gyroscope measurement noise variance
	float w_meas_var;

	/// @param[out] shutterDelay
	/// Camera shutter's delay time
	float shutterDelay;
	/// @param[out] shutterPeriod
	/// Camera shutter's period
	float shutterPeriod;

	/// @param[out] timeStampOffset
	/// Timestamp offset between IMU timestamp reference - Color camera timestamp reference
	/// in micro-seconds
	float timeStampOffset;

	/// @param[out] gyroscopeTransform
	/// Rotation transformation applicable to gyroscope samples
	float gyroscopeTransform[9];

	/// @param[out] accelerometerTransform
	/// Rotation transformation applicable to accelerometer samples
	float accelerometerTransform[9];
} SP_Calibration;
typedef struct SP_Calibration rcCalibration;
#endif //SP_CALIBRATION_H