//
//  TMCorVisManagerFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <CoreMedia/CoreMedia.h>
@protocol RCCorvisManager <NSObject>

typedef void (^filterStatusCallback)(bool is_measuring, float x, float stdx, float y, float stdy, float z, float stdz, float path, float stdpath, float rx, float stdrx, float ry, float stdry, float rz, float stdrz, float orientx, float orienty, int code, float converged, bool steady, bool aligned, bool speed_warning, bool vision_warning, bool vision_failure, bool speed_failure, bool other_failure);

- (void)setupPluginsWithFilter:(bool)filter withCapture:(bool)capture withReplay:(bool)replay withLocationValid:(bool)locationValid withLatitude:(double)latitude withLongitude:(double)longitude withAltitude:(double)altitude withStatusCallback:(filterStatusCallback)_statusCallback;
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

@end

@interface RCCorvisManagerFactory : NSObject
+ (id<RCCorvisManager>)getCorvisManagerInstance;
+ (void)setCorvisManagerInstance:(id<RCCorvisManager>)mockObject;
@end
