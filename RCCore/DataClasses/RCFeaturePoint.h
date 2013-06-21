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

- (id) initWithId:(uint64_t)id withX:(float)x withStdX:(float)stdx withY:(float)y withStdY:(float)stdy withDepth:(float)depth withStdDepth:(float)stdDepth withWx:(float)wx withStdWx:(float)stdwx withWy:(float)wy withStdWy:(float)stdwy withWz:(float)wz withStdWz:(float)stdwz;

@end