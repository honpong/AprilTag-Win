//
//  LocationManager.h
//
//  Created by Ben Hirashima on 1/18/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <CoreLocation/CoreLocation.h>
#import <RC3DK/RC3DK.h>

/** Simply gets the current location, and makes it available via getStoredLocation.
 
 This class is identical to RCLocationManager, included in the 3DK framework. 
 */
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

@property (weak, nonatomic) id<CLLocationManagerDelegate> delegate;

+ (LocationManager *) sharedInstance;

@end


