//
//  RCLocationManager.h
//
//  Created by Ben Hirashima on 1/18/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <CoreLocation/CoreLocation.h>

/** Simply gets the current location, and makes it available via getStoredLocation.
 
 */
@interface RCLocationManager : NSObject <CLLocationManagerDelegate>

- (void) requestLocationAccessWithCompletion:(void (^)(BOOL granted))handler;
- (void) startLocationUpdates;
- (void) stopLocationUpdates;
- (CLLocation*) getStoredLocation;
- (BOOL) isUpdatingLocation;
- (BOOL) isLocationDisallowed;
- (BOOL) isLocationExplicitlyAllowed;
- (void) startHeadingUpdates;
- (void) stopHeadingUpdates;

@property (weak, nonatomic) id<CLLocationManagerDelegate> delegate;

+ (RCLocationManager *) sharedInstance;

@end


