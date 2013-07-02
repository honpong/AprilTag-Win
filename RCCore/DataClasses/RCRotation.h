//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#include "RCVector.h"
#include "RCPoint.h"
#include "RCTranslation.h"

/** Represents the rotation of the device. */
@interface RCRotation : RCVector

/** TODO: document
 @param matrix A matrix
 */
- (void) getOpenGLMatrix:(float[16])matrix;

/** TODO: document
 @param point An RCPoint object
 @returns An RCPoint object
 */
- (RCPoint *) transformPoint:(RCPoint *)point;

/** TODO: document
 @param translation An RCTranslation object
 @returns An RCTranslation object
 */
- (RCTranslation *) transformTranslation:(RCTranslation *)translation;

/** TODO: document
 @returns An RCRotation object
 */
- (RCRotation *) getInverse;

/** TODO: document
 @param other An RCRotation object
 @returns An RCRotation object
 */
- (RCRotation *) composeWithRotation:(RCRotation *)other;

@end