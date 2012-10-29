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

- (AVCaptureSession*)startVideoCap
{
	NSLog(@"Starting video capture");
	
	NSError * error;
	session = [[AVCaptureSession alloc] init];
	
    [session beginConfiguration];
    [session setSessionPreset:AVCaptureSessionPreset640x480];
	
    AVCaptureDevice * videoDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
    AVCaptureDeviceInput *input = [AVCaptureDeviceInput deviceInputWithDevice:videoDevice error:&error];
    [session addInput:input];
	
    AVCaptureVideoDataOutput * dataOutput = [[AVCaptureVideoDataOutput alloc] init];
    [dataOutput setAlwaysDiscardsLateVideoFrames:NO];
    [dataOutput setVideoSettings:[NSDictionary dictionaryWithObject:[NSNumber numberWithInt:kCVPixelFormatType_32BGRA] forKey:(id)kCVPixelBufferPixelFormatTypeKey]];
	
	dispatch_queue_t queue = dispatch_queue_create("MyQueue", NULL); //docs "You use the queue to modify the priority given to delivering and processing the video frames."
	[dataOutput setSampleBufferDelegate:self queue:queue];
	dispatch_release(queue);
	
    [session addOutput:dataOutput];
    [session commitConfiguration];
    [session startRunning];
	
	return session;
}

//called on each video frame
-(void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{
	CMTime timestamp = (CMTime)CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
	
	NSLog(@"%f video frame received", (double)timestamp.value / (double)timestamp.timescale);
	
	CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)CMSampleBufferGetImageBuffer(sampleBuffer);
	
    CIImage *image = [CIImage imageWithCVPixelBuffer:pixelBuffer];
	
	//black and white filter
	image = [CIFilter filterWithName:@"CIColorControls" keysAndValues:kCIInputImageKey, image, @"inputBrightness", [NSNumber numberWithFloat:0.0], @"inputContrast", [NSNumber numberWithFloat:1.1], @"inputSaturation", [NSNumber numberWithFloat:0.0], nil].outputImage;
	
	//pass packet
}

- (void)stopVideoCap
{
	NSLog(@"Stopping video capture");
	[session stopRunning];
}
@end
