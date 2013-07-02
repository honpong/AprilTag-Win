//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#include "RCScalar.h"
#include "RCPoint.h"

@interface RCFeaturePoint : NSObject

@property (nonatomic, readonly) uint64_t id;
@property (nonatomic, readonly) float x, y;
@property (nonatomic, readonly) RCScalar *depth;
@property (nonatomic, readonly) RCPoint *worldPoint;
@property (nonatomic, readonly) bool initialized;

- (id) initWithId:(uint64_t)id withX:(float)x withY:(float)y withDepth:(RCScalar *)depth withWorldPoint:(RCPoint *)worldPoint withInitialized:(bool)initialized;
- (NSDictionary*) dictionaryRepresenation;

@end