package com.realitycap.android.rcutility;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;

import com.intel.camera.toolkit.depth.Point2DF;

/**
 * Created by benhirashima on 8/18/15.
 */
public class MyGLSurfaceView extends GLSurfaceView
{
    private static final String TAG = MyGLSurfaceView.class.getSimpleName();
    public static final double TAP_MOVEMENT_THRESHOLD = 50.0;
    private MyRenderer mMyRenderer;
    private int pointerId1 = -1;
    private int pointerId2 = -1;
    private Point2DF touchDownPoint = null;
    private double maxDragDist = 0;

    enum TouchState
    {
        None, Dragging, Pinching
    }
    private TouchState touchState = TouchState.None;

    static
    {
        System.loadLibrary("tracker_wrapper");
    }
    private native void handleDrag(float x, float y);
    private native void handleDragEnd();
    private native void handlePinch(double pixelDist);

    public MyGLSurfaceView(Context context, AttributeSet attrs)
    {
        super(context, attrs);
        setEGLContextClientVersion(2);
        mMyRenderer = new MyRenderer();
        setRenderer(mMyRenderer);
    }

    public void startRendering()
    {
        mMyRenderer.setIsEnabled(true);
    }

    public void stopRendering()
    {
        mMyRenderer.setIsEnabled(false);
    }

    @Override public boolean onTouchEvent(MotionEvent event)
    {
        if (event.getPointerCount() > 1)
            return handlePinchEvent(event);
        else if (event.getPointerCount() == 1)
            return handleDragEvent(event);
        else
            return super.onTouchEvent(event);
    }

    private boolean handleDragEvent(MotionEvent event)
    {
//        Log.v(TAG, "handleDragEvent(" + event.getAction() + ")");

        if (event.getAction() == MotionEvent.ACTION_DOWN)
        {
            pointerId1 = event.getPointerId(0);
            touchDownPoint = new Point2DF(event.getX(0), event.getY(0));
        }

        if (pointerId1 < 0) return false;
        int pointerIndex = event.findPointerIndex(pointerId1);
        if (pointerIndex >= event.getPointerCount() || pointerIndex < 0) return false;

        if (event.getAction() == MotionEvent.ACTION_MOVE)
        {
//            Log.v(TAG, "drag " + event.getX(pointerIndex) + ", " + event.getY(pointerIndex));
            touchState = TouchState.Dragging;

            if (touchDownPoint != null)
            {
                Point2DF currentTouchPoint = new Point2DF(event.getX(pointerIndex), event.getY(pointerIndex));
                double dist = distBetweenPoints(touchDownPoint, currentTouchPoint);
                if (dist > maxDragDist) maxDragDist = dist;
            }

            handleDrag(event.getX(pointerIndex), event.getY(pointerIndex));
            return true;
        }

        if (event.getAction() == MotionEvent.ACTION_CANCEL || event.getAction() == MotionEvent.ACTION_UP)
        {
            if (touchState == TouchState.Dragging)
            {
                touchDownPoint = null;
                touchState = TouchState.None;
                if (maxDragDist > TAP_MOVEMENT_THRESHOLD)
                {
                    maxDragDist = 0;
                    handleDragEnd();
                    return true; // if moved more than a certain amount, consume this event and prevent tap detection.
                }
            }
        }

        return super.onTouchEvent(event);
    }

    private boolean handlePinchEvent(MotionEvent event)
    {
//        Log.v(TAG, "handlePinchEvent(" + event.getAction() + ")");

        if (event.getAction() == MotionEvent.ACTION_POINTER_2_DOWN) // we have to use this deprecated event because intel's build of android doesn't send ACTION_POINTER_DOWN
        {
            pointerId2 = event.getPointerId(event.getActionIndex());
            if (touchState == TouchState.Dragging) handleDragEnd();
            return super.onTouchEvent(event);
        }

        if (event.getAction() == MotionEvent.ACTION_MOVE)
        {
            touchState = TouchState.Pinching;

            if (pointerId1 < 0 || pointerId2 < 0) return super.onTouchEvent(event);
            int pointerIndex1 = event.findPointerIndex(pointerId1);
            int pointerIndex2 = event.findPointerIndex(pointerId2);

            if (pointerIndex1 >= event.getPointerCount() || pointerIndex2 >= event.getPointerCount()) return false;

            Point2DF touchPoint1 = new Point2DF(event.getX(pointerIndex1), event.getY(pointerIndex1));
            Point2DF touchPoint2 = new Point2DF(event.getX(pointerIndex2), event.getY(pointerIndex2));
            double dist = distBetweenPoints(touchPoint1, touchPoint2);

//            Log.v(TAG, "Pinch: " + dist);
            handlePinch(dist);

            return true;
        }

        if (event.getAction() == MotionEvent.ACTION_CANCEL || event.getAction() == MotionEvent.ACTION_POINTER_UP || event.getAction() == MotionEvent.ACTION_POINTER_2_UP)
        {
            touchState = TouchState.None;
            pointerId1 = -1;
            pointerId2 = -1;
            return true;
        }

        return super.onTouchEvent(event);
    }


    public double distBetweenPoints(Point2DF pointA, Point2DF pointB)
    {
        return Math.sqrt(Math.pow(pointB.x - pointA.x, 2) + Math.pow(pointB.y - pointA.y, 2));
    }
}
