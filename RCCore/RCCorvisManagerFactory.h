//
//  TMCorVisManagerFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <CoreMedia/CoreMedia.h>
#include "feature_info.h"

@protocol RCCorvisManagerDelegate <NSObject>

- (void) didUpdateMeasurementStatus:(bool)measurement_active code:(int)code converged:(float)converged steady:(bool)steady aligned:(bool)aligned speed_warning:(bool)speed_warning vision_warning:(bool)vision_warning vision_failure:(bool)vision_failure speed_failure:(bool)speed_failure other_failure:(bool)other_failure;
- (void) didUpdateMeasurementData:(float)x stdx:(float)stdx y:(float)y stdy:(float)stdy z:(float)z stdz:(float)stdz path:(float)path stdpath:(float)stdpath rx:(float)rx stdrx:(float)stdrx ry:(float)ry stdry:(float)stdry rz:(float)rz stdrz:(float)stdrz;
- (void) didUpdatePose:(float)x withY:(float)y;

@end

@protocol RCCorvisManager <NSObject>

@property (weak) id<RCCorvisManagerDelegate> delegate;

- (void)setupPluginsWithFilter:(bool)filter
                   withCapture:(bool)capture
                    withReplay:(bool)replay
             withLocationValid:(bool)locationValid
                  withLatitude:(double)latitude
                 withLongitude:(double)longitude
                  withAltitude:(double)altitude;
- (void)teardownPlugins;
- (void)startPlugins;
- (void)stopPlugins;
- (BOOL)isPluginsStarted;
- (void)startMeasurement;
- (void)stopMeasurement;
- (void)saveDeviceParameters;
- (void)receiveVideoFrame:(unsigned char*)pixel withWidth:(uint32_t)width withHeight:(uint32_t)height withTimestamp:(CMTime)timestamp;
- (void)receiveAccelerometerData:(double)timestamp withX:(double)x withY:(double)y withZ:(double)z;
- (void)receiveGyroData:(double)timestamp withX:(double)x withY:(double)y withZ:(double)z;
- (int)getCurrentFeatures:(struct corvis_feature_info *)features withMax:(int)max;
- (void)getCurrentCameraMatrix:(float [16])matrix withFocalCenterRadial:(float [5])focalCenterRadial withVirtualTapeStart:(float[3])start;
@end

@interface RCCorvisManagerFactory : NSObject
+ (id<RCCorvisManager>)getCorvisManagerInstance;
+ (void)setCorvisManagerInstance:(id<RCCorvisManager>)mockObject;
@end
