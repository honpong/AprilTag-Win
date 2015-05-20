package com.realitycap.android.rcutility;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;

public class IMUManager
{
	private static final int SENSOR_REPORT_RATE = 10000; // 100hz
	protected SensorManager mSensorManager;
	protected SensorEventListener mListener;
	
	public IMUManager()
	{
		Context context = MyApplication.getContext();
		mSensorManager = (SensorManager) context.getSystemService(Context.SENSOR_SERVICE);
	}
	
	public void startSensors()
	{
		final Sensor accelerometer = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
		if (accelerometer != null && mListener != null) mSensorManager.registerListener(mListener, accelerometer, SENSOR_REPORT_RATE);
		
		final Sensor gyro = mSensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE_UNCALIBRATED);
		if (gyro != null && mListener != null) mSensorManager.registerListener(mListener, gyro, SENSOR_REPORT_RATE);
	}
	
	public void stopSensors()
	{
		if (mListener != null) mSensorManager.unregisterListener(mListener);
	}
	
	public SensorEventListener getSensorEventListener()
	{
		return mListener;
	}
	
	public void setSensorEventListener(SensorEventListener listener)
	{
		mListener = listener;
	}
}
