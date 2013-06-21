//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

@interface RCFeaturePoint : NSObject

@property (nonatomic, readonly) uint64_t id;
@property (nonatomic, readonly) float x, y, stdx, stdy;
@property (nonatomic, readonly) float depth, stdDepth;
@property (nonatomic, readonly) float wx, wy, wz, stdwx, stdwy, stdwz;

@end