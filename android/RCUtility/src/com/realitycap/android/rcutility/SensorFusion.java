package com.realitycap.android.rcutility;

import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.util.Log;

public class SensorFusion implements SensorEventListener
{
	static
	{
		System.loadLibrary("sensor-fusion");
	}
	
	public native String  stringFromJNI();
	public native void receiveAccelerometer(float x, float y, float z, long timestamp);
	public native void receiveGyro(float x, float y, float z, long timestamp);
	
	public SensorFusion()
	{
		
	}
	
	@Override
	public void onSensorChanged(SensorEvent sensorEvent)
	{
		if (sensorEvent.sensor.getType() == Sensor.TYPE_ACCELEROMETER)
		{
//			Log.d(MyApplication.TAG, String.format("accel %.3f, %.3f, %.3f", sensorEvent.values[0], sensorEvent.values[1], sensorEvent.values[2]));
			receiveAccelerometer(sensorEvent.values[0], sensorEvent.values[1], sensorEvent.values[2], sensorEvent.timestamp);
		}
		else if (sensorEvent.sensor.getType() == Sensor.TYPE_GYROSCOPE || sensorEvent.sensor.getType() == Sensor.TYPE_GYROSCOPE_UNCALIBRATED)
		{
//			Log.d(MyApplication.TAG, String.format("gyro %.3f, %.3f, %.3f", sensorEvent.values[0], sensorEvent.values[1], sensorEvent.values[2]));
			receiveGyro(sensorEvent.values[0], sensorEvent.values[1], sensorEvent.values[2], sensorEvent.timestamp);
		}
	}
	
	@Override
	public void onAccuracyChanged(Sensor sensor, int accuracy)
	{
		Log.d(MyApplication.TAG, String.format("onAccuracyChanged(%s, %d)", sensor.getName(), accuracy));
	}
}
