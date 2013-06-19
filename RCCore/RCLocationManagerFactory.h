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
- (void)stopLocationUpdates;
- (CLLocation*)getStoredLocation;
- (NSString*)getStoredLocationAddress;
- (BOOL)isUpdatingLocation;
- (BOOL) shouldAttemptLocationAuthorization;
- (BOOL) isLocationAuthorized;
- (void) startHeadingUpdates;
- (void) stopHeadingUpdates;

#ifdef DEBUG
- (void)startLocationUpdates:(CLLocationManager*)locMan;
#endif

@property (weak, nonatomic) id<CLLocationManagerDelegate> delegate;

@end

@interface RCLocationManagerFactory : NSObject 

+ (id<RCLocationManager>) getInstance;

#ifdef DEBUG
+ (void) setInstance:(id<RCLocationManager>)mockObject;
#endif

@end


