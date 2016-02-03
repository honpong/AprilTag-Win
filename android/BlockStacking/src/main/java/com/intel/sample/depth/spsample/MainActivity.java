/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2015 Intel Corporation. All Rights Reserved.

*******************************************************************************/

package com.intel.sample.depth.spsample;
import android.app.Activity;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.os.Bundle;

// To Launch in playback mode use the command:
// 	- am start -W -n com.intel.sample.depth.spsample/.MainActivity -e play <filename>
// 	
// 	For example:
// 	adb shell 'am start -W -n com.intel.sample.depth.spsample/.MainActivity -e play myfile.rssdk'

public class MainActivity extends Activity{
   
    private SPBasicFragment mSPBasicFrag = null;
    private CameraHandler mCameraModule =  null;
    private final String SP_BASIC_FRAG_TAG = "SP_BASIC";
	
	/** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        FragmentManager fm = getFragmentManager();
    	mSPBasicFrag = (SPBasicFragment) fm.findFragmentByTag(SP_BASIC_FRAG_TAG);
    	if (mSPBasicFrag == null) {
    		FragmentTransaction ft = fm.beginTransaction();
    		//switch between displaying live mesh and displaying images of reconstruction rendering
    		ft.add(R.id.uiFragContainer, new SPBasicFragment(SPBasicFragment.DISPLAY_RECONSTRUCTION_RENDERING), 
    				SP_BASIC_FRAG_TAG);
    		ft.commit();
    		fm.executePendingTransactions();
    		mSPBasicFrag = (SPBasicFragment) fm.findFragmentByTag(SP_BASIC_FRAG_TAG);
    	}
    	
    	mSPBasicFrag.setRetainInstance(true);
    	mCameraModule = new SDKCameraManager(this, mSPBasicFrag);        		    	
    	mSPBasicFrag.setCameraModule(mCameraModule);        
    }
    
	@Override
	protected void onStop(){
		super.onStop();
	}	
}