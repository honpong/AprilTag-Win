//
//  TMCorVisManagerFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <CoreMedia/CoreMedia.h>
#include "feature_info.h"
#import "RCMeasurementManagerDelegate.h"

@protocol RCCorvisManager <NSObject>

@property (weak) id<RCMeasurementManagerDelegate> delegate;

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
+ (id<RCCorvisManager>) getInstance;

#ifdef DEBUG
+ (void) setInstance:(id<RCCorvisManager>)mockObject;
#endif

@end
