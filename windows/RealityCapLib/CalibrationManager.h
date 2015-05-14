#pragma once

#include "SensorManager.h"

namespace RealityCap
{
	class CalibrationManagerDelegate
	{
	protected:
		CalibrationManagerDelegate() {};
	public:
		virtual void OnProgressUpdated(float progress) {};
		virtual void OnStatusUpdated(int status) {};
	};

	class CalibrationManager : virtual public SensorManager
	{
	public:
		CalibrationManager();
		bool StartCalibration();
		void StopCalibration();
		bool isCalibrating();
		void SetDelegate(CalibrationManagerDelegate* del);

	protected:
		virtual void OnColorFrame(PXCImage* colorImage);
		virtual void OnAmeterSample(struct imu_sample* sample);
		virtual void OnGyroSample(struct imu_sample* sample);

	private:		
		bool _isCalibrating;
		CalibrationManagerDelegate* _delegate;
	};
}