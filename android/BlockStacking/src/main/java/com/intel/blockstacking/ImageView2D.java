/********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
terms of a license agreement or nondisclosure agreement with Intel Corporation
and may not be copied or disclosed except in accordance with the terms of that
agreement.
Copyright(c) 2011-2015 Intel Corporation. All Rights Reserved.

*********************************************************************************/

package com.intel.blockstacking;

import java.nio.ByteBuffer;
import android.content.Context;
import android.graphics.Bitmap;
import android.os.Handler;
import android.util.AttributeSet;
import android.util.Size;
import android.widget.ImageView;

public class ImageView2D extends ImageView {
	private volatile boolean mIsLastDisplayComplete = true;
	public ImageView2D(Context context) {
		super(context);
	}
	
	public ImageView2D(Context context, AttributeSet attrs) {
		super(context, attrs);
	}
	
	public ImageView2D(Context context, AttributeSet attrs, int defStyleAttr) {
		super(context, attrs, defStyleAttr);
	}
	
	public ImageView2D(Context context, AttributeSet attrs, int defStyleAttr,
			int defStyleRes) {
		super(context, attrs, defStyleAttr, defStyleRes);
	}

	private Bitmap mInputBitmap = null;
	private Size mInputSize;
	private Handler mUiHandler = null;
	
	public void setUiHandler(Handler uiHandler){
		mUiHandler = uiHandler;
	}
	
	public void setInputSize(Size sizeInPixel) {
		mInputSize = sizeInPixel;
	}

	/**
	 * onImageInputUpdate is called when there is new data in the display 
	 * content. 
	 */	
	public void onImageInputUpdate(ByteBuffer newContent, Size inputSize) {
		if (mInputSize == null || newContent == null) {
			return;
		}
		
		if(!mInputSize.equals(inputSize))
		{
			mInputBitmap = null;
			mInputSize = inputSize;
		}
			
		if (mInputBitmap == null) {
			mInputBitmap = Bitmap.createBitmap(mInputSize.getWidth(),
					mInputSize.getHeight(), Bitmap.Config.ARGB_8888);
		}
		
		if (newContent != null && mUiHandler != null){		
			if (mIsLastDisplayComplete) {
				mIsLastDisplayComplete = false;
				newContent.rewind();
				mInputBitmap.copyPixelsFromBuffer(newContent);	
				mUiHandler.post(new Runnable(){
					@Override
					public void run(){
						setImageBitmap(mInputBitmap);
						mIsLastDisplayComplete = true;
					}				
				});		
			}
		}
	}
	
	public void setOnClickListener(OnClickListener clickListener) {
		super.setOnClickListener(clickListener);		
	}
}
