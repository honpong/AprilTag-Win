package com.realitycap.android.rcutility;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;

/**
 * Created by benhirashima on 8/18/15.
 */
public class MyGLSurfaceView extends GLSurfaceView
{
    private MyRenderer mMyRenderer;

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

//    public boolean onKeyDown(int keyCode, KeyEvent event) {
//        if (keyCode == KeyEvent.KEYCODE_DPAD_CENTER) {
//            queueEvent(new Runnable() {
//                // This method will be called on the rendering
//                // thread:
//                public void run() {
//                    mMyRenderer.handleDpadCenter();
//                }});
//            return true;
//        }
//        return super.onKeyDown(keyCode, event);
//    }
}
