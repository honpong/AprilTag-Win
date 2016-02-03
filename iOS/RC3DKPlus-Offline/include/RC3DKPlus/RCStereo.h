//
//  RCStereo.h
//  RCCore
//
//  Created by Eagle Jones on 5/20/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>
#import "RC3DK.h"

/** The delegate of RCStereoDelegate must implement this protocol in order to receive status updates. */
@protocol RCStereoDelegate <NSObject>

/** Sent to the delegate to provide the progress of the post-capture processing.
 
 After stopping sensor fusion, some post-processing is required. The delegate is called periodically with an update on the progress of this processing.
 @param progress Current progress, between 0 and 1.
 */
- (void) stereoDidUpdateProgress:(float)progress;

/** Sent to the delegate to confirm post-processing has finished.
 
 The delegate is called once after all post-processing steps have been completed
 */
- (void) stereoDidFinish;
@end

@interface RCStereo : NSObject

@property (weak) id<RCStereoDelegate> delegate;
@property (nonatomic, readonly) NSURL* workingDirectory;
@property (nonatomic, readonly) NSString* fileBaseName;
@property (nonatomic, readonly) NSString* guid;
@property (nonatomic, readonly) UIDeviceOrientation orientation;

+ (RCStereo *) sharedInstance;

- (void) setWorkingDirectory:(NSURL *)workingDirectory andGuid:(NSString*)guid andOrientation:(UIDeviceOrientation)orientation;
- (void) processFrame:(RCSensorFusionData *) data withFinal:(bool)final;
- (bool) preprocess;
- (RCPoint *) triangulatePoint:(CGPoint)point;
- (RCPoint *) triangulatePointWithMesh:(CGPoint)point;
- (void) reset;

@end

