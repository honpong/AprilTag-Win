//
//  RCDistanceImperial.h
//  RCCore
//
//  Created by Ben Hirashima on 5/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCDistanceMetric.h"

@interface RCDistanceImperial : NSObject <RCDistance>

@property float meters;
@property UnitsScale scale;
@property NSString* unitSymbol;

@end
