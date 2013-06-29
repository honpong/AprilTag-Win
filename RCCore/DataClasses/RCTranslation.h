//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#import "RCVector.h"
#import "RCPoint.h"
#import "RCScalar.h"

@interface RCTranslation : RCVector

- (RCPoint *)transformPoint:(RCPoint *)point;
- (RCScalar *)getDistance;
- (RCTranslation *)getInverse;
- (RCTranslation *)composeWithTranslation:(RCTranslation *)other;

@end