//
//  CaptureController.m
//  RCCapture
//
//  Created by Brian on 10/3/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import <CoreMotion/CoreMotion.h>

#import "RCCaptureManager.h"

#define POLL

@interface RCCaptureManager ()
{
    AVCaptureSession * session;
    AVCaptureVideoDataOutput * output;
    AVCaptureDevice * device;
    AVCaptureVideoOrientation videoOrientation;

    CMMotionManager * cmMotionManager;
    NSTimer* timerMotion;
    NSTimeInterval lastGyro, lastAccelerometer;
    bool isCapturing;
    bool isFocusing;
    bool hasFocused;

    dispatch_io_t outputChannel;
    dispatch_queue_t outputQueue;
}
@end

@implementation RCCaptureManager

@synthesize delegate;

- (id)init
{
	if(self = [super init])
	{
        cmMotionManager = [CMMotionManager new];
        isCapturing = false;
        timerMotion = nil;
	}
	return self;
}

- (void) startVideoCapture
{
    isCapturing = true;
    hasFocused = true;
    if([delegate respondsToSelector:@selector(captureDidStart)])
        [delegate captureDidStart];
}

- (void) observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if ([keyPath isEqualToString:@"adjustingFocus"]) {
        bool wasFocusing = isFocusing;
        isFocusing = [change[NSKeyValueChangeNewKey] isEqualToNumber:@1];
        if(wasFocusing && !isFocusing & !hasFocused) {
            // Capture doesn't start until focus has locked and finished focusing
            if ([device lockForConfiguration:nil]) {
                if([device isFocusModeSupported:AVCaptureFocusModeLocked]) {
                    [device setFocusMode:AVCaptureFocusModeLocked];
                }
                [device unlockForConfiguration];
            }
            [self startVideoCapture];
        }
    }
}

- (void) startVideoCapture:(AVCaptureSession *)avSession withDevice:(AVCaptureDevice *)avDevice
{
    AVCaptureVideoDataOutput* avOutput = [[AVCaptureVideoDataOutput alloc] init];
    [output setAlwaysDiscardsLateVideoFrames:YES];
    [output setVideoSettings:@{(id)kCVPixelBufferPixelFormatTypeKey: @('420f')}];

    session = avSession;
    output = avOutput;
    device = avDevice;

    [device addObserver:self forKeyPath:@"adjustingFocus" options:NSKeyValueObservingOptionNew context:nil];
    if(![device isFocusModeSupported:AVCaptureFocusModeAutoFocus]) {
        [self startVideoCapture];
    }
    if([device lockForConfiguration:nil]) {
        if([device isFocusModeSupported:AVCaptureFocusModeAutoFocus]) {
            [device setFocusMode:AVCaptureFocusModeAutoFocus];
        }
        [device unlockForConfiguration];
    }

    //causes lag
    [session addOutput:output];

    AVCaptureConnection *videoConnection = [output connectionWithMediaType:AVMediaTypeVideo];
    videoOrientation = [videoConnection videoOrientation];

    dispatch_queue_t queue = dispatch_queue_create("MyQueue", DISPATCH_QUEUE_SERIAL); //docs "You use the queue to modify the priority given to delivering and processing the video frames."
    [output setSampleBufferDelegate:self queue:queue];
}

- (void) stopVideoCapture
{
    [device removeObserver:self forKeyPath:@"adjustingFocus"];
    if([device lockForConfiguration:nil]) {
        if([device isFocusModeSupported:AVCaptureFocusModeContinuousAutoFocus]) {
            [device setFocusMode:AVCaptureFocusModeContinuousAutoFocus];
        }
        [device unlockForConfiguration];
    }
    [session removeOutput:output];
}

packet_t *packet_alloc(enum packet_type type, uint32_t bytes, uint64_t time)
{
    //add 7 and mask to pad to 8-byte boundary
    bytes = ((bytes + 7) & 0xfffffff8u);
    //header
    bytes += 16;

    packet_t * ptr = malloc(bytes);
    ptr->header.type = type;
    ptr->header.bytes = bytes;
    ptr->header.time = time;
    ptr->header.user = 0;
    return ptr;
}

- (void) writeFrame:(CMSampleBufferRef)sampleBuffer
{
    if(!CMSampleBufferDataIsReady(sampleBuffer) )
    {
        NSLog( @"sample buffer is not ready. Skipping sample" );
        return;
    }
    CFRetain(sampleBuffer);
    CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)CMSampleBufferGetImageBuffer(sampleBuffer);
    CVPixelBufferRetain(pixelBuffer);

    CMTime timestamp = (CMTime)CMSampleBufferGetPresentationTimeStamp(sampleBuffer);

    //capture image meta data
    //        CFDictionaryRef metadataDict = CMGetAttachment(sampleBuffer, kCGImagePropertyExifDictionary , NULL);
    //        DLog(@"metadata: %@", metadataDict);

    uint32_t width = (uint32_t)CVPixelBufferGetWidth(pixelBuffer);
    uint32_t height = (uint32_t)CVPixelBufferGetHeight(pixelBuffer);
    uint64_t time_us = timestamp.value / (timestamp.timescale / 1000000.);

    CVPixelBufferLockBaseAddress(pixelBuffer, 0);
    unsigned char *pixel = (unsigned char *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer,0);

    packet_t *buf = packet_alloc(packet_camera, width*height+16, time_us); // 16 bytes for pgm header

    sprintf((char *)buf->data, "P5 %4d %3d %d\n", width, height, 255);
    unsigned char *outbase = buf->data + 16;
    memcpy(outbase, pixel, width*height);
    CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
    CVPixelBufferRelease(pixelBuffer);
    CFRelease(sampleBuffer);

    // 16 bytes for pgm header, 16 bytes for header
    dispatch_data_t data = dispatch_data_create(buf, buf->header.bytes, 0, DISPATCH_DATA_DESTRUCTOR_FREE);
    dispatch_io_write(outputChannel, 0, data, outputQueue, ^(bool done, dispatch_data_t data, int error) {
        if(done && error)
            NSLog(@"Error %d on write", error);
    });

}

