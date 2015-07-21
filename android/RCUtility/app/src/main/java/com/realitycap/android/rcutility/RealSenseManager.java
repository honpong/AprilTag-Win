package com.realitycap.android.rcutility;

import android.content.Context;
import android.hardware.Sensor;
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
import com.intel.camera.toolkit.depth.sensemanager.IMUCaptureManager;
import com.intel.camera.toolkit.depth.sensemanager.SenseManager;
import com.intel.camera.toolkit.depth.sensemanager.SensorSample;

import java.nio.ByteBuffer;
import java.util.concurrent.CountDownLatch;

/**
 * Created by benhirashima on 7/2/15.
 */
public class RealSenseManager
{
    private SenseManager mSenseManager;
    private IMUCaptureManager mIMUManager;
    private boolean mIsCamRunning = false;
    private boolean enablePlayback = false;
    private IRealSenseSensorReceiver receiver;

    private StreamTypeSet userStreamTypes = new StreamTypeSet(StreamType.COLOR, StreamType.DEPTH, StreamType.UVMAP);
    private Camera.Desc playbackCamDesc = new Camera.Desc(Camera.Type.PLAYBACK, Camera.Facing.ANY, userStreamTypes);

    protected Camera.Calibration.Intrinsics mColorParams; //intrinsics param of color camera
    protected Camera.Calibration.Intrinsics mDepthParams; //intrinsics param of depth camera
    protected Camera.Calibration.Extrinsics mDepthToColorParams;
    private CountDownLatch startupLatch;

    RealSenseManager(Context context, IRealSenseSensorReceiver receiver)
    {
        mSenseManager = new SenseManager(context);
        mIMUManager = IMUCaptureManager.instance(context);
        this.receiver = receiver;
    }

    public boolean startCameras()
    {
        Log.d(MyApplication.TAG, "startCameras");

        if (false == mIsCamRunning)
        {
            try
            {
                startupLatch = new CountDownLatch(1);
                if (enablePlayback)
                {
                    mSenseManager.enableStreams(mSenseEventHandler, playbackCamDesc);
                }
                else
                {
                    mSenseManager.enableStreams(mSenseEventHandler, getUserProfiles(), null);
                }
                startupLatch.await();
                mIsCamRunning = true;
            }
            catch (Exception e)
            {
                Log.e(MyApplication.TAG, "Exception:" + e.getMessage());
                e.printStackTrace();
            }
        }
        return mIsCamRunning;
    }

    public void stopCameras()
    {
        Log.d(MyApplication.TAG, "stopCameras");

        if (true == mIsCamRunning)
        {
            try
            {
                mSenseManager.close();
            } catch (Exception e)
            {
                Log.e(MyApplication.TAG, "Exception:" + e.getMessage());
                e.printStackTrace();
            }

            mIsCamRunning = false;
        }
    }

    /**
     * start delivery of IMU samples
     */
    public boolean startImu()
    {
        if (mIMUManager != null) return false;
        try
        {
            if (!mIMUManager.enableSensor(Sensor.TYPE_ACCELEROMETER))
            {
                Log.e(MyApplication.TAG, "Failed to enable accelerometer");
                return false;
            }
            if (!mIMUManager.enableSensor(Sensor.TYPE_GYROSCOPE_UNCALIBRATED))
            {
                Log.e(MyApplication.TAG, "Failed to enable gyro");
                return false;
            }
            return true;
        }
        catch (Exception e)
        {
            Log.e(MyApplication.TAG, "Error starting IMU: " + e.getMessage());
            return false;
        }
    }

    /**
     * Close delivery of IMU samples
     */
    public void stopImu()
    {
        try
        {
            if (mIMUManager != null) mIMUManager.close();
        }
        catch (Exception e)
        {
            Log.e(MyApplication.TAG, "Error closing IMUManager: " + e.getMessage());
        }
    }

    public SensorSample getLatestAccelerometerSample()
    {
        return getSample(Sensor.TYPE_ACCELEROMETER);
    }

    public SensorSample getLatestGyroSample()
    {
        return getSample(Sensor.TYPE_GYROSCOPE_UNCALIBRATED);
    }

    // This comment is from RS sample code for getting the gravity vector - Ben
    /**
     * Get the most recent gravity vector from sensor.
     * Note: Conversion must be made from coordinate system of gravity sensor into
     * Scene Perception's coordinate system.
     * In particular, when camera is perfectly placed horizontally and viewing outward, the
     * normalized value of gravity should be (x, y, z) ~ (0, 1, 0).
     * Also, when the camera is rotated clock wise from such horizontal position to
     * be placed in vertically and view outward, the normalized value of gravity should
     * be (x, y, z) ~ (1, 0, 0).
     *
     * @return x, y, z coordinate values of the normalized gravity vector (magnitude of 1)
     */
    protected SensorSample getSample(int sensorType)
    {
        if (mIMUManager == null) return null;

        SensorSample[] samples = mIMUManager.querySensorSamples(sensorType);
        if (samples != null && samples.length > 0)
        {
            return samples[0];
        }
        else return null;
    }

    public Camera.Calibration.Intrinsics getCameraIntrinsics()
    {
        return mColorParams;
    }

    OnSenseManagerHandler mSenseEventHandler = new OnSenseManagerHandler()
    {
        @Override
        public void onSetProfile(Camera.CaptureInfo info)
        {
//            Log.i(MyApplication.TAG, "OnSetProfile");
            Camera.Calibration cal = info.getCalibrationData();
            if (cal != null)
            {
                mColorParams 		= cal.colorIntrinsics;
//                mDepthParams 		= cal.depthIntrinsics;
//                mDepthToColorParams = cal.depthToColorExtrinsics;
            }
            startupLatch.countDown();
        }


        @Override
        public void onNewSample(ImageSet images)
        {
            if (receiver == null) return; // no point in any of this if no one is receiving it

            Image color = images.acquireImage(StreamType.COLOR);
            Image depth = images.acquireImage(StreamType.DEPTH);

            if (color == null || depth == null)
            {
                if (color == null) Log.i(MyApplication.TAG, "color is null");
                if (depth == null) Log.i(MyApplication.TAG, "depth is null");
                return;
            }

            int colorStride = color.getInfo().DataSize / color.getHeight();
            int depthStride = depth.getInfo().DataSize / depth.getHeight();

//            Log.v(MyApplication.TAG, "RealSense camera sample received.");

            ByteBuffer colorData = color.acquireAccess();
            ByteBuffer depthData = depth.acquireAccess();

            receiver.onSyncedFrames(color.getTimeStamp(), 33333, color.getWidth(), color.getHeight(), colorStride, colorData, depth.getWidth(), depth.getHeight(), depthStride, depthData);

            // temp, for testing
            receiver.onAccelerometerSample(getLatestAccelerometerSample());
            receiver.onGyroSample(getLatestGyroSample());

            color.releaseAccess();
            depth.releaseAccess();
        }


        @Override
        public void onError(StreamProfileSet profile, int error)
        {
            stopCameras();
            Log.e(MyApplication.TAG, "Error code " + error + ". The camera is not present or failed to initialize.");
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
