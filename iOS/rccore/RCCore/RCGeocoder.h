//
//  RCLocationManager+RCGeocoder.h
//  RCCore
//
//  Created by Eagle Jones on 6/14/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

@interface RCGeocoder : NSObject

/** Asynchronously sends reverse geocode location request */
+ (void) reverseGeocodeLocation:(CLLocation *)location withCompletionBlock:(void (^)(NSString * address, NSError *error))completionBlock;

@end
