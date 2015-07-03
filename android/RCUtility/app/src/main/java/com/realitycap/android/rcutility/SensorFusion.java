package com.realitycap.android.rcutility;

import android.hardware.Camera;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.util.Log;

import java.nio.ByteBuffer;

public class SensorFusion implements SensorEventListener, PreviewCallback, ISyncedFrameReceiver
{
	static
	{
		System.loadLibrary("sensor-fusion");
	}
	
	public native boolean startSensorFusion();
	public native void stopSensorFusion();
	public native boolean startStaticCalibration();
	public native boolean startCapture();
	public native void stopCapture();
	
	protected native void receiveAccelerometer(float x, float y, float z, long timestamp);
	protected native void receiveGyro(float x, float y, float z, long timestamp);
	protected native boolean receiveVideoFrame(byte[] data);
    protected native boolean receiveSyncedFrames(ByteBuffer colorData, ByteBuffer depthData);
	
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
	
	@Override
	public void onPreviewFrame(final byte[] data, Camera camera)
	{
		boolean result = receiveVideoFrame(data);
		if (!result) Log.w(MyApplication.TAG, "receiveVideoFrame() returned FALSE");
		camera.addCallbackBuffer(data);
	}

    @Override
    public void onSyncedFrames(final ByteBuffer colorData, final ByteBuffer depthData)
    {
        boolean result = receiveSyncedFrames(colorData, depthData);
        if (!result) Log.w(MyApplication.TAG, "receiveSyncedFrames() returned FALSE");
    }
	
	protected void onSensorFusionStatusUpdate(SensorFusionStatus status)
	{
//		Log.d(MyApplication.TAG, String.format("runState: %d progress: %f", status.runState, status.progress));
	}
	
	protected void onSensorFusionDataUpdate(SensorFusionData data)
	{
//		Log.d(MyApplication.TAG, String.format("onSensorFusionDataUpdate %d", data.timestamp));
	}
}
