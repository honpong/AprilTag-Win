#include "pch.h"
#include "IMUManager.h"
#include "Debug.h"
//#include "AccelerometerManager.h"
#using <Windows.winmd>
#using <Platform.winmd>

using namespace Windows::Devices::Sensors;
using namespace Windows::Foundation;
using namespace Platform;
using namespace RealityCap;
using namespace Windows::UI::Xaml;

#define USE_WIN32_AMETER_API true
static const int DESIRED_INTERVAL = 10; // desired sensor reporting interval in milliseconds

IMUManager::IMUManager()
{
	/*Debug::Log(L"IMUManager");

	if (USE_WIN32_AMETER_API)
	{
		accelMan.Initialize();
		accelMan.SetChangeSensitivity(0);
	}
	else
	{
		accelerometer = Accelerometer::GetDefault();
		if (accelerometer == nullptr) Debug::Log(L"ERROR: No accelerometer found");
	}
	
	gyro = Gyrometer::GetDefault();
	if (gyro == nullptr) Debug::Log(L"ERROR: No gyro found");*/
}

IMUManager^ IMUManager::GetSharedInstance()
{
	Debug::Log(L"GetSharedInstance");
	static IMUManager^ _sharedInstance;
	if (_sharedInstance == nullptr)
	{
		Debug::Log(L"Instantiating shared IMUManager instance");
		_sharedInstance = ref new IMUManager();
	}
	return _sharedInstance;
}

bool IMUManager::StartSensors()
{
	/*Debug::Log(L"StartSensors");*/
	bool result = true;

	//if (USE_WIN32_AMETER_API)
	//{
	//	// Set up a DispatchTimer
	//	//TimeSpan span;
	//	//span.Duration = static_cast<int32>(DESIRED_INTERVAL) * 10000;   // convert to 100ns ticks
	//	//dispatcherTimer = ref new DispatcherTimer();
	//	//dispatcherTimer->Interval = span;
	//	//dispatcherTimer->Tick += ref new Windows::Foundation::EventHandler<Object^>(this, &IMUManager::ReadAccelerometerData);
	//	//dispatcherTimer->Start();
	//}
	//else // subscribe to updates
	//{
	//	if (accelerometer != nullptr)
	//	{
	//		accelerometer->ReportInterval = accelerometer->MinimumReportInterval > DESIRED_INTERVAL ? accelerometer->MinimumReportInterval : DESIRED_INTERVAL;  // milliseconds
	//		Debug::Log(L"accel reporting interval: %ims", accelerometer->ReportInterval);
	//		accelToken = accelerometer->ReadingChanged::add(ref new TypedEventHandler<Accelerometer^, AccelerometerReadingChangedEventArgs^>(this, &IMUManager::AccelReadingChanged));
	//	}
	//	else
	//		result = false;
	//}

	//if (gyro != nullptr)
	//{
	//	gyro->ReportInterval = gyro->MinimumReportInterval > DESIRED_INTERVAL ? gyro->MinimumReportInterval : DESIRED_INTERVAL;  // milliseconds
	//	Debug::Log(L"gyro reporting interval: %ims", gyro->ReportInterval);
	//	gyroToken = gyro->ReadingChanged::add(ref new TypedEventHandler<Gyrometer^, GyrometerReadingChangedEventArgs^>(this, &IMUManager::GyroReadingChanged));
	//}
	//else
	//	result = false;

	return result;
}

void IMUManager::StopSensors()
{
	//if (USE_WIN32_AMETER_API && dispatcherTimer != nullptr)
	//{
	//	dispatcherTimer->Stop();
	//}
	//else if (accelerometer != nullptr)
	//{
	//	accelerometer->ReadingChanged::remove(accelToken);
	//	accelerometer->ReportInterval = 0; // Restore the default report interval to release resources while the sensor is not in use
	//}

	//if (gyro != nullptr)
	//{
	//	gyro->ReadingChanged::remove(gyroToken);
	//	gyro->ReportInterval = 0; // Restore the default report interval to release resources while the sensor is not in use
	//}
}

// polling
void IMUManager::ReadAccelerometerData(Object^ sender, Object^ e)
{
	/*AccelerometerReading^ reading = accelerometer->GetCurrentReading();
	String^ timestamp = reading->Timestamp.UniversalTime.ToString();
	Debug::Log(L"%s\taccel\tx: %1.3f\ty: %1.3f\tz: %1.3f", timestamp->Data(), reading->AccelerationX, reading->AccelerationY, reading->AccelerationZ);*/

	/*AccelSample sample = accelMan.GetSample();
	Debug::Log(L"%i\taccel\tx: %1.3f\ty: %1.3f\tz: %1.3f", sample.timestamp.wMilliseconds, sample.x, sample.y, sample.z);*/
}

// subscription
void IMUManager::AccelReadingChanged(Accelerometer^ sender, AccelerometerReadingChangedEventArgs^ e)
{
	/*AccelerometerReading^ reading = e->Reading;
	String^ timestamp = reading->Timestamp.UniversalTime.ToString();
	Debug::Log(L"%s\taccel\tx: %1.3f\ty: %1.3f\tz: %1.3f", timestamp->Data(), reading->AccelerationX, reading->AccelerationY, reading->AccelerationZ);*/
}

void IMUManager::GyroReadingChanged(Gyrometer^ sender, GyrometerReadingChangedEventArgs^ e)
{
	/*GyrometerReading^ reading = e->Reading;
	String^ timestamp = reading->Timestamp.UniversalTime.ToString();
	Debug::Log(L"%s\tgyro\tx: %1.3f\ty: %1.3f\tz: %1.3f", timestamp->Data(), reading->AngularVelocityX, reading->AngularVelocityY, reading->AngularVelocityZ);*/
}