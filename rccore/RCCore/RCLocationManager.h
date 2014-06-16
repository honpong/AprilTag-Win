//
//  TMLocationManagerFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/18/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <CoreLocation/CoreLocation.h>

@interface RCLocationManager : NSObject <CLLocationManagerDelegate>

- (void) startLocationUpdates;
- (void) stopLocationUpdates;
- (CLLocation*) getStoredLocation;
- (BOOL) isUpdatingLocation;
- (BOOL) shouldAttemptLocationAuthorization;
- (BOOL) isLocationAuthorized;
- (void) startHeadingUpdates;
- (void) stopHeadingUpdates;

#ifdef DEBUG
- (void)startLocationUpdates:(CLLocationManager*)locMan;
#endif

@property (weak, nonatomic) id<CLLocationManagerDelegate> delegate;

+ (RCLocationManager *) sharedInstance;

@end


