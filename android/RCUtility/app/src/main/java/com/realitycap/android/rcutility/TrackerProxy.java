package com.realitycap.android.rcutility;

import android.hardware.Camera;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.util.Log;

import java.nio.ByteBuffer;

public class TrackerProxy implements SensorEventListener, ISyncedFrameReceiver
{
	static
	{
		System.loadLibrary("tracker_wrapper");
	}
	
	public native boolean start();
	public native void stop();
	public native boolean startCalibration();
	public native boolean startReplay();
	public native void setOutputLog();
	
	protected native void receiveAccelerometer(float x, float y, float z, long timestamp);
	protected native void receiveGyro(float x, float y, float z, long timestamp);
    protected native boolean receiveSyncedFrames(ByteBuffer colorData, ByteBuffer depthData);
	
	public TrackerProxy()
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

    @Override
    public void onSyncedFrames(final ByteBuffer colorData, final ByteBuffer depthData)
    {
        boolean result = receiveSyncedFrames(colorData, depthData);
        if (!result) Log.w(MyApplication.TAG, "receiveSyncedFrames() returned FALSE");
    }
	
	protected void onStatusUpdated(SensorFusionStatus status)
	{
//		Log.d(MyApplication.TAG, String.format("runState: %d progress: %f", status.runState, status.progress));
	}
	
	protected void onProgressUpdated(SensorFusionData data)
	{
//		Log.d(MyApplication.TAG, String.format("onProgressUpdated %d", data.timestamp));
	}
}
