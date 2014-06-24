//
//  RCStereo.h
//  RCCore
//
//  Created by Eagle Jones on 5/20/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "RC3DK/RCFeaturePoint.h"
#import "RC3DK/RCSensorFusion.h"

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

+ (RCStereo *) sharedInstance;

- (void) processFrame:(RCSensorFusionData *) data withFinal:(bool)final;
- (bool) preprocess;
- (RCFeaturePoint *) triangulatePoint:(CGPoint)point;
- (RCFeaturePoint *) triangulatePointWithMesh:(CGPoint)point;
- (void) reset;
@end

