//
//  TMScalar.h
//  TrueMeasureSDK
//
//  Created by Ben Hirashima on 10/22/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

/** Represents a scalar value and its standard deviation. */
@interface TMScalar : NSObject <NSSecureCoding>

@property (nonatomic, readonly) float scalar;
@property (nonatomic, readonly) float standardDeviation;

- (id) initWithScalar:(float)scalar withStdDev:(float)stdDev;

@end
