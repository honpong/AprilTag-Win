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

    public static final int CAMERA_EGRAY8 = 0;
    public static final int CAMERA_EDEPTH16 = 1;

    public native boolean createTracker();
	public native boolean destroyTracker();
	public native boolean startTracker();
	public native void stopTracker();
	public native boolean startCalibration();
	public native boolean startReplay();
	public native boolean setOutputLog();
    public native void configureCamera(int camera, int width_px, int height_px, float center_x_px, float center_y_px, float focal_length_x_px, float focal_length_y_px, float skew, boolean fisheye, float fisheye_fov_radians);
	public native void setOutputLog(String filename);
    
	protected native void receiveAccelerometer(float x, float y, float z, long timestamp);
	protected native void receiveGyro(float x, float y, float z, long timestamp);
    protected native boolean receiveImageWithDepth(long time_us, long shutter_time_us, boolean force_recognition, int width, int height, int stride, ByteBuffer colorData, int depthWidth, int depthHeight, int depthStride, ByteBuffer depthData);
	
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
    public void onSyncedFrames(long time_us, long shutter_time_us, int width, int height, int stride, final ByteBuffer colorData, int depthWidth, int depthHeight, int depthStride, final ByteBuffer depthData)
    {
        boolean result = receiveImageWithDepth(time_us, shutter_time_us, false, width, height, stride, colorData, depthWidth, depthHeight, depthStride, depthData);
        if (!result) Log.w(MyApplication.TAG, "receiveImageWithDepth() returned FALSE");
    }
	
	protected void onStatusUpdated(SensorFusionStatus status)
	{
		Log.d(MyApplication.TAG, String.format("runState: %d progress: %f", status.runState, status.progress));
	}
}
