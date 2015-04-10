//
//  ReplayController.m
//  RCCapture
//
//  Created by Brian on 2/18/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

#import "RCReplayManager.h"
#define PREF_DEVICE_PARAMS @"DeviceCalibration"

// TODO: Unify this with RCSensorFusion.mm get_timestamp which should probably be moved to a platform specific place as part of the porting branch
#include <mach/mach_time.h>
uint64_t get_timestamp()
{
    static mach_timebase_info_data_t s_timebase_info;
    if (s_timebase_info.denom == 0) {
        mach_timebase_info(&s_timebase_info);
    }
    return mach_absolute_time() * s_timebase_info.numer / s_timebase_info.denom / 1000;
}

@interface RCReplayManager () {
    NSFileHandle * replayFile;
    RCSensorFusion * sensorFusion;
    BOOL isRunning;
    BOOL isRealtime;
    int packetsDispatched;
    float currentProgress;
    unsigned long long totalBytes;
    unsigned long long bytesDispatched;
    dispatch_queue_t queue;

    float pathLength;
    int width, height;
    int framerate;

    CFDictionaryRef pixelBufferAttributes;
    CVPixelBufferPoolRef pixelBufferPool;
}
@end


@interface RCSensorFusion (CategoryInternal)
- (void) startReplayWithRealtime:(bool)realtime withWidth:(int)width withHeight:(int)height withFramerate:(int)framerate;
@end

// No need to override the videodevice, as passing nil returns false for supports focus

@interface CMGyroDataReplay : CMGyroData

@property (nonatomic) CMRotationRate rotationRate;
@property (nonatomic) NSTimeInterval timestamp;

@end

@implementation CMGyroDataReplay

@synthesize rotationRate, timestamp;

@end

@interface CMAccelerometerDataReplay : CMAccelerometerData

@property (nonatomic) CMAcceleration acceleration;
@property (nonatomic) NSTimeInterval timestamp;

@end

@implementation CMAccelerometerDataReplay

@synthesize acceleration, timestamp;

@end


@implementation RCReplayManager


+ (id) sharedInstance
{
    static RCReplayManager *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [RCReplayManager new];
    });
    return instance;
}

- (id) init
{
    self = [super init];
    if (self) {
        queue = dispatch_queue_create("com.realitycap.replay", DISPATCH_QUEUE_SERIAL);
    }
    return self;
}

packet_t *replay_packet_alloc(enum packet_type type, uint32_t bytes, uint64_t time)
{
    //add 7 and mask to pad to 8-byte boundary
    bytes = ((bytes + 7) & 0xfffffff8u);
    //header
    bytes += 16;

    packet_t * ptr = malloc(bytes);
    //fprintf(stderr, "malloc %d bytes for type %d at time %llu\n", bytes, type, time);
    ptr->header.type = type;
    ptr->header.bytes = bytes;
    ptr->header.time = time;
    ptr->header.user = 0;
    return ptr;
}

packet_t * packet_read(FILE * file)
{
    packet_header_t header;
    fread(&header, 16, 1, file);

    packet_t * ptr = replay_packet_alloc(header.type, header.bytes, header.time);
    fread(ptr + 16, header.bytes, 1, file);

    return ptr;
}

- (void)dispatchAccelerometerPacket:(packet_accelerometer_t *)packet
{
    CMAccelerometerDataReplay * data = [[CMAccelerometerDataReplay alloc] init];
    CMAcceleration acc;
    acc.x = -packet->a[0] / 9.80665;
    acc.y = -packet->a[1] / 9.80665;
    acc.z = -packet->a[2] / 9.80665;
    data.acceleration = acc;
    NSTimeInterval timestamp = packet->header.time / 1.e6;
    data.timestamp = timestamp;
    [sensorFusion receiveAccelerometerData:data];
}

