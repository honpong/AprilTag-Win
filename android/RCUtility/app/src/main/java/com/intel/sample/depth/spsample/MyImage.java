package com.intel.sample.depth.spsample;

import java.nio.ByteBuffer;

import android.util.Size;

import com.intel.camera.toolkit.depth.Image;
import com.intel.camera.toolkit.depth.ImageInfo;
import com.intel.camera.toolkit.depth.RSPixelFormat;
import com.intel.camera.toolkit.depth.StreamType;

class MyImage implements Image {

	private ImageInfo mInfo;
	private ByteBuffer mBuffer;
	private StreamType mType;
	private long mTimeStamp;
	
	public MyImage(int width, int height, RSPixelFormat format){
		this(width, height, format, ByteBuffer.allocateDirect((int)(width * height * format.sizeBytes)));
	}
	
	public MyImage(Size size, RSPixelFormat format){
		this(size.getWidth(), size.getHeight(), format, ByteBuffer.allocateDirect((int)(size.getWidth() * size.getHeight() * format.sizeBytes)));
	}
	
	protected MyImage(int width, int height, RSPixelFormat format, ByteBuffer buffer){
		mInfo = new ImageInfo(width, height, format);
		mBuffer = buffer;		
	}
	
	protected void setType(StreamType type){
		mType = type;
	}
	
	@Override
	public StreamType getType() {		
		return mType;
	}
	
	@Override
	public ImageInfo getInfo() {
		return mInfo;
	}

	@Override
	public int getWidth() {
		return mInfo.Width;
	}

	@Override
	public int getHeight() {
		return mInfo.Height;
	}

	@Override
	public int getFormat() {
		return mInfo.Format.value;
	}

	
	protected void setTimeStamp(long ts){
		mTimeStamp = ts;
	}
	
	
	@Override
	public long getTimeStamp() {
		return mTimeStamp;
	}

	
	@Override
	public ByteBuffer acquireAccess() {
		return mBuffer;
	}

	@Override
	public void releaseAccess() {
		// TODO Auto-generated method stub		
	}
}
