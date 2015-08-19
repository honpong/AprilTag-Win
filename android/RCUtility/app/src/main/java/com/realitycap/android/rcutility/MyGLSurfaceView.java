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
    private int pointerId = 0;
    private boolean isMoving = false;
    private Point2DF touchDownPoint = null;

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
        if (event.getAction() == MotionEvent.ACTION_DOWN)
        {
            pointerId = event.getPointerId(0);
            touchDownPoint = new Point2DF(event.getX(0), event.getY(0));
        }

        int pointerIndex = event.findPointerIndex(pointerId);

        if (event.getAction() == MotionEvent.ACTION_MOVE)
        {
            isMoving = true;
            Log.i(TAG, "drag " + event.getX(pointerIndex) + ", " + event.getY(pointerIndex));
            return true;
        }

        if (event.getAction() == MotionEvent.ACTION_UP)
        {
            if (isMoving && touchDownPoint != null)
            {
                Point2DF touchUpPoint = new Point2DF(event.getX(pointerIndex), event.getY(pointerIndex));
                double dist = distBetweenPoints(touchDownPoint, touchUpPoint);
                isMoving = false;
                touchDownPoint = null;
                if (dist > TAP_MOVEMENT_THRESHOLD) return true; // if moved more than a certain amount, consume this event and prevent tap detection.
            }
        }

        return super.onTouchEvent(event);
    }

    public double distBetweenPoints(Point2DF pointA, Point2DF pointB)
    {
        return Math.sqrt(Math.pow(pointB.x - pointA.x, 2) + Math.pow(pointB.y - pointA.y, 2));
    }
}
