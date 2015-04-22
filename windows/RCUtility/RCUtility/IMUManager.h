#pragma once

#using <Windows.winmd>
#using <Platform.winmd>
#include "AccelerometerManager.h"

namespace RealityCap
{
	public ref class AmeterSample sealed
	{
	public:
		AmeterSample();
		property double AccelerationX;
		property double AccelerationY;
		property double AccelerationZ;
		property uint64 Timestamp;
	};

	public delegate void AmeterEventHandler(Windows::Devices::Sensors::AccelerometerReading^ sample);
	public delegate void GyroEventHandler(Windows::Devices::Sensors::GyrometerReading^ sample);

	ref class IMUManager sealed
	{
	public:
		IMUManager();
		bool StartSensors(); // returns true if all sensors started successfully
		void StopSensors();

		event AmeterEventHandler^ OnAmeterSample;
		event GyroEventHandler^ OnGyroSample;

	private:
		Windows::Devices::Sensors::Accelerometer^ accelerometer;
		Windows::Devices::Sensors::Gyrometer^ gyro;
		Windows::Foundation::EventRegistrationToken accelToken;
		Windows::Foundation::EventRegistrationToken gyroToken;
		AccelerometerManager accelMan;

		void ReadAccelerometerData(Object^ sender, Object^ e);
		void AccelReadingChanged(Windows::Devices::Sensors::Accelerometer^ sender, Windows::Devices::Sensors::AccelerometerReadingChangedEventArgs^ e);
		void GyroReadingChanged(Windows::Devices::Sensors::Gyrometer^ sender, Windows::Devices::Sensors::GyrometerReadingChangedEventArgs^ e);
	};
}


