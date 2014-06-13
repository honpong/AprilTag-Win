//
//  RCLocationManager.h
//
//  Created by Ben Hirashima on 1/18/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <CoreLocation/CoreLocation.h>
#import "CLPlacemark+RCPlacemark.h"

/** Simply gets the current location, and makes it available via getStoredLocation.
 
 */
@interface RCLocationManager : NSObject <CLLocationManagerDelegate>

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

+ (RCLocationManager *) sharedInstance;

@end


