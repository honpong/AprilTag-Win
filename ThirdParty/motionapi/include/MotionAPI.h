#pragma once

#include "MotionServiceCommon.h"


namespace motion{
    
    class MotionDevice;
    class MotionListner;
    
    
    class MotionDeviceManager
    {
    public:
        virtual MotionDevice* connect(MotionListner* listner) = 0;
        virtual void disconnect(MotionDevice* device) = 0;
        virtual ~MotionDeviceManager() {};
    };
    
    class MotionDevice
    {
    public:
        //Starts the streaming of motion data
        //services selected used to determine which data sources will be utilized for this session
        //data itself will be posted by callbacks provided by the MotionListner, implemented by the client
        virtual bool StartData(MotionCapabilities services) = 0;
        
        //Starts the streaming of timestamps
        //sources selected used to determine which timestamps will be sent to client
        //data itself will be posted by callbacks provided by the MotionListner, implemented by the client
        virtual bool StartTimestamps(TimestampCapabilities sources) = 0;
        
        //stops all data
        virtual bool StopData() = 0;
        
        //stops all timestamps
        virtual bool StopTimestamps() = 0;
        
        //Fisheye data path APIs
        
        //queries a frame descriptor for a Fisheye slot-id
        virtual FisheyeFrame* GetFisheyeFrameDescriptor(int slot) =0;
        
        //returns a single fisheye buffer slot provided in the MotionListner
        //client must call this function for each fisheye buffer provided, as this is a shared buffer pool
        virtual bool ReturnFisheyeBuffer(uint32_t slot) =0;
        
        //configuration APIs
        
        //sets the depth of the IMU callback buffer
        //Such that the callback will be called whenever numOfFrames samples have been collected from either sources (gyro + accel)
        //must be called before StartData() or StartTimestamps()
        virtual bool SetIMUCallbackBufferDepth(int numOfFrames) =0;
        
        //sets the depth of the fisheye buffer pool
        //System will allocate the numOfBuffers fisheye buffers - i.e. slots
        virtual bool SetFisheyeBufferDepth(int numOfFrames) = 0;
        
        //set the anti-flicker rate (matching power line frequency)
        //valid values per MotionAntiFlickerRate enum
        //Must be called before StartData() or StartTimestamps()
        virtual bool SetAntiFlickerRate(MotionAntiFlickerRate rate) =0;
        
        //set the exposure mode -> auto/manual
        //must be called before startData() or startTimestamps()
       // virtual bool SetAutoExposureMode(MotionAutoExposureMode mode) =0;
        
	//set the AE limits 
        //exposure values are in seconds
	// gain range is between 2 to 15
        virtual bool SetExposureLimits(float ExposureSecMin, float ExposureSecMax, float GainMin, float GainMax) = 0;
        
        virtual ~MotionDevice(){};
        
    };
    
    class MotionListner
    {
    public:
        
        virtual void sensorCallback(SensorFrame* frame, uint32_t numFrames) = 0;
        
        virtual void fisheyeCallback(int slot, uint64_t timestamp) = 0;
        
        virtual void timestampCallback(uint32_t source, uint32_t frame_num, uint64_t timestamp_ns) = 0;
        
        virtual ~MotionListner(){};
    };
    
    
    extern MotionDeviceManager* getMotionDeviceManagerInstance();
    
    extern void joinBinderThreadPool();
    
    
}
