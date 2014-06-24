//
//  RCGeocoder.m
//  RCCore
//
//  Created by Eagle Jones on 6/14/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCGeocoder.h"
#import "CLPlacemark+RCPlacemark.h"

@implementation RCGeocoder

+ (void) reverseGeocodeLocation:(CLLocation *)location withCompletionBlock:(void (^)(NSString * address, NSError *error))completionBlock;
{
    if (location == nil) completionBlock(nil, [NSError errorWithDomain:@"com.realitycap.geocoder" code:1 userInfo:nil]);
    
    CLGeocoder *geocoder = [[CLGeocoder alloc] init];
    
    [geocoder reverseGeocodeLocation:location completionHandler:
     ^(NSArray *placemarks, NSError *error)
     {
         if (error)
         {
             DLog(@"Geocode failed with error: %@", error);
             completionBlock(nil, error);
             return;
         }
         
         if(placemarks && placemarks.count > 0)
         {
             CLPlacemark *topResult = placemarks[0];
             NSString *address = [topResult getFormattedAddress];
             completionBlock(address, nil);
         }
     }
     ];
}

@end
