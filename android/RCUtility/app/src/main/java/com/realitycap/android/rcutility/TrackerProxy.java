package com.realitycap.android.rcutility;

import android.hardware.Camera;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.util.Log;

import java.nio.ByteBuffer;

public class TrackerProxy implements SensorEventListener, ISyncedFrameReceiver, ITrackerReceiver
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
    public native String getCalibration();
    public native boolean setCalibration(String cal);

    protected native void receiveAccelerometer(float x, float y, float z, long timestamp);
    protected native void receiveGyro(float x, float y, float z, long timestamp);
    protected native boolean receiveImageWithDepth(long time_us, long shutter_time_us, boolean force_recognition, int width, int height, int stride, ByteBuffer colorData, int depthWidth, int depthHeight, int depthStride, ByteBuffer depthData);

    protected ITrackerReceiver receiver;

    public TrackerProxy()
    {

    }

    public ITrackerReceiver getReceiver()
    {
        return receiver;
    }

    public void setReceiver(ITrackerReceiver receiver)
    {
        this.receiver = receiver;
    }

    // SensorEventListener interface

    @Override
    public void onSensorChanged(SensorEvent sensorEvent)
    {
        if (sensorEvent.sensor.getType() == Sensor.TYPE_ACCELEROMETER)
        {
//			Log.d(MyApplication.TAG, String.format("accel %.3f, %.3f, %.3f, %d", sensorEvent.values[0], sensorEvent.values[1], sensorEvent.values[2], sensorEvent.timestamp));
            receiveAccelerometer(sensorEvent.values[0], sensorEvent.values[1], sensorEvent.values[2], sensorEvent.timestamp);
        }
        else if (sensorEvent.sensor.getType() == Sensor.TYPE_GYROSCOPE || sensorEvent.sensor.getType() == Sensor.TYPE_GYROSCOPE_UNCALIBRATED)
        {
//			Log.d(MyApplication.TAG, String.format("gyro %.3f, %.3f, %.3f, %d", sensorEvent.values[0], sensorEvent.values[1], sensorEvent.values[2], sensorEvent.timestamp));
            receiveGyro(sensorEvent.values[0], sensorEvent.values[1], sensorEvent.values[2], sensorEvent.timestamp);
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy)
    {
        Log.d(MyApplication.TAG, String.format("onAccuracyChanged(%s, %d)", sensor.getName(), accuracy));
    }

    //ISyncedFrameReceiver interface

    @Override
    public void onSyncedFrames(long time_us, long shutter_time_us, int width, int height, int stride, final ByteBuffer colorData, int depthWidth, int depthHeight, int depthStride, final ByteBuffer depthData)
    {
        boolean result = receiveImageWithDepth(time_us, shutter_time_us, false, width, height, stride, colorData, depthWidth, depthHeight, depthStride, depthData);
//        if (!result) Log.w(MyApplication.TAG, "receiveImageWithDepth() returned FALSE");
    }

    // ITrackerReceiver interface

    @Override
    public void onStatusUpdated(int runState, int errorCode, int confidence, float progress)
    {
        Log.d(MyApplication.TAG, String.format("onStatusUpdated - runState: %d progress: %f", runState, progress));
//        if (receiver != null) receiver.onStatusUpdated(status);
    }

    @Override
    public void onDataUpdated(SensorFusionData data)
    {
        Log.d(MyApplication.TAG, String.format("onDataUpdated - %d features", data.getFeatures().size()));
        if (receiver != null) receiver.onDataUpdated(data);
    }
}
