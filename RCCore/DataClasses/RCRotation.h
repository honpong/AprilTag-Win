//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#include "RCVector.h"
#include "RCPoint.h"
#include "RCTranslation.h"

@interface RCRotation : RCVector

- (void) getOpenGLMatrix:(float[16])matrix;
- (RCPoint *) transformPoint:(RCPoint *)point;
- (RCTranslation *) transformTranslation:(RCTranslation *)translation;
- (RCRotation *) getInverse;
- (RCRotation *) composeWithRotation:(RCRotation *)other;

@end