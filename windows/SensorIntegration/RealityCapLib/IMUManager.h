#using <Windows.winmd>
#using <Platform.winmd>
#include <memory>
//#include "AccelerometerManager.h"

#pragma once

namespace RealityCap
{
	public ref class IMUManager sealed
	{
	public:
		IMUManager();
		static IMUManager^ GetSharedInstance();
		bool StartSensors(); // returns true if all sensors started successfully
		void StopSensors();

	private:
		Windows::Devices::Sensors::Accelerometer^ accelerometer;
		Windows::Devices::Sensors::Gyrometer^ gyro;
		Windows::Foundation::EventRegistrationToken accelToken;
		Windows::Foundation::EventRegistrationToken gyroToken;
		Windows::UI::Xaml::DispatcherTimer^ dispatcherTimer;
		//AccelerometerManager accelMan;

		void ReadAccelerometerData(Object^ sender, Object^ e);
		void AccelReadingChanged(Windows::Devices::Sensors::Accelerometer^ sender, Windows::Devices::Sensors::AccelerometerReadingChangedEventArgs^ e);
		void GyroReadingChanged(Windows::Devices::Sensors::Gyrometer^ sender, Windows::Devices::Sensors::GyrometerReadingChangedEventArgs^ e);
	};
}


