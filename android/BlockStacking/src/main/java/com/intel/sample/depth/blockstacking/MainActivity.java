/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2015 Intel Corporation. All Rights Reserved.

*******************************************************************************/

package com.intel.sample.depth.blockstacking;
import android.app.Activity;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.os.Bundle;
import android.os.PowerManager;

import com.intel.sample.depth.spsamplecommon.CameraHandler;
import com.intel.sample.depth.spsamplecommon.SDKCameraManager;

// To Launch in playback mode use the command:
// 	- am start -W -n com.intel.sample.depth.blockstacking/.MainActivity -e play <filename>
// 	
// 	For example:
// 	adb shell 'am start -W -n com.intel.sample.depth.blockstacking/.MainActivity -e play myfile.rssdk'

public class MainActivity extends Activity{
   
    private SPBasicFragment mSPBasicFrag = null;
    private CameraHandler mCameraModule =  null;
    private final String SP_BASIC_FRAG_TAG = "SP_BASIC";
	private final String WAKE_LOCK_TAG = "SP_BASIC_WAKELOCK";
	
	/** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        FragmentManager fm = getFragmentManager();
    	mSPBasicFrag = (SPBasicFragment) fm.findFragmentByTag(SP_BASIC_FRAG_TAG);
    	if (mSPBasicFrag == null) {
    		FragmentTransaction ft = fm.beginTransaction();
    		ft.add(R.id.uiFragContainer, new SPBasicFragment(), SP_BASIC_FRAG_TAG);
    		ft.commit();
    		fm.executePendingTransactions();
    		mSPBasicFrag = (SPBasicFragment) fm.findFragmentByTag(SP_BASIC_FRAG_TAG);
    	}
    	
    	mSPBasicFrag.setRetainInstance(true);
    	mCameraModule = new SDKCameraManager(this, mSPBasicFrag);
    	mSPBasicFrag.setCameraModule(mCameraModule);

		PowerManager powerManager = (PowerManager) getSystemService(POWER_SERVICE);
		PowerManager.WakeLock wakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,
				WAKE_LOCK_TAG);
		wakeLock.acquire();
    }
    
	@Override
	protected void onStop(){
		super.onStop();
	}	
}