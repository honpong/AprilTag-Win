//
//  TMLocation+TMLocationExt.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/1/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMLocation+TMLocationExt.h"
#import "TMConstants.h"

@implementation TMLocation (TMLocationExt)

+ (TMLocation*) getLocationNear:(CLLocation*)clLocation
{
    NSArray *locations = [DATA_MANAGER queryObjectsOfType:[TMLocation getEntity] withPredicate:nil];
    CLLocationDistance leastDist = 1000000000; //meters
    TMLocation *closestLoc;
    
    //get the closest location in the db
    for (TMLocation *thisLoc in locations)
    {
        CLLocationDistance thisDist = [thisLoc getDistanceFrom:clLocation];
        if (thisDist < leastDist)
        {
            closestLoc = thisLoc;
            leastDist = thisDist;
        }
    }
    
    //determine if it's close enough to be a match
    if (leastDist <= 65)
    {
        return closestLoc;
    }
    else
    {
        return nil;
    }
}

- (CLLocationDistance) getDistanceFrom:(CLLocation*)location
{
    CLLocation *thisLoc = [[CLLocation alloc] initWithLatitude:self.latititude longitude:self.longitude];
    return [location distanceFromLocation:thisLoc];
}

+ (TMLocation*) getNewLocation
{
    return (TMLocation*)[DATA_MANAGER getNewObjectOfType:[self getEntity]];
}

+ (TMLocation*) getLocationById:(int)dbid
{
    return (TMLocation*)[DATA_MANAGER getObjectOfType:[self getEntity] byDbid:dbid];
}

@end
