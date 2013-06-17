//
//  RCMeasurementManagerDelegate.h
//  RCCore
//
//  Created by Ben Hirashima on 6/17/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

@protocol RCMeasurementManagerDelegate <NSObject>

- (void) didUpdateMeasurementStatus:(bool)measurement_active code:(int)code converged:(float)converged steady:(bool)steady aligned:(bool)aligned speed_warning:(bool)speed_warning vision_warning:(bool)vision_warning vision_failure:(bool)vision_failure speed_failure:(bool)speed_failure other_failure:(bool)other_failure;
- (void) didUpdateMeasurementData:(float)x stdx:(float)stdx y:(float)y stdy:(float)stdy z:(float)z stdz:(float)stdz path:(float)path stdpath:(float)stdpath rx:(float)rx stdrx:(float)stdrx ry:(float)ry stdry:(float)stdry rz:(float)rz stdrz:(float)stdrz;
- (void) didUpdatePose:(float)x withY:(float)y;

@end
