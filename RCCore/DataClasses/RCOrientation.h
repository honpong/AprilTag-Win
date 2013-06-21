//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

@interface RCOrientation : NSObject

@property (nonatomic, readonly) float rx, ry, rz;
@property (nonatomic, readonly) float stdrx, stdry, stdrz;

- (id) initWithRx:(float)rx withStdRx:(float)stdrx withRy:(float)ry withStdRy:(float)stdry withRz:(float)rz withStdRz:(float)stdrz;

@end