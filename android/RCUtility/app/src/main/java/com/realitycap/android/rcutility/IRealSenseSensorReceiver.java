package com.realitycap.android.rcutility;

import android.media.Image;

/**
 * Created by benhirashima on 7/3/15.
 */
public interface IRealSenseSensorReceiver
{
    void onSyncedFrames(final Image colorImage, final Image depthImage);
//    void onAccelerometerSamples(ArrayList<SensorSample> samples);
//    void onGyroSamples(ArrayList<SensorSample> samples);
}
