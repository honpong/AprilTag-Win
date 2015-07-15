package com.realitycap.android.rcutility;

import java.nio.ByteBuffer;

/**
 * Created by benhirashima on 7/3/15.
 */
public interface ISyncedFrameReceiver
{
    void onSyncedFrames(long time_us, long shutter_time_us, int width, int height, int stride, final ByteBuffer colorData, int depthWidth, int depthHeight, int depthStride, final ByteBuffer depthData);
}
