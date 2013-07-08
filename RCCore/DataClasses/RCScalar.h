//
//  RCScalar.h
//  RCCore
//
//  Created by Eagle Jones on 6/27/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

/** Represents a scalar value and it's standard deviation. */
@interface RCScalar : NSObject

@property (nonatomic, readonly) float scalar;
@property (nonatomic, readonly) float standardDeviation;

/** You will not typically need to instantiate this class yourself. */
- (id) initWithScalar:(float)scalar withStdDev:(float)stdDev;
- (NSDictionary*) dictionaryRepresenation;

@end