- (void)dispatchGyroscopePacket:(packet_gyroscope_t *)packet
{
    CMGyroDataReplay * data = [[CMGyroDataReplay alloc] init];
    CMRotationRate rate;
    rate.x = packet->w[0];
    rate.y = packet->w[1];
    rate.z = packet->w[2];
    data.rotationRate = rate;
    NSTimeInterval timestamp = packet->header.time / 1.e6;
    data.timestamp = timestamp;
    [sensorFusion receiveGyroData:data];
}

- (void)dispatchCameraPacket:(packet_camera_t *)packet
{
    CVImageBufferRef imageBuffer;

    if(CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault, pixelBufferPool, &imageBuffer) != kCVReturnSuccess)
    {
        NSLog(@"Error, failed to create a pixel buffer");
    }

    CVPixelBufferLockBaseAddress(imageBuffer, 0);
    void *baseAddress;
    if(CVPixelBufferIsPlanar(imageBuffer))
        baseAddress = CVPixelBufferGetBaseAddressOfPlane(imageBuffer, 0);
    else
        baseAddress = CVPixelBufferGetBaseAddress(imageBuffer);
    memcpy(baseAddress, packet->data + 16, width*height); // +16 bytes to skip P5 pgm header
    CVPixelBufferUnlockBaseAddress(imageBuffer, 0);

    CMVideoFormatDescriptionRef formatDescription;
    CMVideoFormatDescriptionCreateForImageBuffer(kCFAllocatorDefault, imageBuffer, &formatDescription);
    CMSampleTimingInfo sampleTiming;
    sampleTiming.duration = CMTimeMake(1, framerate);
    sampleTiming.decodeTimeStamp = CMTimeMake((packet->header.time)*1.e3, 1.e9); // time in 1/1e9 sec
    sampleTiming.presentationTimeStamp = sampleTiming.decodeTimeStamp;

    CMSampleBufferRef ref;
    CMSampleBufferCreateForImageBuffer(kCFAllocatorDefault,imageBuffer,TRUE,nil,nil,formatDescription,&sampleTiming,&ref);
    [sensorFusion receiveVideoFrame:ref];

    CFRelease(formatDescription);
    CVPixelBufferRelease(imageBuffer);
    CFRelease(ref);
}

- (BOOL)dispatchNextPacket:(packet_t *)packet
{
    if(!packet)
        return FALSE;

    switch(packet->header.type) {
        case packet_camera:
            [self dispatchCameraPacket:(packet_camera_t *)packet];
            break;
        case packet_gyroscope:
            [self dispatchGyroscopePacket:(packet_gyroscope_t *)packet];
            break;
        case packet_accelerometer:
            [self dispatchAccelerometerPacket:(packet_accelerometer_t *)packet];
            break;
    }
    return TRUE;
}

- (void) sensorFusionDidChangeStatus:(RCSensorFusionStatus *)status
{
    if(status.error.code != RCSensorFusionErrorCodeNone)
    {
        NSLog(@"SENSOR FUSION ERROR %li", (long)status.error.code);
    }
}

- (void) sensorFusionDidUpdateData:(RCSensorFusionData*)data
{
    pathLength = [data totalPathLength].scalar;
}

- (void)cleanup
{
    if(sensorFusion)
        [sensorFusion stopSensorFusion];

    NSLog(@"Path length %f", pathLength);

    if(replayFile)
        [replayFile closeFile];

    CFRelease(pixelBufferAttributes);
    CVPixelBufferPoolRelease(pixelBufferPool);

    [sensorFusion stopSensorFusion];
    if(_delegate && [_delegate respondsToSelector:@selector(replayFinished)])
        dispatch_async(dispatch_get_main_queue(), ^{
            [_delegate replayFinished];
        });

}

- (void)updateProgressWithBytes:(unsigned long long)bytes
{
    // Throttle progress reporting to only occury every 0.5%
    float progress = bytes*1./totalBytes;
    if(progress - currentProgress > 0.005) {
        currentProgress = progress;
        if(_delegate && [_delegate respondsToSelector:@selector(replayProgress:)])
            dispatch_async(dispatch_get_main_queue(), ^{
                [_delegate replayProgress:progress];
            });
    }
}

