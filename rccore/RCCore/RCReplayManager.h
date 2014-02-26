//
//  ReplayController.h
//  RCCapture
//
//  Created by Brian on 2/18/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

#import <Foundation/Foundation.h>

#import <RC3DK/RC3DK.h>

#include "CaptureFile.h"

@protocol RCReplayManagerDelegate <NSObject>

@optional
- (void) replayProgress:(float)progress;
- (void) replayFinished;

@end

@interface RCReplayManager : NSObject <RCSensorFusionDelegate>

@property (weak, nonatomic) id<RCReplayManagerDelegate> delegate;

- (void)startReplay:(NSString *)path withCalibration:(NSString *)calibrationPath withRealtime:(BOOL)isRealtime;
- (void)stopReplay;

+ (NSString *)getFirstReplayFilename;
+ (NSString *)getFirstCalibrationFilename;

@end
