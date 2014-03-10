//
//  ArcBall.h
//  RC3DKSampleGL
//
//  Created by Brian on 3/4/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <GLKit/GLKit.h>

@interface ArcBall : NSObject

- (void) startRotation:(CGPoint)location;
- (void) continueRotation:(CGPoint)newLocation;
- (void) startViewRotation:(float)rotation;
- (void) continueViewRotation:(float)newRotation;
- (GLKMatrix4) rotateMatrixByArcBall:(GLKMatrix4)matrix;

@end
