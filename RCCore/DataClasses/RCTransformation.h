//
//  RCTransformation.h
//  RCCore
//
//  Created by Ben Hirashima on 6/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCTranslation.h"
#import "RCRotation.h"

/**
 Represents the transformation of the device in space, relative to the sensor fusion starting point.
 */
@interface RCTransformation : NSObject

/** An RCTranslation object that represents the translation of the device relative to the starting point. */
@property (nonatomic, readonly) RCTranslation* translation;
/** An RCRotation object that represents the rotation of the device. */
@property (nonatomic, readonly) RCRotation* rotation;

/** You will not typically need to instantiate this class yourself. */
- (id) initWithTranslation:(RCTranslation*)translation withRotation:(RCRotation*)rotation;
- (void) getOpenGLMatrix:(float[16])matrix;
- (RCPoint *) transformPoint:(RCPoint *)point;
- (RCTransformation *) getInverse;
- (RCTransformation *) composeWithTransformation:(RCTransformation *)other;

@end
