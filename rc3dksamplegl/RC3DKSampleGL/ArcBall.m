//
//  ArcBall.m
//  RC3DKSampleGL
//
//  Created by Brian on 3/4/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "ArcBall.h"

#import <GLKit/GLKit.h>

@implementation ArcBall
{
    CGPoint touchStart;
    GLKQuaternion quarternion;
    float viewRotationStart;
}
#define RADIANS_PER_PIXEL (M_PI / 640.f)

- (id) init
{
    self = [super init];
    if(self) {
        quarternion = GLKQuaternionIdentity;
        touchStart = CGPointMake(0.f, 0.f);
    }
    return self;
}

- (void) printMatrix4:(GLKMatrix4)matrix
{
    for(int i = 0; i < 16; i++) {
        fprintf(stderr, "%f ", matrix.m[i]);
        if(i != 0 && (i+1) % 4 == 0)
            fprintf(stderr, "\n");
    }
}
- (GLKMatrix4) rotateMatrixByArcBall:(GLKMatrix4)matrix
{
	GLKVector3 axis = GLKQuaternionAxis(quarternion);
	float angle = GLKQuaternionAngle(quarternion);
    if(angle != 0.f)
        matrix = GLKMatrix4Rotate(matrix, angle, axis.x, axis.y, axis.z);
    return matrix;
}

- (void) rotateQuaternionWithVector:(CGPoint)delta
{
	GLKVector3 up = GLKVector3Make(0.f, 1.f, 0.f);
	GLKVector3 right = GLKVector3Make(-1.f, 0.f, 0.f);

	up = GLKQuaternionRotateVector3( GLKQuaternionInvert(quarternion), up );
	quarternion = GLKQuaternionMultiply(quarternion, GLKQuaternionMakeWithAngleAndVector3Axis(delta.x * RADIANS_PER_PIXEL, up));

	right = GLKQuaternionRotateVector3( GLKQuaternionInvert(quarternion), right );
	quarternion = GLKQuaternionMultiply(quarternion, GLKQuaternionMakeWithAngleAndVector3Axis(delta.y * RADIANS_PER_PIXEL, right));
}

- (void) startRotation:(CGPoint)location
{
	touchStart = location;
}

- (void) continueRotation:(CGPoint)newLocation
{
	// get touch delta
	CGPoint delta = CGPointMake(newLocation.x - touchStart.x, -(newLocation.y - touchStart.y));

	// rotate
	[self rotateQuaternionWithVector:delta];

    // reset starting point
    touchStart = newLocation;
}

- (void) startViewRotation:(float)rotation
{
    viewRotationStart = rotation;
}

- (void) continueViewRotation:(float)newRotation
{
    GLKVector3 axis = GLKVector3Make(0, 0, -1);
    axis = GLKQuaternionRotateVector3( GLKQuaternionInvert(quarternion), axis );

    float delta = newRotation - viewRotationStart;
    GLKQuaternion viewQuaternion = GLKQuaternionMakeWithAngleAndVector3Axis(delta, axis);

    quarternion = GLKQuaternionMultiply(quarternion, viewQuaternion);
    viewRotationStart = newRotation;
}
@end
