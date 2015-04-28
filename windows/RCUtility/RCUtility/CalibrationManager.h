#pragma once

#using <Windows.winmd>
#using <Platform.winmd>

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

	public ref class CalibrationManager sealed
	{
	public:
		CalibrationManager();
		bool StartCalibration();
		void StopCalibration();
		bool isCalibrating();
		void SetDelegate(Platform::IntPtr del);

	private:		
		bool _isCalibrating;
		CalibrationManagerDelegate* _delegate;
		SensorManager^ sensorMan;
	};
}