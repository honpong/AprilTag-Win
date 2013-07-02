//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#include "RCScalar.h"
#include "RCPoint.h"

/** Represents a point in space where a visual feature was tracked. */
@interface RCFeaturePoint : NSObject

// TODO: document
@property (nonatomic, readonly) uint64_t id;
@property (nonatomic, readonly) float x, y;
@property (nonatomic, readonly) RCScalar *depth;
@property (nonatomic, readonly) RCPoint *worldPoint;
@property (nonatomic, readonly) bool initialized;

/** You will not typically need to instantiate this class yourself. */
- (id) initWithId:(uint64_t)id withX:(float)x withY:(float)y withDepth:(RCScalar *)depth withWorldPoint:(RCPoint *)worldPoint withInitialized:(bool)initialized;
- (NSDictionary*) dictionaryRepresenation;

@end