//
//  RCVideoManager.m
//
//  Created by Ben Hirashima on 1/17/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCVideoManager.h"
#import "RCDebugLog.h"
#import "RC3DK.h"

@implementation RCVideoManager
{
    bool isCapturing;
    NSMutableArray *_delegates;
}
@synthesize videoOrientation, session, output;

+ (RCVideoManager *) sharedInstance
{
    static RCVideoManager* instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[RCVideoManager alloc] init];
    });
    return instance;
}


- (id)init
{
    if(self = [super init])
    {
        LOGME
        isCapturing = NO;
        _delegates = [NSMutableArray new];
        [self addDelegate:[RCSensorFusion sharedInstance]];
    }
    return self;
}

- (NSArray*) delegates
{
    return [NSArray arrayWithArray:_delegates];
}

- (void)addDelegate:(id <RCVideoFrameDelegate>)videoDelegate
{
    if (videoDelegate) [_delegates addObject:videoDelegate];
}

- (void)removeDelegate:(id <RCVideoFrameDelegate>)videoDelegate
{
    if (videoDelegate) [_delegates removeObject:videoDelegate];
}

/** Invocations after the first have no effect */
- (void) setupWithSession:(AVCaptureSession*)avSession
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        LOGME
        
        AVCaptureVideoDataOutput* avOutput = [[AVCaptureVideoDataOutput alloc] init];
        [self setupWithSession:avSession withOutput:avOutput];
    });
}

- (void) setupWithSession:(AVCaptureSession *)avSession withOutput:(AVCaptureVideoDataOutput *)avOutput
{
    session = avSession;
    output = avOutput;

    //causes lag
    [self.session addOutput:self.output];
    
    [avOutput setAlwaysDiscardsLateVideoFrames:YES];
    [avOutput setVideoSettings:@{(id)kCVPixelBufferPixelFormatTypeKey: [NSNumber numberWithInt:'420f']}];

    AVCaptureConnection *videoConnection = [self.output connectionWithMediaType:AVMediaTypeVideo];
    videoOrientation = [videoConnection videoOrientation];
            
    isCapturing = NO;

    dispatch_queue_t queue = dispatch_queue_create("MyQueue", DISPATCH_QUEUE_SERIAL); //docs "You use the queue to modify the priority given to delivering and processing the video frames."
    [output setSampleBufferDelegate:self queue:queue];
//    dispatch_release(queue); // illegal on iOS 6+
}

/** @returns True if successfully started. False if setupWithSession: was not called first, or AV session not running. */
- (bool) startVideoCapture
{
	LOGME
    
    if(session == nil || ![session isRunning])
    {
        DLog(@"Failed to start video capture. AV capture session not running.");
        return false;
    }
    
    isCapturing = YES;
    
    return true;
}

- (void) stopVideoCapture
{
    LOGME
    isCapturing = NO;
}

- (BOOL)isCapturing
{
    return isCapturing;
}

//called on each video frame
-(void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{
    sampleBuffer = (CMSampleBufferRef)CFRetain(sampleBuffer);
  
    for (id<RCVideoFrameDelegate> delegate in _delegates)
    {
        if ([delegate respondsToSelector:@selector(receiveVideoFrame:)]) [delegate receiveVideoFrame:sampleBuffer];
    }
    
    CFRelease(sampleBuffer);
}

@end
