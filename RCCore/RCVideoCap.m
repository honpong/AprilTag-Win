//
//  RCVideoCap.m
//  RCCore
//
//  Created by Ben Hirashima on 10/29/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "RCVideoCap.h"
#import <CoreImage/CoreImage.h>

@implementation RCVideoCap

- (id)initWithSession:(AVCaptureSession*)session withOutput:(struct mapbuffer *) output
{
    if(self = [super init])
    {
        _session = session;
        
        if(!_session.isRunning)
        {
            NSLog(@"Failed to init video capture. Session not running.");
            return nil;
        }
        
        _output = output;

        _avDataOutput = [[AVCaptureVideoDataOutput alloc] init];
        [_avDataOutput setAlwaysDiscardsLateVideoFrames:NO];
        [_avDataOutput setVideoSettings:[NSDictionary dictionaryWithObject:[NSNumber numberWithInt:kCVPixelFormatType_32BGRA] forKey:(id)kCVPixelBufferPixelFormatTypeKey]];
        
        dispatch_queue_t queue = dispatch_queue_create("MyQueue", NULL); //docs "You use the queue to modify the priority given to delivering and processing the video frames."
        [_avDataOutput setSampleBufferDelegate:self queue:queue];
        dispatch_release(queue);
        
        [_session addOutput:_avDataOutput];
        
        isCapturing = NO;
    }
    
    return self;
}

- (void)startVideoCap
{
	NSLog(@"Starting video capture");
    
    isCapturing = YES;
}

- (void)stopVideoCap
{
	NSLog(@"Stopping video capture");
    
    //    [_avDataOutput setSampleBufferDelegate:nil queue:nil]; //doesn't work. frames keep coming in.
    
    isCapturing = NO;
}

//called on each video frame
-(void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{
	if(isCapturing)
    {
        CMTime timestamp = (CMTime)CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
        
        NSLog(@"%f video frame received", (double)timestamp.value / (double)timestamp.timescale);
        
        CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)CMSampleBufferGetImageBuffer(sampleBuffer);
        
        CIImage *image = [CIImage imageWithCVPixelBuffer:pixelBuffer];
        
        //black and white filter
        image = [CIFilter filterWithName:@"CIColorControls" keysAndValues:kCIInputImageKey, image, @"inputBrightness", [NSNumber numberWithFloat:0.0], @"inputContrast", [NSNumber numberWithFloat:1.1], @"inputSaturation", [NSNumber numberWithFloat:0.0], nil].outputImage;
        
        //pass packet
    }
}

@end
