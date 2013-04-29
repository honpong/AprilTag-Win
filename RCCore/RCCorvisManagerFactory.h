//
//  TMCorVisManagerFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <CoreMedia/CoreMedia.h>
@protocol RCCorvisManager <NSObject>

- (void)setupPluginsWithFilter:(bool)filter withCapture:(bool)capture withReplay:(bool)replay withLocationValid:(bool)locationValid withLatitude:(double)latitude withLongitude:(double)longitude withAltitude:(double)altitude withUpdateProgress:(void(*)(void *, float))updateProgress withUpdateMeasurement:(void(*)(void *, float, float, float, float, float, float, float, float, float, float, float, float, float, float))updateMeasurement withCallbackObject:(void *)callbackObject;
- (void)teardownPlugins;
- (void)startPlugins;
- (void)stopPlugins;
- (BOOL)isPluginsStarted;
- (void)startMeasurement;
- (void)stopMeasurement;
- (void)receiveVideoFrame:(unsigned char*)pixel withWidth:(uint32_t)width withHeight:(uint32_t)height withTimestamp:(CMTime)timestamp;
- (void)receiveAccelerometerData:(double)timestamp withX:(double)x withY:(double)y withZ:(double)z;
- (void)receiveGyroData:(double)timestamp withX:(double)x withY:(double)y withZ:(double)z;
- (void)getProjectedOrientationWithX:(float *)x withY:(float *)y;

@end

@interface RCCorvisManagerFactory : NSObject
+ (id<RCCorvisManager>)getCorvisManagerInstance;
+ (void)setCorvisManagerInstance:(id<RCCorvisManager>)mockObject;
@end
