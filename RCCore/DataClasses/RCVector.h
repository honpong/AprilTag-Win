//
//  RCVector.h
//  RCCore
//
//  Created by Eagle Jones on 6/27/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Accelerate/Accelerate.h>

@interface RCVector : NSObject

@property (nonatomic, readonly) vFloat vector;
@property (nonatomic, readonly) vFloat standardDeviation;
@property (nonatomic, readonly) float x, y, z, stdx, stdy, stdz;

- (id) initWithVector:(vFloat)vector withStandardDeviation:(vFloat)standardDeviation;
- (id) initWithX:(float)x withStdX:(float)stdx withY:(float)y withStdY:(float)stdy withZ:(float)z withStdZ:(float)stdz;
- (id) initWithX:(float)x withY:(float)y withZ:(float)z;
- (NSDictionary*) dictionaryRepresenation;

@end