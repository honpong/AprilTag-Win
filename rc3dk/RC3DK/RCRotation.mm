//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#import "RCRotation.h"
#import "vec4.h"
#import "quaternion.h"

@implementation RCRotation
{
    quaternion q;
}

- (float) quaternionW
{
    return q.w();
}

- (float) quaternionX
{
    return q.x();
}

- (float) quaternionY
{
    return q.y();
}

- (float) quaternionZ
{
    return q.z();
}

- (id) initWithQuaternionW:(float)w withX:(float)x withY:(float)y withZ:(float)z
{
    if(self = [super init])
    {
        q = quaternion(w, x, y, z);
    }
    return self;
}

- (id) initWithAxisX:(float)ax withAxisY:(float)ay withAxisZ:(float)az withAngle:(float)theta
{
    if(self = [super init])
    {
        float st = sin(theta/2.);
        float w = cos(theta/2.);
        float x = ax * st;
        float y = ay * st;
        float z = az * st;
        q = quaternion(w, x, y, z);
    }
    return self;
}


- (void) getOpenGLMatrix:(float[16])matrix
{
    m4 rot = to_rotation_matrix(q);
    //transpose for OpenGL
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            matrix[j * 4 + i] = rot(i, j);
        }
    }
}

- (RCPoint *)transformPoint:(RCPoint *)point
{
    m4 R = to_rotation_matrix(q);
    v4 rotated = R * point.vector;
    //TODO: account for standard deviation of rotation in addition to that of the point
    v4 stdev = R * point.standardDeviation * transpose(R);
    return [[RCPoint alloc] initWithVector:rotated withStandardDeviation:stdev];
}

- (RCTranslation *)transformTranslation:(RCTranslation *)translation
{
    m4 R = to_rotation_matrix(q);
    v4 rotated = R * translation.vector;
    //TODO: account for standard deviation of rotation in addition to that of the point
    v4 stdev = R * translation.standardDeviation * transpose(R);
    return [[RCTranslation alloc] initWithVector:rotated withStandardDeviation:stdev];
}

- (RCRotation *)getInverse
{
    quaternion i = conjugate(q);
    return [[RCRotation alloc] initWithQuaternionW:i.w() withX:i.x() withY:i.y() withZ:i.z()];
}

- (RCRotation *)composeWithRotation:(RCRotation *)other
{
    //TODO: standard deviation
    quaternion a = quaternion_product(q, quaternion(other.quaternionW, other.quaternionX, other.quaternionY, other.quaternionZ));
    return [[RCRotation alloc] initWithQuaternionW:a.w() withX:a.x() withY:a.y() withZ:a.z()];
}

- (RCRotation *)flipAxis:(int)axis
{
    m4 R = to_rotation_matrix(q);
    m4 flip = m4::Identity();
    flip(axis, axis) = -1;
    quaternion res = to_quaternion(flip * R * flip);
    return [[RCRotation alloc] initWithQuaternionW:res.w() withX:res.x() withY:res.y() withZ:res.z()];
}

- (NSDictionary*) dictionaryRepresentation
{
    return @{ @"qw": @(self.quaternionW), @"qx": @(self.quaternionX), @"qy": @(self.quaternionY), @"qz": @(self.quaternionZ) };
}

@end