- (void)replayLoop
{
    uint64_t first_timestamp, realtime_offset;
    uint64_t time_started, now;

    NSNumber * nsWidth = [NSNumber numberWithInt:width];
    NSNumber * nsHeight = [NSNumber numberWithInt:height];
    pixelBufferAttributes = CFBridgingRetain(@{(id)kCVPixelBufferWidthKey: nsWidth,
                                              (id)kCVPixelBufferHeightKey: nsHeight,
                                              (id)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_OneComponent8)});
    CVPixelBufferPoolCreate (kCFAllocatorDefault, nil, pixelBufferAttributes, &pixelBufferPool);
    NSData * headerData;
    headerData = [replayFile readDataOfLength:16];
    packet_header_t * packetHeader = (packet_header_t *)headerData.bytes;
    first_timestamp = packetHeader->time;
    NSLog(@"First_timestamp %llu", packetHeader->time);
    time_started = get_timestamp();
    now = time_started;

    // filter->ignore_lateness is not set on realtime replay, so we must adjust
    // the timestamps to be relative to the current system time
    realtime_offset = 0;
    if(isRealtime)
        realtime_offset = now - first_timestamp;

    while (isRunning && headerData.length == 16) {
        @autoreleasepool {
            NSData * packetData = [replayFile readDataOfLength:(packetHeader->bytes - 16)];
            if(packetData.length != packetHeader->bytes - 16) {
                NSLog(@"ERROR: Malformed packet");
                break;
            }

            packet_t * packet = (packet_t *)malloc(packetHeader->bytes);
            memcpy(&packet->header, headerData.bytes, 16);
            memcpy(packet->data, packetData.bytes, packetHeader->bytes - 16);

            // Adjust the time of the packet to be consistent with the start time of replay
            packet->header.time += realtime_offset;

            now = get_timestamp();
            float delta_seconds = (1.*packet->header.time - now)/1e6;
            if(isRealtime && delta_seconds > 0) {
                [NSThread sleepForTimeInterval:delta_seconds];
            }


            [self dispatchNextPacket:packet];
            free(packet);

            bytesDispatched += packetHeader->bytes;
            packetsDispatched++;
            [self updateProgressWithBytes:bytesDispatched];

            headerData = [replayFile readDataOfLength:16];
            packetHeader = (packet_header_t *)headerData.bytes;
        }
    }
    NSLog(@"Dispatched %d packets %.2f Mbytes", packetsDispatched, bytesDispatched/1.e6);
}

- (void)startReplay
{
    if(replayFile) {
        isRunning = TRUE;

        // Reset device calibration
        //NSLog(@"Reset device calibration");
        //[[NSUserDefaults standardUserDefaults] removeObjectForKey:PREF_DEVICE_PARAMS];

        // Initialize sensor fusion.
        sensorFusion = [RCSensorFusion sharedInstance];
        sensorFusion.delegate = self;

        [sensorFusion startReplayWithRealtime:isRealtime withWidth:width withHeight:height withFramerate:framerate];
        [sensorFusion startSensorFusionUnstableWithDevice:nil];

        dispatch_async(queue, ^(void) {
            [self replayLoop];
            [self cleanup];
        });
    }
}

- (void)setupWithPath:(NSString *)path withRealtime:(BOOL)realtime withWidth:(int)_width withHeight:(int)_height withFramerate:(int)_framerate
{
    NSLog(@"Setup replay with %@", path);

    NSDictionary *attributes = [[NSFileManager defaultManager] attributesOfItemAtPath:path error:NULL];
    totalBytes = [attributes fileSize]; // in bytes
    replayFile = [NSFileHandle fileHandleForReadingAtPath:path];
    if(!replayFile)
        NSLog(@"ERROR: Unable to open %@", path);

    packetsDispatched = 0;
    bytesDispatched = 0;
    currentProgress = 0;
    width = _width;
    height = _height;
    framerate = _framerate;
    isRealtime = realtime;
}

- (void)stopReplay
{
    isRunning = FALSE;
}

@end
