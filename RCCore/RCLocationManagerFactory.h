//
//  TMLocationManagerFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/18/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreLocation/CoreLocation.h>
#import "CLPlacemark+RCPlacemark.h"

@protocol RCLocationManager <CLLocationManagerDelegate>

- (void)startLocationUpdates;
- (void)startLocationUpdates:(CLLocationManager*)locMan;
- (void)stopLocationUpdates;
- (CLLocation*)getStoredLocation;
- (NSString*)getStoredLocationAddress;
- (BOOL)isUpdatingLocation;
- (BOOL) shouldAttemptLocationAuthorization;
- (BOOL) isLocationAuthorized;

@end

@interface RCLocationManagerFactory : NSObject 

+ (id<RCLocationManager>)getLocationManagerInstance;
+ (void)setLocationManagerInstance:(id<RCLocationManager>)mockObject;

@end


