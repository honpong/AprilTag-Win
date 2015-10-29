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
	int imageWidth;

	/// @param[out] imageHeight
	/// Color camera's image height in pixels
	int imageHeight;

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

	/// @param[out] abias0
	/// Accelerometer bias
	float abias0;
	/// @param[out] abias1
	/// Accelerometer bias
	float abias1;
	/// @param[out] abias2
	/// Accelerometer bias
	float abias2;

	/// @param[out] wbias0
	/// Gyroscope bias
	float wbias0;
	/// @param[out] wbias1
	/// Gyroscope bias
	float wbias1;
	/// @param[out] wbias2
	/// Gyroscope bias
	float wbias2;

	/// @param[out] Tc0
	/// X-dimension of translation between camera and accelerometer in meters
	float Tc0;
	/// @param[out] Tc1
	/// Y-dimension of translation between camera and accelerometer in meters
	float Tc1;
	/// @param[out] Tc2
	/// Z-dimension of translation between camera and accelerometer in meters
	float Tc2;

	/// @param[out] Wc0
	/// Rotation between camera and accelerometer, around x-axis in radians
	float Wc0;
	/// @param[out] Wc1
	/// Rotation between camera and accelerometer, around y-axis in radians
	float Wc1;
	/// @param[out] Wc2
	/// Rotation between camera and accelerometer, around z-axis in radians
	float Wc2;

	/// @param[out] abiasvar0
	/// Variance of accelerometer bias
	float abiasvar0;
	/// @param[out] abiasvar1
	/// Variance of accelerometer bias
	float abiasvar1;
	/// @param[out] abiasvar2
	/// Variance of accelerometer bias
	float abiasvar2;

	/// @param[out] wbiasvar0
	/// Variance of gyroscope bias
	float wbiasvar0;
	/// @param[out] wbiasvar1
	/// Variance of gyroscope bias
	float wbiasvar1;
	/// @param[out] wbiasvar2
	/// Variance of gyroscope bias
	float wbiasvar2;

	/// @param[out] TcVar0
	/// Variances of Tc
	float TcVar0;
	/// @param[out] TcVar1
	/// Variances of Tc
	float TcVar1;
	/// @param[out] TcVar2
	/// Variances of Tc
	float TcVar2;

	/// @param[out] WcVar0
	/// Variances of Wc
	float WcVar0;
	/// @param[out] WcVar1
	/// Variances of Wc
	float WcVar1;
	/// @param[out] WcVar2
	/// Variances of Wc
	float WcVar2;

	/// @param[out] aMeasVar
	/// Accelerometer measurement noise variance
	float aMeasVar;

	/// @param[out] wMeasVar
	/// Gyroscope measurement noise variance
	float wMeasVar;


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

} SP_Calibration;
typedef struct SP_Calibration rcCalibration;
#endif //SP_CALIBRATION_H