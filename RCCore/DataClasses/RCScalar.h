//
//  RCScalar.h
//  RCCore
//
//  Created by Eagle Jones on 6/27/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface RCScalar : NSObject

@property (nonatomic, readonly) float scalar;
@property (nonatomic, readonly) float standardDeviation;

- (id) initWithScalar:(float)scalar withStdDev:(float)stdDev;

@end
