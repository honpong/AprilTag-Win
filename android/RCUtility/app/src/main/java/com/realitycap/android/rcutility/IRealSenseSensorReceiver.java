package com.realitycap.android.rcutility;

import com.intel.camera.toolkit.depth.sensemanager.SensorSample;

import java.nio.ByteBuffer;
import java.util.ArrayList;

/**
 * Created by benhirashima on 7/3/15.
 */
public interface IRealSenseSensorReceiver
{
    void onSyncedFrames(long time_ns, long shutter_time_ns, int width, int height, int stride, final ByteBuffer colorData, int depthWidth, int depthHeight, int depthStride, final ByteBuffer depthData);
    void onAccelerometerSamples(ArrayList<SensorSample> samples);
    void onGyroSamples(ArrayList<SensorSample> samples);
}
