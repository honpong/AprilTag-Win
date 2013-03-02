//
//  TMLocation+TMLocationSync.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/1/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMLocation+TMLocationSync.h"

@implementation TMLocation (TMLocationSync)

+ (NSString*)getHttpGetPath
{
    return @"api/locations/";
}
+ (NSString*)getHttpPostPath
{
    return [self getHttpGetPath];
}
+ (NSString*)getHttpPutPath
{
    return @"api/location/%i/";
}

@end
