//
//  TMPoint.h
//  TrueMeasureSDK
//
//  Created by Ben Hirashima on 10/22/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <Accelerate/Accelerate.h>

/** Represents a 3D point in space and its standard deviation. */
@interface TMPoint : NSObject <NSSecureCoding>

/** The underlying representation of the vector. */
@property (nonatomic, readonly) vFloat vector;

/** The underlying representation of the standard deviation. */
@property (nonatomic, readonly) vFloat standardDeviation;

/** The first element of the vector. */
@property (nonatomic, readonly) float x;
/** The second element of the vector. */
@property (nonatomic, readonly) float y;
/** The third element of the vector. */
@property (nonatomic, readonly) float z;

/** The first element of the standard deviation. */
@property (nonatomic, readonly) float stdx;
/** The second element of the standard deviation. */
@property (nonatomic, readonly) float stdy;
/** The third element of the standard deviation. */
@property (nonatomic, readonly) float stdz;

/** Init with the given vector and standard deviation. */
- (id) initWithVector:(vFloat)vector withStandardDeviation:(vFloat)standardDeviation;

/** Init with the given values for the three elements of the vector and standard deviation. */
- (id) initWithX:(float)x withStdX:(float)stdx withY:(float)y withStdY:(float)stdy withZ:(float)z withStdZ:(float)stdz;

/** Init with the given values for the three elements of the vector. */
- (id) initWithX:(float)x withY:(float)y withZ:(float)z;

@end
