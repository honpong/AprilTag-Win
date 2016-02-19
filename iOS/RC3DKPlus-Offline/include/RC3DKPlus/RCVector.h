//
//  RCVector.h
//  RCCore
//
//  Created by Eagle Jones on 6/27/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Accelerate/Accelerate.h>

/** Represents a 3 or 4 dimensional vector and its standard deviation.
 
 This wraps a vFloat vector - see the Accelerate framework documentation for more information.
 */
@interface RCVector : NSObject

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
/** The fourth element of the vector. */
@property (nonatomic, readonly) float v0;
/** The fourth element of the vector. */
@property (nonatomic, readonly) float v1;
/** The fourth element of the vector. */
@property (nonatomic, readonly) float v2;
/** The fourth element of the vector. */
@property (nonatomic, readonly) float v3;
/** The first element of the standard deviation. */
@property (nonatomic, readonly) float stdx;
/** The second element of the standard deviation. */
@property (nonatomic, readonly) float stdy;
/** The third element of the standard deviation. */
@property (nonatomic, readonly) float stdz;
/** The fourth element of the standard deviation. */
@property (nonatomic, readonly) float std0;
/** The fourth element of the standard deviation. */
@property (nonatomic, readonly) float std1;
/** The fourth element of the standard deviation. */
@property (nonatomic, readonly) float std2;
/** The fourth element of the standard deviation. */
@property (nonatomic, readonly) float std3;

/** Instantiate an RCVector with the given vector and standard deviation. */
- (id) initWithVector:(vFloat)vector withStandardDeviation:(vFloat)standardDeviation;

/** Instantiate an RCVector with the given values for the first three elements of the vector and standard deviation. */
- (id) initWithX:(float)x withStdX:(float)stdx withY:(float)y withStdY:(float)stdy withZ:(float)z withStdZ:(float)stdz;

/** Instantiate an RCVector with the given values for the first three elements of the vector. */
- (id) initWithX:(float)x withY:(float)y withZ:(float)z;

/** Return a dictionary representation of the vector. */
- (NSDictionary*) dictionaryRepresentation;

@end