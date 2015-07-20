package com.realitycap.android.rcutility;

/**
 * Created by benhirashima on 7/20/15.
 */
public interface ITrackerReceiver
{
    void onStatusUpdated(SensorFusionStatus status);
    void onDataUpdated(SensorFusionData data);
}
