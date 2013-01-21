//
//  TMLocationManagerFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/18/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <CoreLocation/CoreLocation.h>

@protocol TMLocationManager <CLLocationManagerDelegate>

- (void)startLocationUpdates;
- (void)stopLocationUpdates;
- (CLLocation*)getStoredLocation;
- (NSString*)getStoredLocationAddress;

@end

@interface TMLocationManagerFactory

+ (id<TMLocationManager>)getLocationManagerInstance;
+ (void)setLocationManagerInstance:(id<TMLocationManager>)mockObject;
+ (NSString*)getFormattedAddress:(CLPlacemark*)place;

@end


