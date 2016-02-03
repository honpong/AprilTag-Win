package com.realitycap.android.rcutility;

import android.content.Context;
import android.util.Log;

import com.intel.camera.toolkit.depth.SensorType;
import com.intel.camera.toolkit.depth.Camera;
import com.intel.camera.toolkit.depth.Image;
import com.intel.camera.toolkit.depth.ImageSet;
import com.intel.camera.toolkit.depth.StreamSet;
import com.intel.camera.toolkit.depth.SenseManagerCallback;
import com.intel.camera.toolkit.depth.RSPixelFormat;
import com.intel.camera.toolkit.depth.RSException;
import com.intel.camera.toolkit.depth.StreamProfile;
import com.intel.camera.toolkit.depth.StreamProfileSet;
import com.intel.camera.toolkit.depth.StreamType;
import com.intel.camera.toolkit.depth.StreamTypeSet;
import com.intel.camera.toolkit.depth.IMUCaptureManager;
import com.intel.camera.toolkit.depth.sensemanager.SenseManager;

/**
 * Created by benhirashima on 7/2/15.
 */
public class RealSenseManager
{
    private static final String TAG = RealSenseManager.class.getSimpleName();
    private SenseManager mSenseManager;
    private IMUCaptureManager mIMUManager;
    private boolean mIsCamRunning = false;
    private boolean enablePlayback = false;
    private IRealSenseSensorReceiver receiver;

    private StreamTypeSet userStreamTypes = new StreamTypeSet(StreamType.COLOR, StreamType.DEPTH, StreamType.UVMAP);
    private Camera.Desc playbackCamDesc = new Camera.Desc(Camera.Type.PLAYBACK, Camera.Facing.ANY, userStreamTypes);

    protected Camera.Calibration.Intrinsics mColorParams; //intrinsics param of color camera
    ICameraIntrinsicsCallback callback;

    RealSenseManager(Context context, IRealSenseSensorReceiver receiver)
    {
        mSenseManager = new SenseManager(context);
        this.receiver = receiver;
    }

    public boolean startCameras(final ICameraIntrinsicsCallback callback)
    {
        Log.d(TAG, "startCameras");

        this.callback = callback;

        if (false == mIsCamRunning)
        {
            try
            {
                if (enablePlayback)
                    mSenseManager.enableCameraDesc(playbackCamDesc);
                else
                    mSenseManager.enableStream(getUserProfiles());
                mSenseManager.init(mSenseEventHandler, null);
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

    public boolean startImu()
    {
        if (mIMUManager == null)
            mIMUManager = mSenseManager.queryIMUCaptureManager();

        try
        {
            if (!mIMUManager.enableSensor(SensorType.ACCELEROMETER))
            {
                Log.e(TAG, "Failed to enable accelerometer");
                return false;
            }
            if (!mIMUManager.enableSensor(SensorType.GYROSCOPE))
            {
                Log.e(TAG, "Failed to enable gyro");
                return false;
            }
            return true;
        }
        catch (Exception e)
        {
            Log.e(TAG, "Error starting IMU: " + e.getMessage());
            return false;
        }
    }

    public void stopImu()
    {
        try
        {
            mIMUManager.disableSensor(SensorType.ACCELEROMETER);
            mIMUManager.disableSensor(SensorType.GYROSCOPE);
        }
        catch (Exception e)
        {
            Log.e(TAG, "Error closing IMUManager: " + e.getMessage());
        }
    }

    // this was for RS IMU, which is not being used currently
//    protected ArrayList<SensorSample> getSamplesSince(int sensorType, long timestamp)
//    {
//        if (mIMUManager == null) return null;
//
//        SensorSample[] allSamples = mIMUManager.querySensorSamples(sensorType); // The sensor samples are saved in reverse chronological order (so index 0, contains the most recent sample).
//        ArrayList<SensorSample> newSamples = new ArrayList<>();
//        if(allSamples != null)
//        {
//            for (SensorSample sample : allSamples)
//            {
//                if (sample.timestamp() > timestamp) newSamples.add(sample);
//                else break;
//            }
//        }
//        return newSamples;
//    }

    public Camera.Calibration.Intrinsics getCameraIntrinsics()
    {
        return mColorParams;
    }

    SenseManagerCallback mSenseEventHandler = new SenseManagerCallback()
    {
        long lastAmeterTimestamp = 0;
        long lastGyroTimestamp = 0;

        @Override
        public void onSetProfile(Camera.CaptureInfo info)
        {
            Camera.Calibration cal = info.getStreamCalibrationData(); //  .getSnapshotCalibrationData()?
            if (cal != null)
            {
                mColorParams = cal.colorIntrinsics;
                if (callback != null && mColorParams != null) callback.cameraIntrinsicsObtained(mColorParams);
            }
        }

        @Override
        public boolean onRequestMatchFound(StreamProfileSet streams, StreamProfileSet snapshot, StreamSet preview)
        {
            //throw new RuntimeException();
            return false;
        }

        @Override
        public void onNewSnapShot(ImageSet images, Camera.CaptureInfo info)
        {
            throw new RuntimeException();
        }

        @Override
        public void onNewSample(ImageSet images, Camera.CaptureInfo info)
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

//            Log.v(TAG, "RealSense camera sample received.");

//            receiver.onSyncedFrames(color, depth); // FIXME

            color.release();
            depth.release();

            // send IMU samples
            /*ArrayList<SensorSample> ameterSamples = getSamplesSince(SensorType.ACCELEROMETER, lastAmeterTimestamp);
            if (ameterSamples != null && ameterSamples.size() > 0 && ameterSamples.get(0) != null)
            {
                lastAmeterTimestamp = ameterSamples.get(0).timestamp();
                receiver.onAccelerometerSamples(ameterSamples);
            }

            ArrayList<SensorSample> gyroSamples = getSamplesSince(SensorType.GYROSCOPE, lastGyroTimestamp);
            if (gyroSamples != null && gyroSamples.size() > 0 && gyroSamples.get(0) != null)
            {
                lastGyroTimestamp = gyroSamples.get(0).timestamp();
                receiver.onGyroSamples(gyroSamples);
            }*/
        }

        @Override
        public void onError(RSException e)
        {
            stopCameras();
            Log.e(TAG, "Error: " + e + ". The camera is not present or failed to initialize.");
        }
    };

    private StreamProfileSet getUserProfiles()
    {
        StreamProfileSet set = new StreamProfileSet();
        StreamProfile colorProfile = new StreamProfile(640, 480, RSPixelFormat.RGBA_8888, 30, StreamType.COLOR);
        StreamProfile depthProfile = new StreamProfile(320, 240, RSPixelFormat.Z16, 30, StreamType.DEPTH);
        set.set(StreamType.COLOR, colorProfile);
        set.set(StreamType.DEPTH, depthProfile);

        return set;
    }
}
