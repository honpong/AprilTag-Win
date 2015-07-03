package com.realitycap.android.rcutility;

import android.graphics.Bitmap;
import android.util.Log;
import android.content.Context;

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
    private Bitmap colorBitmap;
    private Bitmap depthBitmap;
    private boolean mIsCamRunning = false;
    private boolean enablePlayback = false;

    private StreamTypeSet userStreamTypes = new StreamTypeSet(StreamType.COLOR, StreamType.DEPTH, StreamType.UVMAP);
    private Camera.Desc playbackCamDesc = new Camera.Desc(Camera.Type.PLAYBACK, Camera.Facing.ANY, userStreamTypes);

    RealSenseManager(Context context)
    {
        mSenseManager = new SenseManager(context);
    }

    public void startCameras()
    {
        Log.d(TAG, "startCameras");

        if (false == mIsCamRunning)
        {
            try
            {
                if (enablePlayback)
                {
                    mSenseManager.enableStreams(mSenseEventHandler, playbackCamDesc);
                } else
                {
                    mSenseManager.enableStreams(mSenseEventHandler, getUserProfiles(), null);
                }
            } catch (Exception e)
            {
                Log.e(TAG, "Exception:" + e.getMessage());
                e.printStackTrace();
            }

            mIsCamRunning = true;
        }
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
        private MyImage mTmpBuffer;

        @Override
        public void onSetProfile(Camera.CaptureInfo profiles)
        {
            Log.i(TAG, "OnSetProfile");
            // Configure Color Plane
            StreamProfile cs = profiles.getStreamProfiles().get(StreamType.COLOR);
            if (null == cs)
            {
                Log.e(TAG, "Error: NULL INDEX_COLOR");
                colorBitmap = null;
            } else
            {
                Log.i(TAG, "Configuring color with format " + cs.Format + " for width " + cs.Width + " and height " + cs.Height);
                colorBitmap = Bitmap.createBitmap(cs.Width, cs.Height, Bitmap.Config.ARGB_8888);
            }

            // Configure Depth Plane
            StreamProfile ds = profiles.getStreamProfiles().get(StreamType.DEPTH);
            if (null == ds)
            {
                Log.e(TAG, "Error: NULL INDEX_DEPTH");
                depthBitmap = null;
            } else
            {
                Log.i(TAG, "Configuring DisplayMode (DEPTH_RAW_GRAYSCALE): format " + ds.Format + " for width " + ds.Width + " and height " + ds.Height);
                depthBitmap = Bitmap.createBitmap(ds.Width, ds.Height, Bitmap.Config.ARGB_8888);
                mTmpBuffer = new MyImage(ds.Width, ds.Height, RSPixelFormat.RGBA_8888);
            }
            Log.i(TAG, "Camera Calibration: \n" + profiles.getCalibrationData());
//            mDisplayRunnable = new SimpleRunnable(mColorView, mDepthView);
        }


        @Override
        public void onNewSample(ImageSet images)
        {
            Image color = images.acquireImage(StreamType.COLOR);
            Image depth = images.acquireImage(StreamType.DEPTH);

            if (color == null) Log.i(TAG, "color is null");
            if (depth == null) Log.i(TAG, "depth is null");

            if (null != colorBitmap && null != color)
            {
                ByteBuffer data = color.acquireAccess();
                colorBitmap.copyPixelsFromBuffer(data);
                data.rewind();
                color.releaseAccess();

                Log.v(TAG, "RealSense color sample received.");
            }

            if (null != depthBitmap && null != depth && null != color)
            {
//                DepthUtils.Z16ToGrayscale8888(depth, mTmpBuffer);
//                depthBitmap.copyPixelsFromBuffer(mTmpBuffer.buffer);
//                mTmpBuffer.buffer.rewind();

                Log.v(TAG, "RealSense depth sample received.");
            }

//            mDisplayRunnable.setBitmaps(colorBitmap, depthBitmap);
//            runOnUiThread(mDisplayRunnable);
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
