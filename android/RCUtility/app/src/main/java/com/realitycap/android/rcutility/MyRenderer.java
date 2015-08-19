package com.realitycap.android.rcutility;

import android.opengl.GLSurfaceView;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Created by benhirashima on 8/18/15.
 */
public class MyRenderer implements GLSurfaceView.Renderer
{
    static
    {
        System.loadLibrary("tracker_wrapper");
    }
    private native void setup();
    private native void render(int width, int height);
    private native void teardown();

    private boolean isEnabled = false;
    int width, height;

    public boolean isEnabled()
    {
        return isEnabled;
    }

    public synchronized void setIsEnabled(boolean isEnabled)
    {
        this.isEnabled = isEnabled;
    }

    @Override public void onSurfaceCreated(GL10 gl, EGLConfig config)
    {
        setup();
    }

    @Override public void onSurfaceChanged(GL10 gl, int width_, int height_)
    {
        width = width_;
        height = height_;
        teardown();
        setup();
    }

    @Override public void onDrawFrame(GL10 gl)
    {
        if (isEnabled) render(width, height);
    }
}
