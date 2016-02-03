//
//  RCTransformation.m
//  RCCore
//
//  Created by Ben Hirashima on 6/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCTransformation.h"

@implementation RCTransformation

- (id) initWithTranslation:(RCTranslation*)translation withRotation:(RCRotation*)rotation
{
    if (self = [super init])
    {
        _translation = translation;
        _rotation = rotation;
    }
    return self;
}

- (void) getOpenGLMatrix:(float[16])matrix
{
    [_rotation getOpenGLMatrix:matrix];
    matrix[12] = _translation.x;
    matrix[13] = _translation.y;
    matrix[14] = _translation.z;
}

- (RCPoint *) transformPoint:(RCPoint *)point
{
    return [_translation transformPoint:[_rotation transformPoint:point]];
}

- (RCTransformation *) getInverse
{
    RCRotation *rot = [_rotation getInverse];
    RCTranslation *trans = [rot transformTranslation:[_translation getInverse]];
    return [[RCTransformation alloc] initWithTranslation:trans withRotation:rot];
}

- (RCTransformation *) flipAxis:(int)axis
{
    return [[RCTransformation alloc] initWithTranslation:[self.translation flipAxis:axis] withRotation:[self.rotation flipAxis:axis]];
}

- (RCTransformation *) transformCoordinates:(RCTransformation *)transformBy
{
    return [transformBy composeWithTransformation:[self composeWithTransformation:[transformBy getInverse]]];
}

- (RCTransformation *) composeWithTransformation:(RCTransformation *)other
{
    RCRotation *rot = [_rotation composeWithRotation:other.rotation];
    RCTranslation *trans = [_translation composeWithTranslation:[_rotation transformTranslation:other.translation]];
    return [[RCTransformation alloc] initWithTranslation:trans withRotation:rot];    
}

- (NSDictionary *) dictionaryRepresentation
{
    return @{ @"rotation": [self.rotation dictionaryRepresentation], @"translation": [self.translation dictionaryRepresentation] };
}

@end
