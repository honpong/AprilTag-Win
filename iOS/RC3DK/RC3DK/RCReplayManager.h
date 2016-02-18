//
//  ReplayController.h
//  RCCapture
//
//  Created by Brian on 2/18/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "RCSensorFusion.h"

@protocol RCReplayManagerDelegate <NSObject>

@optional
- (void) replayProgress:(float)progress;
- (void) replayFinishedWithLength:(float)length withPathLength:(float)pathLength;

@end

@interface RCReplayManager : NSObject

@property (weak, nonatomic) id<RCReplayManagerDelegate> delegate;

- (void)setupWithPath:(NSString *)path withRealtime:(BOOL)isRealtime withWidth:(int)width withHeight:(int)height withFramerate:(int)framerate;
- (void)startReplay;
- (void)stopReplay;

+ (RCReplayManager*) sharedInstance;

@end
