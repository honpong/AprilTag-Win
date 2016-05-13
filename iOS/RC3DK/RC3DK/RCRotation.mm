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
    return (float)q.w();
}

- (float) quaternionX
{
    return (float)q.x();
}

- (float) quaternionY
{
    return (float)q.y();
}

- (float) quaternionZ
{
    return (float)q.z();
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
        float st = sinf(theta/2.f);
        float w = cosf(theta/2.f);
        float x = ax * st;
        float y = ay * st;
        float z = az * st;
        q = quaternion(w, x, y, z);
    }
    return self;
}


- (void) getOpenGLMatrix:(float[16])matrix
{
    m3 rot = to_rotation_matrix(q);
    //transpose for OpenGL
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            matrix[j * 4 + i] = (float)rot(i, j);
        }
    }
}

- (RCPoint *)transformPoint:(RCPoint *)point
{
    m3 R = to_rotation_matrix(q);
    v3 rotated = R * v3_from_vFloat(point.vector);
    //TODO: account for standard deviation of rotation in addition to that of the point
    v3 stdev = (R * v3_from_vFloat(point.standardDeviation)).transpose() * R.transpose();
    return [[RCPoint alloc] initWithVector:vFloat_from_v3(rotated) withStandardDeviation:vFloat_from_v3(stdev)];
}

- (RCTranslation *)transformTranslation:(RCTranslation *)translation
{
    m3 R = to_rotation_matrix(q);
    v3 rotated = R * v3_from_vFloat(translation.vector);
    //TODO: account for standard deviation of rotation in addition to that of the point
    v3 stdev = (R * v3_from_vFloat(translation.standardDeviation)).transpose() * R.transpose();
    return [[RCTranslation alloc] initWithVector:vFloat_from_v3(rotated) withStandardDeviation:vFloat_from_v3(stdev)];
}

- (RCRotation *)getInverse
{
    quaternion i = conjugate(q);
    return [[RCRotation alloc] initWithQuaternionW:(float)i.w() withX:(float)i.x() withY:(float)i.y() withZ:(float)i.z()];
}

- (RCRotation *)composeWithRotation:(RCRotation *)other
{
    //TODO: standard deviation
    quaternion a = q * quaternion(other.quaternionW, other.quaternionX, other.quaternionY, other.quaternionZ);
    return [[RCRotation alloc] initWithQuaternionW:(float)a.w() withX:(float)a.x() withY:(float)a.y() withZ:(float)a.z()];
}

- (RCRotation *)flipAxis:(int)axis
{
    m3 R = to_rotation_matrix(q);
    m3 flip = m3::Identity();
    flip(axis, axis) = -1;
    quaternion res = to_quaternion(flip * R * flip);
    return [[RCRotation alloc] initWithQuaternionW:(float)res.w() withX:(float)res.x() withY:(float)res.y() withZ:(float)res.z()];
}

- (NSDictionary*) dictionaryRepresentation
{
    return @{ @"qw": @(self.quaternionW), @"qx": @(self.quaternionX), @"qy": @(self.quaternionY), @"qz": @(self.quaternionZ) };
}

@end