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
    private native void render();

    private boolean isEnabled = false;

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

    }

    @Override public void onSurfaceChanged(GL10 gl, int width, int height)
    {

    }

    @Override public void onDrawFrame(GL10 gl)
    {
        if (isEnabled) render();
    }
}
