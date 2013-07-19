//
//  TMLocationManagerFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/18/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <CoreLocation/CoreLocation.h>
#import <RC3DK/RC3DK.h>

@interface LocationManager : NSObject <CLLocationManagerDelegate>

- (void) startLocationUpdates;
- (void) stopLocationUpdates;
- (CLLocation*) getStoredLocation;
- (NSString*) getStoredLocationAddress;
- (BOOL) isUpdatingLocation;
- (BOOL) shouldAttemptLocationAuthorization;
- (BOOL) isLocationAuthorized;
- (void) startHeadingUpdates;
- (void) stopHeadingUpdates;

#ifdef DEBUG
- (void)startLocationUpdates:(CLLocationManager*)locMan;
#endif

@property (weak, nonatomic) id<CLLocationManagerDelegate> delegate;

+ (LocationManager *) sharedInstance;

@end