//called on each video frame
-(void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{
	if (isCapturing) // && !isFocusing would not log video during autofocus events
    {
        [self writeFrame:sampleBuffer];
    }
}

- (void)timerCallback:(id)userInfo
{
    if(!isCapturing)
        return;

    CMGyroData *gyroData = cmMotionManager.gyroData;
    if(gyroData && gyroData.timestamp != lastGyro)
    {
        uint64_t time = gyroData.timestamp * 1000000;
        packet_t *p = packet_alloc(packet_gyroscope, 3*4, time);
        ((float*)p->data)[0] = gyroData.rotationRate.x;
        ((float*)p->data)[1] = gyroData.rotationRate.y;
        ((float*)p->data)[2] = gyroData.rotationRate.z;
        dispatch_data_t data = dispatch_data_create(p, p->header.bytes, 0, DISPATCH_DATA_DESTRUCTOR_FREE);
        dispatch_io_write(outputChannel, 0, data, outputQueue, ^(bool done, dispatch_data_t data, int error) {
            if(done && error)
                NSLog(@"Error %d on write", error);
        });

        lastGyro = gyroData.timestamp;
    }
    CMAccelerometerData *accelerometerData = cmMotionManager.accelerometerData;
    if(accelerometerData && accelerometerData.timestamp != lastAccelerometer)
    {
        // Copies the data and frees it once written, can use DISPATCH_DATA_DESTRUCTOR_FREE to pass in a pointer for the destructor to free
        uint64_t time = accelerometerData.timestamp * 1000000;
        packet_t *p = packet_alloc(packet_accelerometer, 3*4, time);
        ((float*)p->data)[0] = -accelerometerData.acceleration.x * 9.80665;
        ((float*)p->data)[1] = -accelerometerData.acceleration.y * 9.80665;
        ((float*)p->data)[2] = -accelerometerData.acceleration.z * 9.80665;
        dispatch_data_t data = dispatch_data_create(p, p->header.bytes, 0, DISPATCH_DATA_DESTRUCTOR_FREE);
        dispatch_io_write(outputChannel, 0, data, outputQueue, ^(bool done, dispatch_data_t data, int error) {
            if(done && error)
                NSLog(@"Error %d on write", error);
        });

        lastAccelerometer = accelerometerData.timestamp;
    }
}

- (BOOL)startMotionCaptureWithTimer:(NSTimer *)timer
{
    timerMotion = timer;
    if(timerMotion == nil)
    {
        NSLog(@"Failed to start motion capture. Timer is nil");
        return NO;
    }

    lastAccelerometer = lastGyro = 0.;
    [cmMotionManager startGyroUpdates];
    [cmMotionManager startAccelerometerUpdates];
    return true;
}

/** @returns True if successfully started motion capture. False if setupMotionCapture has not been called, or plugins not started. */
- (BOOL) startMotionCapture
{
    if(cmMotionManager == nil)
    {
        NSLog(@"Failed to start motion capture. Motion Manager is nil");
        return NO;
    }

    if (!isCapturing)
    {
        //Timer interval of .011 determined experimentally using DEBUG_TIMER - setting it to .01 makes the gyro rate drop dramatically
        [cmMotionManager setAccelerometerUpdateInterval:.011];
        [cmMotionManager setGyroUpdateInterval:.011];
        [self startMotionCaptureWithTimer:[NSTimer scheduledTimerWithTimeInterval:.011 target:self selector:@selector(timerCallback:) userInfo:nil repeats:true]];
    }
    return true;
}

- (void) stopMotionCapture
{
    if(timerMotion) [timerMotion invalidate];

    if(cmMotionManager)
    {
        if(cmMotionManager.isAccelerometerActive) [cmMotionManager stopAccelerometerUpdates];
        if(cmMotionManager.isGyroActive) [cmMotionManager stopGyroUpdates];
    }

    timerMotion = nil;
}

- (void) openStream:(const char *)path
{
    outputQueue = dispatch_queue_create("CaptureStreamQueue", DISPATCH_QUEUE_SERIAL);
    outputChannel = dispatch_io_create_with_path(DISPATCH_IO_STREAM, path,  O_CREAT | O_RDWR | O_TRUNC, 0644, outputQueue, ^(int error){
        NSLog(@"Finished dispatch io");
        if(error)
            NSLog(@"Closed with error %d", error);
        else if(delegate) {
            //send the callback to the main/ui thread
            dispatch_async(dispatch_get_main_queue(), ^{
                [delegate captureDidStop];
            });
        }
    });
}

- (void)startCapture:(NSString *)path withSession:(AVCaptureSession *)avSession withDevice:(AVCaptureDevice *)avDevice withDelegate:(id<RCCaptureManagerDelegate>)captureDelegate
{
    if (isCapturing) return;

    hasFocused = false;
    [self openStream:[path cStringUsingEncoding:NSUTF8StringEncoding]];
    [self startVideoCapture:avSession withDevice:avDevice];
    [self startMotionCapture];

    self.delegate = captureDelegate;
}

- (void) stopCapture
{
    if (isCapturing)
    {
        [self stopVideoCapture];
        [self stopMotionCapture];

        isCapturing = NO;
        dispatch_io_close(outputChannel, 0); // DISPATCH_IO_STOP
    }
}

@end
