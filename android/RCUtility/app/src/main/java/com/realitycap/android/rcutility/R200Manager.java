package com.realitycap.android.rcutility;

import android.content.Context;
import android.graphics.ImageFormat;
import android.graphics.PixelFormat;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.Image;
import android.media.ImageReader;
import android.os.Handler;
import android.util.Log;
import android.util.Pair;
import android.util.Size;
import android.view.Surface;

import com.intel.camera2.extensions.depthcamera.DepthCameraCaptureSessionConfiguration;
import com.intel.camera2.extensions.depthcamera.DepthCameraCharacteristics;
import com.intel.camera2.extensions.depthcamera.DepthCameraImageReader;
import com.intel.camera2.extensions.depthcamera.DepthCameraStreamConfigurationMap;
import com.intel.camera2.extensions.depthcamera.DepthImage;
import com.intel.camera2.extensions.depthcamera.DepthImageFormat;
import com.intel.camera2.extensions.depthcamera.UVMAPImage;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Set;

/**
 * Created by benhirashima on 8/11/15.
 */
public class R200Manager
{
    private static final String TAG = R200Manager.class.getSimpleName();
    public static final int COLOR_WIDTH = 640;
    public static final int COLOR_HEIGHT = 480;
    public static final int DEPTH_WIDTH = 320;
    public static final int DEPTH_HEIGHT = 240;

    private CameraDevice mCamera;
    private CameraCharacteristics mCameraChar;
    private final Handler mHandler = new Handler();
    private static final int MAX_NUM_FRAMES = 5;
    private CameraCaptureSession mPreviewSession = null;
    private List<Pair<Surface, Integer>> mSurfaceList = new ArrayList<Pair<Surface, Integer>>();
    private boolean mRepeating = false;
    private DepthCameraImageReader depthReader;
    private ImageReader colorReader;
    private ImageSynchronizer mImageSyncronizer;
    private IRealSenseSensorReceiver receiver;

    /**
     * A internal state object to prevent the app open the camera before its closing process is completed.
     */
    private DepthCameraState mCurState = new DepthCameraState();

    public R200Manager(IRealSenseSensorReceiver receiver)
    {
        mCurState.set(DepthCameraState.CAMERA_CLOSED);
        this.receiver = receiver;
    }

