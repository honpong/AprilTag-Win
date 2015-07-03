package com.realitycap.android.rcutility;

import java.nio.ByteBuffer;

/**
 * Created by benhirashima on 7/3/15.
 */
public interface ISyncedFrameReceiver
{
    void onSyncedFrames(final ByteBuffer colorData, final ByteBuffer depthData);
}
