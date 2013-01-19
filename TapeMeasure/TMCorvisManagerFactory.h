//
//  TMCorVisManagerFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <CoreMedia/CoreMedia.h>
#import "RCCore/cor.h"

@protocol TMCorvisManager <NSObject>

- (void)startPlugins;
- (void)stopPlugins;
- (void)receiveVideoFrame:(unsigned char*)pixel withWidth:(uint32_t)width withHeight:(uint32_t)height withTimestamp:(CMTime)timestamp;
- (void)receiveAccelerometerData:(uint64_t)timestamp withX:(long)x withY:(long)y withZ:(long)z;
- (void)receiveGyroData:(uint64_t)timestamp withX:(long)x withY:(long)y withZ:(long)z;

@end

@interface TMCorvisManagerFactory : NSObject
+ (id<TMCorvisManager>)getCorvisManagerInstance;
+ (void)setCorvisManagerInstance:(id<TMCorvisManager>)mockObject;
@end
