#pragma once

#include <sys/types.h>

namespace motion{
#define ALOGVV(...) ((void)0)
    
#pragma pack(1)
    
    //typedef struct
    //{
    //	uint32_t motionSources;
    //	uint32_t timestampSources;
    //
    //} MotionCapbilities;
    
    typedef uint32_t MotionCapabilities;
#define MOTION_COMPONENT_NONE  0x0000
#define MOTION_COMPONENT_GYRO  0x0001
#define MOTION_COMPONENT_ACCEL  0x0002
#define MOTION_COMPONENT_FISHEYE  0x0004
#define MOTION_COMPONENT_FEATURE_TRACKER  0x0008
#define MOTION_COMPONENT_DEPTH  0x0020
#define MOTION_COMPONENT_RGB  0x0040
    
    
    
    typedef uint32_t TimestampCapabilities;
#define TIMESTAMP_SOURCE1  0x0001
#define TIMESTAMP_SOURCE2  0x0002
#define TIMESTAMP_SOURCE3  0x0004
#define TIMESTAMP_SOURCE4  0x0008
#define TIMESTAMP_SOURCE5  0x0010
#define TIMESTAMP_SOURCE6  0x0020
#define TIMESTAMP_SOURCE7  0x0040
#define TIMESTAMP_SOURCE8  0x0080
    
    
    enum {
        MOTION_SENSOR_GYRO = 1,
        MOTION_SENSOR_ACCEL,
        MOTION_SENSOR_MAGNOMETER,
    };
    
    typedef uint32_t MotionSensorType;
    
    typedef enum
    {
        MOTION_EXPOSURE_AUTO=0,
        MOTION_EXPOSURE_MANUAL
        
    } MotionExposureMode;
    
    typedef struct
    {
        uint64_t timestamp;
        MotionSensorType type;
        float x;
        float y;
        float z;
    } SensorFrame;
    
    typedef struct
    {
        uint32_t slot;
        uint32_t frameWidth;
        uint32_t frameHeight;
        uint32_t frameStrideInBytes;
        uint32_t frameNum;
        uint64_t timestamp;
        uint32_t frameExposureNs;
        uint8_t* frameData;
        
    } FisheyeFrame;
    
    typedef enum
    {
        MOTION_AUTO_EXPOSURE_ON,
        MOTION_AUTO_EXPOSURE_OFF,
    }MotionAutoExposureMode;
    
    typedef enum
    {
        MOTION_EXPOSURE_ANTI_FLICKER_50HZ,
        MOTION_EXPOSURE_ANTI_FLICKER_60HZ,
        MOTION_EXPOSURE_ANTI_FLICKER_NOT_SET,
    } MotionAntiFlickerRate;
    
    typedef enum
    {
        
        MOTION_FRAME_RATE_05FPS=0,
        MOTION_FRAME_RATE_15FPS,
        MOTION_FRAME_RATE_20FPS,
        MOTION_FRAME_RATE_25FPS,
        MOTION_FRAME_RATE_30FPS,
        
    } MotionFrameRate;
    
#pragma pack(0)
    
}
