//
//  CLPlacemark+RCPlacemark.m
//  RCCore
//
//  Created by Ben Hirashima on 1/29/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "CLPlacemark+RCPlacemark.h"

@implementation CLPlacemark (RCPlacemark)

- (NSString*)getFormattedAddress
{
    return [self getFormattedAddress:self];
}

- (NSString*)getFormattedAddress:(CLPlacemark*)place
{
    NSString *result = @"";
    
    if ([place administrativeArea]) result = [place administrativeArea];
    if ([place locality]) result = [NSString stringWithFormat:@"%@, %@", [place locality], result];
    if ([place thoroughfare]) result = [NSString stringWithFormat:@"%@, %@", [place thoroughfare], result];
    if ([place subThoroughfare]) result = [NSString stringWithFormat:@"%@ %@", [place subThoroughfare], result];
    
    return result;
}

@end
