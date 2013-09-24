//
//  RCTransformation.h
//  RCCore
//
//  Created by Ben Hirashima on 6/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCTranslation.h"
#import "RCRotation.h"

/** Represents a 3D transformation. */
@interface RCTransformation : NSObject

/** An RCTranslation object that represents the translation component. */
@property (nonatomic, readonly) RCTranslation* translation;

/** An RCRotation object that represents the rotation component. */
@property (nonatomic, readonly) RCRotation* rotation;

/** Instantiate an RCTransformation composed of the given rotation and translation.
 
 @param translation An RCTranslation object representing the desired translation.
 @param rotation An RCRotation object representing the desired rotation.
 */
- (id) initWithTranslation:(RCTranslation*)translation withRotation:(RCRotation*)rotation;

/** Fills in a matrix representation of the transformation that is compatible with OpenGL.
 
 This matrix can be loaded in an OpenGL ES 2.0 shader by calling glUniformMatrix4fv, with transpose set to false.
 @param matrix A C-style array of 16 floats that will be overwritten with the transformation matrix.
 */
- (void) getOpenGLMatrix:(float[16])matrix;

/** Apply the transformation to a point.
 
 The rotation is applied first, then the translation.
 @param point An RCPoint object
 @returns The transformed version of the point.
 */
- (RCPoint *) transformPoint:(RCPoint *)point;

/** Compute the inverse transformation.
 @returns The inverse of the transformation.
 */
- (RCTransformation *) getInverse;

/** Compute the composition of two transformations.
 
 The composition is performed in such a way that, if T1 and T2 are RCTransformation objects, and pt is an RCPoint object, the following two lines would produce the same results (up to numerical precision):
 
    [T1 composeWithTransformation:T2] transformPoint:pt];
    [T1 transformPoint:[T2 transformPoint:pt]];
 
 @param other An RCTransformation object
 @returns An RCTransformation object representing the combined transformation. */
- (RCTransformation *) composeWithTransformation:(RCTransformation *)other;

/** Return a dictionary representation of the transformation. */
- (NSDictionary*) dictionaryRepresentation;

@end
