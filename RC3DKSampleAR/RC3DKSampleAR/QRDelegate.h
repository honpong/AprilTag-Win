//
//  QRDelegate.h
//  RC3DKSampleAR
//
//  Created by Brian on 11/7/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "RC3DK.h"

@protocol QRDetectionDelegate <NSObject>

/*
 (NSArray *)corners
 The value of this property is an array of CFDictionary objects, each of which has been created from a CGPoint struct using the CGPointCreateDictionaryRepresentation function, representing the coordinates of the corners of the object with respect to the image in which it resides.

 If the metadata originates from video, the points may be expressed as scalar values from 0 to 1.

 The points in the corners differ from the bounds rectangle in that bounds is axis aligned to orientation of the captured image, and the values of the corners reside within the bounds rectangle.

 The points are arranged in counterclockwise order (clockwise if the code or image is mirrored), starting with the top left of the code in its canonical orientation.

 // Convert the upper left corner to a CGPoint in image coordinates
 NSDictionary * upper_left = [corners objectAtIndex:0];
 CGPoint point;
 CGPointMakeWithDictionaryRepresentation((CFDictionaryRef)upper_left, &point);
 point.x *= 640;
 point.y *= 480;
 */
- (void) codeDetected:(NSString *)code withCorners:(NSArray *)corners withTransformation:(RCTransformation *)transformation withTimestamp:(uint64_t)timestamp;

@end

@interface QRDelegate : NSObject <AVCaptureMetadataOutputObjectsDelegate>

@property (nonatomic) id<QRDetectionDelegate> delegate;

- (void) addSensorFusionData:(RCSensorFusionData *)data;

- (void) captureOutput:(AVCaptureOutput *)captureOutput didOutputMetadataObjects:(NSArray *)metadataObjects fromConnection:(AVCaptureConnection *)connection;

@end
