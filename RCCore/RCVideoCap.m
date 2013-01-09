//
//  RCVideoCap.m
//  RCCore
//
//  Created by Ben Hirashima on 10/29/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "RCVideoCap.h"
#import <CoreImage/CoreImage.h>
#include <stdio.h>
#import <ImageIO/ImageIO.h>

@implementation RCVideoCap

- (id)initWithSession:(AVCaptureSession*)session withOutput:(struct outbuffer *) output
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
        [_avDataOutput setVideoSettings:[NSDictionary dictionaryWithObject:[NSNumber numberWithInt:'420f'] forKey:(id)kCVPixelBufferPixelFormatTypeKey]];
        
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
        
        //capture image meta data
//        CFDictionaryRef metadataDict = CMGetAttachment(sampleBuffer, kCGImagePropertyExifDictionary , NULL);
//        NSLog(@"metadata: %@", metadataDict);
        
        CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)CMSampleBufferGetImageBuffer(sampleBuffer);
        
        //pass packet
        uint32_t width = CVPixelBufferGetWidth(pixelBuffer);
        uint32_t height = CVPixelBufferGetHeight(pixelBuffer);
        packet_t *buf = outbuffer_alloc(_output, packet_camera, width*height + 16); // 16 bytes for pgm header

        sprintf((char *)buf->data, "P5 %4d %3d %d\n", width, height, 255);
        char *outbase = buf->data + 16;
        CVPixelBufferLockBaseAddress(pixelBuffer, 0);
        unsigned char *pixel = (unsigned char *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer,0);
        memcpy(outbase, pixel, width*height);
        
        CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
        uint64_t time_us = timestamp.value / (timestamp.timescale / 1000000.);
        outbuffer_enqueue(_output, buf, time_us);
    }
}

@end
