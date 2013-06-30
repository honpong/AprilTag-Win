//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#import "RCRotation.h"
#import "vec4.h"

@implementation RCRotation

- (void) getOpenGLMatrix:(float[16])matrix
{
    m4 rot = rodrigues(self.vector, NULL);
    //transpose for OpenGL
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            matrix[j * 4 + i] = rot[i][j];
        }
    }
}

- (RCPoint *)transformPoint:(RCPoint *)point
{
    m4 R = rodrigues(self.vector, NULL);
    v4 rotated = R * point.vector;
    //TODO: account for standard deviation of rotation in addition to that of the point
    v4 stdev = R * point.standardDeviation * transpose(R);
    return [[RCPoint alloc] initWithVector:rotated withStandardDeviation:stdev];
}

- (RCTranslation *)transformTranslation:(RCTranslation *)translation
{
    m4 R = rodrigues(self.vector, NULL);
    v4 rotated = R * translation.vector;
    //TODO: account for standard deviation of rotation in addition to that of the point
    v4 stdev = R * translation.standardDeviation * transpose(R);
    return [[RCTranslation alloc] initWithVector:rotated withStandardDeviation:stdev];
}

- (RCRotation *)getInverse
{
    return [[RCRotation alloc] initWithVector:-self.vector withStandardDeviation:self.standardDeviation];
}

- (RCRotation *)composeWithRotation:(RCRotation *)other
{
    //TODO: standard deviation
    m4 R = rodrigues(self.vector, NULL);
    m4 R2 = rodrigues(other.vector, NULL);
    v4 res = invrodrigues(R * R2, NULL);
    return [[RCRotation alloc] initWithVector:res withStandardDeviation:self.standardDeviation];
}

@end