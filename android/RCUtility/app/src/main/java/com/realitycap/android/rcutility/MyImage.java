package com.realitycap.android.rcutility;

import com.intel.camera.toolkit.depth.Image;
import com.intel.camera.toolkit.depth.ImageInfo;
import com.intel.camera.toolkit.depth.RSPixelFormat;
import com.intel.camera.toolkit.depth.StreamType;

import java.nio.ByteBuffer;

/**
 * Created by benhirashima on 7/3/15.
 */
public class MyImage implements Image
{

    ByteBuffer buffer;
    ImageInfo info;

    public MyImage(int w, int h, RSPixelFormat f){
        buffer = ByteBuffer.allocateDirect((int) (w * h * f.sizeBytes));
        info = new ImageInfo(w, h, f);
    }

    public MyImage(ImageInfo f){
        this(f.Width, f.Height, f.Format);
    }

    @Override
    public ImageInfo getInfo() {
        return info;
    }

    @Override
    public StreamType getType() {
        return StreamType.ANY;
    }

    @Override
    public int getWidth() {
        return info.Width;
    }

    @Override
    public int getHeight() {
        return info.Height;
    }

    @Override
    public int getFormat() {
        return info.Format.value;
    }

    @Override
    public long getTimeStamp() {
        return 0;
    }

    @Override
    public ByteBuffer acquireAccess() {
        return buffer;
    }

    @Override
    public void releaseAccess() {
    }
}