//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#import "RCVector.h"
#import "RCPoint.h"
#import "RCScalar.h"

/** Represents the translation of the device relative to the starting point. */
@interface RCTranslation : RCVector

/** TODO: document
 @param point An RCPoint object
 @returns An RCPoint object
 */
- (RCPoint *)transformPoint:(RCPoint *)point;

/** TODO: document
 @returns An RCScalar object
 */
- (RCScalar *)getDistance;

/** TODO: document
 @returns An RCTranslation object
 */
- (RCTranslation *)getInverse;

/** TODO: document
 @param other An RCTranslation object
 @returns An RCTranslation object
 */
- (RCTranslation *)composeWithTranslation:(RCTranslation *)other;

@end