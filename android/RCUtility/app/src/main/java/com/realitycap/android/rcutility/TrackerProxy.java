package com.realitycap.android.rcutility;

import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.util.Log;
import android.view.Surface;

import com.intel.camera.toolkit.depth.sensemanager.SensorSample;

import java.nio.ByteBuffer;
import java.util.ArrayList;

public class TrackerProxy implements SensorEventListener, IRealSenseSensorReceiver, ITrackerReceiver
{
    static
    {
        System.loadLibrary("tracker_wrapper");
    }

    private static final String TAG = TrackerProxy.class.getSimpleName();
    public static final int CAMERA_EGRAY8 = 0;
    public static final int CAMERA_EDEPTH16 = 1;

    public native boolean createTracker();
    public native boolean destroyTracker();
    public native boolean startTracker();
    public native void stopTracker();
    public native boolean startCalibration();
    public native boolean startReplay(String absFilePath);
    public native boolean setOutputLog();
    public native void configureCamera(int camera, int width_px, int height_px, float center_x_px, float center_y_px, float focal_length_x_px, float focal_length_y_px, float skew, boolean fisheye, float fisheye_fov_radians);
    public native void setOutputLog(String filename);
    public native String getCalibration();
    public native boolean setCalibration(String cal);
    public native void setGLSurface(Surface surface);

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

    //region SensorEventListener interface. sensor data from android API comes in here.
    @Override
    public void onSensorChanged(SensorEvent sensorEvent)
    {
        if (sensorEvent.sensor.getType() == Sensor.TYPE_ACCELEROMETER)
        {
//			Log.d(TAG, String.format("accel %.3f, %.3f, %.3f, %d", sensorEvent.values[0], sensorEvent.values[1], sensorEvent.values[2], sensorEvent.timestamp));
            receiveAccelerometer(sensorEvent.values[0], sensorEvent.values[1], sensorEvent.values[2], sensorEvent.timestamp);
        }
        else if (sensorEvent.sensor.getType() == Sensor.TYPE_GYROSCOPE || sensorEvent.sensor.getType() == Sensor.TYPE_GYROSCOPE_UNCALIBRATED)
        {
//			Log.d(TAG, String.format("gyro %.3f, %.3f, %.3f, %d", sensorEvent.values[0], sensorEvent.values[1], sensorEvent.values[2], sensorEvent.timestamp));
            receiveGyro(sensorEvent.values[0], sensorEvent.values[1], sensorEvent.values[2], sensorEvent.timestamp);
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy)
    {
        Log.d(TAG, String.format("onAccuracyChanged(%s, %d)", sensor.getName(), accuracy));
    }
    //endregion

    //region IRealSenseSensorReceiver interface. sensor data from RS API comes in here.
    @Override
    public void onSyncedFrames(long time_ns, long shutter_time_ns, int width, int height, int stride, final ByteBuffer colorData, int depthWidth, int depthHeight, int depthStride, final ByteBuffer depthData)
    {
        //Log.d(TAG, String.format("camera %d", time_ns));
        boolean result = receiveImageWithDepth(time_ns, shutter_time_ns, false, width, height, stride, colorData, depthWidth, depthHeight, depthStride, depthData);
//        if (!result) Log.w(TAG, "receiveImageWithDepth() returned FALSE");
    }

    @Override
    public void onAccelerometerSamples(ArrayList<SensorSample> samples)
    {
        if (samples == null) return;

        for (SensorSample sample : samples)
        {
            if (sample != null) receiveAccelerometer(sample.values()[0], sample.values()[1], sample.values()[2], sample.timestamp());
        }
    }

    @Override
    public void onGyroSamples(ArrayList<SensorSample> samples)
    {
        if (samples == null) return;

        for (SensorSample sample : samples)
        {
            if (sample != null) receiveGyro(sample.values()[0], sample.values()[1], sample.values()[2], sample.timestamp());
        }
    }
    //endregion

    //region ITrackerReceiver interface. callbacks from tracker library come in here.
    @Override
    public void onStatusUpdated(int runState, int errorCode, int confidence, float progress)
    {
//        Log.d(TAG, String.format("onStatusUpdated - runState: %d progress: %f", runState, progress));
        if (receiver != null) receiver.onStatusUpdated(runState, errorCode, confidence, progress);
    }

    @Override
    public void onDataUpdated(SensorFusionData data)
    {
//        Log.d(TAG, String.format("onDataUpdated"));
        if (receiver != null) receiver.onDataUpdated(data);
    }
    //endregion
}
