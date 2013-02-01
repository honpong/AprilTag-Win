//
//  TMMeasurement+TMMeasurementExt.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMMeasurement.h"
#import "RCCore/RCDistanceFormatter.h"

@interface TMMeasurement (TMMeasurementExt)

- (NSString*)getFormattedDistance:(float)meters;

@end
