//
//  TMLocation+TMLocationExt.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/1/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMLocation+TMLocationExt.h"

@implementation TMLocation (TMLocationExt)

#pragma mark - Database operations

+ (TMLocation*)getNewLocation
{
    return (TMLocation*)[DATA_MANAGER getNewObjectOfType:[self getEntity]];
}

+ (TMLocation*)getLocationById:(int)dbid
{
    return (TMLocation*)[DATA_MANAGER getObjectOfType:[self getEntity] byDbid:dbid];
}

@end
