package com.realitycap.android.rcutility;

/**
 * Created by benhirashima on 7/20/15.
 */
public interface ITrackerReceiver
{
    void onStatusUpdated(final int runState, final int errorCode, final int confidence, final float progress);
    void onDataUpdated(SensorFusionData data);
}