    public boolean startCamera()
    {
        Log.d(TAG, "startCamera");

        CameraManager camManager = (CameraManager) MyApplication.getContext().getSystemService(Context.CAMERA_SERVICE);
        String cameraId = null;

        try
        {
            String[] cameraIds = camManager.getCameraIdList();
            if (cameraIds.length == 0)
                throw new Exception(TAG + ": camera ids list= 0");

            Log.w(TAG, "Number of cameras: " + cameraIds.length);

            for (int i = 0; i < cameraIds.length; i++)
            {
                Log.w(TAG, "Evaluating camera " + cameraIds[i]);

                mCameraChar = camManager.getCameraCharacteristics(cameraIds[i]);
                try
                {
                    if (DepthCameraCharacteristics.isDepthCamera(mCameraChar))
                    {
                        cameraId = cameraIds[i];
                        break;
                    }
                }
                catch (Exception e)
                {
                    Log.w(TAG, "Camera " + cameraId + ": failed on isDepthCamera");
                }
            }

            if (cameraIds.length > 0 && cameraId == null && mCameraChar != null)
                throw new Exception(TAG + "No Depth Camera Found");

            if (cameraId != null)
            {
                // Color
                Log.v(TAG, "Camera " + cameraId + " color characteristics");
                StreamConfigurationMap configMap = mCameraChar.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);

                int[] colorFormats = configMap.getOutputFormats();
                for (int format : colorFormats)
                {
                    Log.v(TAG, "Camera " + cameraId + ": Supports color format " + formatToText(format));
                    Size[] mColorSizes = configMap.getOutputSizes(format);
                    for (Size s : mColorSizes)
                        Log.v(TAG, "Camera " + cameraId + ":     color size " + s.getWidth() + ", " + s.getHeight());
                }

                // Depth
                int streamIds[] = {
                        DepthCameraStreamConfigurationMap.DEPTH_STREAM_SOURCE_ID,
                        DepthCameraStreamConfigurationMap.LEFT_STREAM_SOURCE_ID,
                        DepthCameraStreamConfigurationMap.RIGHT_STREAM_SOURCE_ID
                };
                for (int streamId : streamIds)
                {
                    Log.v(TAG, "Camera " + cameraId + " DepthCameraStreamConfigurationMap for " + streamIdToText(streamId));
                    DepthCameraStreamConfigurationMap depthConfigMap = new DepthCameraStreamConfigurationMap(mCameraChar);

                    int[] depthFormats = depthConfigMap.getOutputFormats(streamId);
                    for (int format : depthFormats)
                    {
                        Log.v(TAG, "Camera " + cameraId + ": Supports depth format " + formatToText(format));

                        Size[] sizes = depthConfigMap.getOutputSizes(streamId, format);
                        for (Size s : sizes)
                            Log.v(TAG, "Camera " + cameraId + ":     color size " + s.getWidth() + ", " + s.getHeight());
                    }
                }
            }

            mCurState.set(DepthCameraState.CAMERA_OPENING);

            camManager.openCamera(cameraId, new SimpleDeviceListener(), mHandler);

            return true;
        }
        catch (Exception e)
        {
            Log.e(TAG, "In startCamera(), Exception:" + e.getMessage());
            e.printStackTrace();
            return false;
        }
    }

    public void stopCamera()
    {
        Log.d(TAG, "stopCamera");

        try
        {
            mCurState.set(DepthCameraState.CAMERA_CLOSING);
            if (mCamera != null)
            {
                if (mRepeating)
                {
                    mPreviewSession.stopRepeating();
                    mRepeating = false;
                }

                mCamera.close();
            }
        }
        catch (Exception e)
        {
            Log.e(TAG, "In stopCamera(), Exception:" + e.getMessage());
            e.printStackTrace();
        }

        //Clean up memory allocated in the createCameraSessioin().
        if (mImageSyncronizer != null)
        {
            mImageSyncronizer.release();
            mImageSyncronizer = null;
        }

        if (mSurfaceList != null)
        {
            mSurfaceList.clear();
        }

        if (colorReader != null)
        {
            colorReader.close();
            colorReader = null;
        }

        if (depthReader != null)
        {
            depthReader.close();
            depthReader = null;
        }

        mPreviewSession = null;
    }

    public boolean isRunning()
    {
        return mCurState.get() != DepthCameraState.CAMERA_CLOSED;
    }

    public class ImageSynchronizer
    {
        private static final int COLOR = 0;
        private static final int DEPTH = 1;
        private static final int UVMAP = 2;
        private static final int MAX_TYPES = 3;

        private HashMap<Integer, ArrayList<Image>> mImageMap = new HashMap<Integer, ArrayList<Image>>();
        int[] mCounters = new int[MAX_TYPES];

        public ImageSynchronizer()
        {
        }

        public void addImageType(int type)
        {
            mImageMap.put(type, new ArrayList<Image>());
            mCounters[type] = 0;
        }

        public void notify(int type, Image data)
        {
            boolean all_available = false;

            ArrayList<Image> list = mImageMap.get(type);
            if (null != list)
            {
                list.add(data);
                mCounters[type]++;
                all_available = true;
                for (int i = 0; i < MAX_TYPES; i++)
                {
                    if (mImageMap.containsKey(i) && mCounters[i] <= 0)
                    {
                        all_available = false;
                        break;
                    }
                }
            }

            if (all_available)
            {
                Image colorImage = null;
                DepthImage depthImage = null;
                if (mImageMap.containsKey(DEPTH))
                {
                    depthImage = (DepthImage) mImageMap.get(DEPTH).remove(0);
                    mCounters[DEPTH]--;
                }
                if (mImageMap.containsKey(COLOR))
                {
                    colorImage = (Image) mImageMap.get(COLOR).remove(0);
                    mCounters[COLOR]--;
                }

                if ((null != colorImage) && (null != depthImage))
                    onImageAvailable(colorImage, depthImage);

                if (colorImage != null)
                    colorImage.close();
                if (depthImage != null)
                    depthImage.close();
            }
        }


        public void onImageAvailable(Image colorImage, DepthImage depthImage)
        {
            if (receiver != null)
            {
                Image.Plane[] planes = colorImage.getPlanes();
                assert (planes != null && planes.length > 0);

                Image.Plane[] depthPlanes = depthImage.getPlanes();
                assert (depthPlanes != null && depthPlanes.length > 0);

                receiver.onSyncedFrames(colorImage.getTimestamp(), 33333000, colorImage.getWidth(), colorImage.getHeight(), planes[0].getRowStride(), planes[0].getBuffer(), depthImage.getWidth(), depthImage.getHeight(), depthPlanes[0].getRowStride(), depthPlanes[0].getBuffer());
            }
        }

        public void release()
        {
            Set<Integer> keys = mImageMap.keySet();

            for (Integer k : keys)
            {
                ArrayList<Image> list = mImageMap.get(k);
                list.clear();
            }
            mCounters = new int[MAX_TYPES];
        }
    }

    private class DepthImageAvailableListener implements DepthCameraImageReader.OnDepthCameraImageAvailableListener
    {
        @Override
        public void onDepthCameraImageAvailable(DepthCameraImageReader reader)
        {
            Image image = reader.acquireNextImage();
            if (image != null)
            {
                Image.Plane[] planes = image.getPlanes();
                if (planes != null && planes[0] != null)
                {
                    if (image instanceof DepthImage)
                        mImageSyncronizer.notify(ImageSynchronizer.DEPTH, image);
                    else if (image instanceof UVMAPImage)
                        mImageSyncronizer.notify(ImageSynchronizer.UVMAP, image);
                }
            }
        }
    }

    private class ColorImageAvailableListener implements ImageReader.OnImageAvailableListener
    {
        @Override
        public void onImageAvailable(ImageReader reader)
        {
            Image image = reader.acquireNextImage();
            if (image != null)
            {
                Image.Plane[] planes = image.getPlanes();
                if (planes != null && planes[0] != null)
                    mImageSyncronizer.notify(ImageSynchronizer.COLOR, image);
            }
        }
    }

    public class SimpleCameraCaptureSession extends CameraCaptureSession.StateCallback
    {
        public SimpleCameraCaptureSession()
        {
        }

        @Override
        public void onConfigured(CameraCaptureSession cameraCaptureSession)
        {
            Log.d(TAG, "(cameraCaptureSession.StateCallback) onConfigured");
            mPreviewSession = cameraCaptureSession;
            createCameraPreviewRequest();
        }

        @Override
        public void onConfigureFailed(CameraCaptureSession cameraCaptureSession)
        {
            Log.d(TAG, "(cameraCaptureSession.StateCallback) onConfigureFailed");
        }
    }


    public class SimpleDeviceListener extends CameraDevice.StateCallback
    {

        public SimpleDeviceListener()
        {
        }

        @Override
        public void onDisconnected(CameraDevice camera)
        {
            Log.d(TAG, "(CameraDevice.StateCallback) onDisconnected");
            mCurState.set(DepthCameraState.CAMERA_CLOSED);
        }

        @Override
        public void onError(CameraDevice camera, int error)
        {
            Log.d(TAG, "(CameraDevice.StateCallback) onError");
            mCurState.set(DepthCameraState.CAMERA_CLOSED);
        }

        @Override
        public void onOpened(CameraDevice camera)
        {
            Log.d(TAG, "(CameraDevice.StateCallback) onOpened");
            mCurState.set(DepthCameraState.CAMERA_OPENED);
            mCamera = camera;
            createCameraSession();
        }

        @Override
        public void onClosed(CameraDevice camera)
        {
            Log.d(TAG, "(CameraDevice.StateCallback) onClosed");
            mCamera = null;
            mCurState.set(DepthCameraState.CAMERA_CLOSED);
        }
    }

    private void createCameraPreviewRequest()
    {
        Log.d(TAG, "createCameraPreviewRequest");

        try
        {
            if (mCamera == null) return;
            CaptureRequest.Builder reqBldr = mCamera.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);

            for (Pair<Surface, Integer> s : mSurfaceList)
                reqBldr.addTarget(s.first);

            mPreviewSession.setRepeatingRequest(reqBldr.build(), null, mHandler);
            mRepeating = true;
        }
        catch (CameraAccessException e)
        {
            Log.e(TAG, "In createCameraPreviewRequest(), Exception:" + e.getMessage());
            e.printStackTrace();
        }
    }


    private void createCameraSession()
    {
        Log.d(TAG, "createCameraSession");
        mImageSyncronizer = new ImageSynchronizer();

        try
        {
            // NOTE: Sample is using hard coded resolutions: A robust app should verify these exist from camera capabilities.

            // Setup Camera
            colorReader = ImageReader.newInstance(COLOR_WIDTH, COLOR_HEIGHT, ImageFormat.YUV_420_888, MAX_NUM_FRAMES);
            colorReader.setOnImageAvailableListener(new ColorImageAvailableListener(), null);
            mSurfaceList.add(new Pair<Surface, Integer>(colorReader.getSurface(), DepthCameraStreamConfigurationMap.COLOR_STREAM_SOURCE_ID));
            mImageSyncronizer.addImageType(ImageSynchronizer.COLOR);

            depthReader = DepthCameraImageReader.newInstance(DEPTH_WIDTH, DEPTH_HEIGHT, DepthImageFormat.Z16, MAX_NUM_FRAMES);
            depthReader.setOnImageAvailableListener(new DepthImageAvailableListener(), null);
            mSurfaceList.add(new Pair<Surface, Integer>(depthReader.getSurface(), DepthCameraStreamConfigurationMap.DEPTH_STREAM_SOURCE_ID));
            mImageSyncronizer.addImageType(ImageSynchronizer.DEPTH);

            DepthCameraCaptureSessionConfiguration.createDepthCaptureSession(mCamera, mCameraChar, mSurfaceList, new SimpleCameraCaptureSession(), null);
        }
        catch (Exception e)
        {
            Log.e(TAG, "In createCameraSession(), Exception:" + e.getMessage());
            e.printStackTrace();
        }
    }

    private class DepthCameraState
    {
        public final static int CAMERA_OPENING = 0x10;
        public final static int CAMERA_OPENED = 0x11;
        public final static int CAMERA_CLOSING = 0x12;
        public final static int CAMERA_CLOSED = 0x13;

        private int mState = CAMERA_CLOSED;

        public DepthCameraState()
        {
            mState = CAMERA_CLOSED;
        }

        public synchronized void set(int state)
        {
            mState = state;
            Log.d(TAG, "Camera State: " + toString());
        }

        public int get()
        {
            return mState;
        }

        @Override
        public String toString()
        {
            String retVal = "NO State";
            switch (mState)
            {
                case CAMERA_OPENING:
                    retVal = "CAMERA_OPENING";
                    break;
                case CAMERA_OPENED:
                    retVal = "CAMERA_OPENED";
                    break;
                case CAMERA_CLOSING:
                    retVal = "CAMERA_CLOSING";
                    break;
                case CAMERA_CLOSED:
                    retVal = "CAMERA_CLOSED";
                    break;
            }

            return retVal;
        }
    }

    public static String streamIdToText(int streamId)
    {
        switch (streamId)
        {
            case DepthCameraStreamConfigurationMap.DEPTH_STREAM_SOURCE_ID:
                return "DEPTH_STREAM_SOURCE_ID";

            case DepthCameraStreamConfigurationMap.LEFT_STREAM_SOURCE_ID:
                return "LEFT_STREAM_SOURCE_ID";

            case DepthCameraStreamConfigurationMap.RIGHT_STREAM_SOURCE_ID:
                return "RIGHT_STREAM_SOURCE_ID";
        }

        return "<unknown streamId>: " + Integer.toHexString(streamId);
    }


    public static String formatToText(int format)
    {
        switch (format)
        {
            case ImageFormat.YUY2:
                return "ImageFormat.YUY2 (0x00000014)";

            case ImageFormat.YUV_420_888:
                return "ImageFormat.YUV_420_888 (0x00000023)";

            case ImageFormat.JPEG:
                return "JPEG";

            case ImageFormat.NV21:
                return "NV21";

            case ImageFormat.YV12:
                return "YV12";

            case PixelFormat.A_8:
                return "A_8";

            case PixelFormat.RGB_332:
                return "RGB_332";

            case PixelFormat.LA_88:
                return "LA_88";

            case PixelFormat.RGBA_4444:
                return "RGBA_4444";

            case PixelFormat.RGBA_5551:
                return "RGBA_5551";

            case PixelFormat.RGBA_8888:
                return "RGBA_8888";

            case DepthImageFormat.Z16:
                return "DepthImageFormat.Z16";

            case DepthImageFormat.UVMAP:
                return "DepthImageFormat.UVMAP";
        }

        return "<unknown format>: " + Integer.toHexString(format);
    }
}
