//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

@interface RCTranslation : NSObject

@property (nonatomic, readonly) float x, y, z, path;
@property (nonatomic, readonly) float stdx, stdy, stdz, stdPath;

- (id) initWithX:(float)x withStdX:(float)stdx withY:(float)y withStdY:(float)stdy withZ:(float)z withStdZ:(float)stdz withPath:(float)path withStdPath:(float)stdPath;

@end