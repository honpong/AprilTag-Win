//
//  TMLocation+TMLocationExt.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/1/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMLocation.h"
#import "TMDataManagerFactory.h"
#import <CoreLocation/CoreLocation.h>

@interface TMLocation (TMLocationExt)

+ (TMLocation*)getNewLocation;
+ (TMLocation*)getLocationById:(int)dbid;
+ (TMLocation*)getLocationNear:(CLLocation*)clLocation;

@end
