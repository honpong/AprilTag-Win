package com.realitycap.android.rcutility;

import android.content.Context;
import android.util.Log;

import com.intel.camera.toolkit.depth.Camera;
import com.intel.camera.toolkit.depth.Image;
import com.intel.camera.toolkit.depth.ImageSet;
import com.intel.camera.toolkit.depth.OnSenseManagerHandler;
import com.intel.camera.toolkit.depth.RSPixelFormat;
import com.intel.camera.toolkit.depth.StreamProfile;
import com.intel.camera.toolkit.depth.StreamProfileSet;
import com.intel.camera.toolkit.depth.StreamType;
import com.intel.camera.toolkit.depth.StreamTypeSet;
import com.intel.camera.toolkit.depth.sensemanager.SenseManager;

import java.nio.ByteBuffer;

/**
 * Created by benhirashima on 7/2/15.
 */
public class RealSenseManager
{
    private static final String TAG = "RealSenseManager";
    private static SenseManager mSenseManager;
    private boolean mIsCamRunning = false;
    private boolean enablePlayback = false;
    private ISyncedFrameReceiver receiver;

    private StreamTypeSet userStreamTypes = new StreamTypeSet(StreamType.COLOR, StreamType.DEPTH, StreamType.UVMAP);
    private Camera.Desc playbackCamDesc = new Camera.Desc(Camera.Type.PLAYBACK, Camera.Facing.ANY, userStreamTypes);

    RealSenseManager(Context context, ISyncedFrameReceiver receiver)
    {
        mSenseManager = new SenseManager(context);
        this.receiver = receiver;
    }

    public boolean startCameras()
    {
        Log.d(TAG, "startCameras");

        if (false == mIsCamRunning)
        {
            try
            {
                if (enablePlayback)
                {
                    mSenseManager.enableStreams(mSenseEventHandler, playbackCamDesc);
                }
                else
                {
                    mSenseManager.enableStreams(mSenseEventHandler, getUserProfiles(), null);
                }

                mIsCamRunning = true;
            }
            catch (Exception e)
            {
                Log.e(TAG, "Exception:" + e.getMessage());
                e.printStackTrace();
            }
        }

        return mIsCamRunning;
    }

    public void stopCameras()
    {
        Log.d(TAG, "stopCameras");

        if (true == mIsCamRunning)
        {
            try
            {
                mSenseManager.close();
            } catch (Exception e)
            {
                Log.e(TAG, "Exception:" + e.getMessage());
                e.printStackTrace();
            }

            mIsCamRunning = false;
        }
    }

    OnSenseManagerHandler mSenseEventHandler = new OnSenseManagerHandler()
    {
        @Override
        public void onSetProfile(Camera.CaptureInfo profiles)
        {
//            Log.i(TAG, "OnSetProfile");
        }


        @Override
        public void onNewSample(ImageSet images)
        {
            if (receiver == null) return; // no point in any of this if no one is receiving it

            Image color = images.acquireImage(StreamType.COLOR);
            Image depth = images.acquireImage(StreamType.DEPTH);

            if (color == null || depth == null)
            {
                if (color == null) Log.i(TAG, "color is null");
                if (depth == null) Log.i(TAG, "depth is null");
                return;
            }

            int colorStride = color.getInfo().DataSize / color.getHeight();
            int depthStride = depth.getInfo().DataSize / depth.getHeight();

//            Log.v(TAG, "RealSense camera sample received.");

            ByteBuffer colorData = color.acquireAccess();
            ByteBuffer depthData = depth.acquireAccess();

            receiver.onSyncedFrames(color.getTimeStamp(), 33333, color.getWidth(), color.getHeight(), colorStride, colorData, depth.getWidth(), depth.getHeight(), depthStride, depthData);

            color.releaseAccess();
            depth.releaseAccess();
        }


        @Override
        public void onError(StreamProfileSet profile, int error)
        {
            stopCameras();
            Log.e(TAG, "Error code " + error + ". The camera is not present or failed to initialize.");
        }
    };

    private StreamProfileSet getUserProfiles()
    {
        StreamProfileSet set = new StreamProfileSet();
        StreamProfile colorProfile = new StreamProfile(640, 480, RSPixelFormat.RGBA_8888, 30, StreamType.COLOR);
        StreamProfile depthProfile = new StreamProfile(320, 240, RSPixelFormat.Z16, 30, StreamType.DEPTH);
//        StreamProfile depthProfile = new StreamProfile(480, 360, RSPixelFormat.Z16, 30, StreamType.DEPTH);
//        StreamProfile uvProfile = new StreamProfile(480, 360, RSPixelFormat.UVMAP, 30, StreamType.UVMAP);
        set.set(StreamType.COLOR, colorProfile);
        set.set(StreamType.DEPTH, depthProfile);
//        set.set(StreamType.UVMAP, uvProfile);

        return set;
    }
}
