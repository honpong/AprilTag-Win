package com.realitycap.android.rcutility;

import android.media.Image;

import com.intel.camera.toolkit.depth.sensemanager.SensorSample;

import java.nio.ByteBuffer;
import java.util.ArrayList;

/**
 * Created by benhirashima on 7/3/15.
 */
public interface IRealSenseSensorReceiver
{
    void onSyncedFrames(final Image colorImage, final Image depthImage);
    void onAccelerometerSamples(ArrayList<SensorSample> samples);
    void onGyroSamples(ArrayList<SensorSample> samples);
}
