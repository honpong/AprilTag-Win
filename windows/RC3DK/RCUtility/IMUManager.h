#pragma once

#using <Windows.winmd>
#using <Platform.winmd>

namespace RealityCap
{
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
		//AccelerometerManager accelMan;

		void AccelReadingChanged(Windows::Devices::Sensors::Accelerometer^ sender, Windows::Devices::Sensors::AccelerometerReadingChangedEventArgs^ e);
		void GyroReadingChanged(Windows::Devices::Sensors::Gyrometer^ sender, Windows::Devices::Sensors::GyrometerReadingChangedEventArgs^ e);
	};
}


