//
//  TMCorVisManagerFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <CoreMedia/CoreMedia.h>

@protocol RCCorvisManager <NSObject>

- (void)setupPluginsWithFilter:(bool)filter withCapture:(bool)capture withReplay:(bool)replay withUpdateProgress:(void(*)(void *, float))updateProgress withUpdateMeasurement:(void(*)(void *, float, float, float, float, float, float, float, float))updateMeasurement withCallbackObject:(void *)callbackObject;
- (void)teardownPlugins;
- (void)startPlugins;
- (void)stopPlugins;
- (BOOL)isPluginsStarted;
- (void)receiveVideoFrame:(unsigned char*)pixel withWidth:(uint32_t)width withHeight:(uint32_t)height withTimestamp:(CMTime)timestamp;
- (void)receiveAccelerometerData:(uint64_t)timestamp withX:(long)x withY:(long)y withZ:(long)z;
- (void)receiveGyroData:(uint64_t)timestamp withX:(long)x withY:(long)y withZ:(long)z;

@end

@interface RCCorvisManagerFactory : NSObject
+ (id<RCCorvisManager>)getCorvisManagerInstance;
+ (void)setCorvisManagerInstance:(id<RCCorvisManager>)mockObject;
@end
