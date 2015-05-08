package com.realitycap.android.rcutility;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.util.Log;

public class IMUManager implements SensorEventListener
{
	private static final int SENSOR_REPORT_RATE = 10000; // 100hz
	private SensorManager mSensorManager;
	
	public IMUManager()
	{
		Context context = MyApplication.getContext();
		mSensorManager = (SensorManager) context.getSystemService(Context.SENSOR_SERVICE);
	}
	
	public void startSensors()
	{
		final Sensor accelerometer = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
		if (accelerometer != null) mSensorManager.registerListener(this, accelerometer, SENSOR_REPORT_RATE);
		
		final Sensor gyro = mSensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
		if (gyro != null) mSensorManager.registerListener(this, gyro, SENSOR_REPORT_RATE);
	}
	
	public void stopSensors()
	{
		mSensorManager.unregisterListener(this);
	}
	
	@Override
	public void onSensorChanged(SensorEvent sensorEvent)
	{
		if (sensorEvent.sensor.getType() == Sensor.TYPE_ACCELEROMETER)
		{
//			Log.d(MyApplication.TAG, String.format("accel %.3f, %.3f, %.3f", sensorEvent.values[0], sensorEvent.values[1], sensorEvent.values[2]));
		}
		else if (sensorEvent.sensor.getType() == Sensor.TYPE_GYROSCOPE || sensorEvent.sensor.getType() == Sensor.TYPE_GYROSCOPE_UNCALIBRATED)
		{
//			Log.d(MyApplication.TAG, String.format("gyro %.3f, %.3f, %.3f", sensorEvent.values[0], sensorEvent.values[1], sensorEvent.values[2]));
		}
	}
	
	@Override
	public void onAccuracyChanged(Sensor sensor, int accuracy)
	{
		Log.d(MyApplication.TAG, String.format("onAccuracyChanged(%s, %d)", sensor.getName(), accuracy));
	}
}
