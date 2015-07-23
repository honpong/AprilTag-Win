package com.realitycap.android.rcutility;

/**
 * Created by benhirashima on 7/20/15.
 */
public interface ITrackerReceiver
{
    void onStatusUpdated(int runState, int errorCode, int confidence, float progress);
    void onDataUpdated(SensorFusionData data);
}
