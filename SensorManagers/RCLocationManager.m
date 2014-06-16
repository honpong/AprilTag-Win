//
//  RCLocationManager.m
//
//  Created by Ben Hirashima on 1/18/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RCLocationManager.h"
#import "RCDebugLog.h"

@implementation RCLocationManager
{
    CLLocationManager *_sysLocationMan;
    CLLocation *_location;
    BOOL isUpdating;
    BOOL shouldStopAutomatically;
}

+ (RCLocationManager *) sharedInstance
{
    static RCLocationManager *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[self alloc] init];
    });
    return instance;
}

- (id)init
{
    self = [super init];
    
    if (self)
    {
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handleTerminate)
                                                     name:UIApplicationWillTerminateNotification
                                                   object:nil];
        isUpdating = NO;
        shouldStopAutomatically = YES;
    }
    
    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)startLocationUpdates
{
    [self startLocationUpdates:[[CLLocationManager alloc] init]];
}

- (void)startLocationUpdates:(CLLocationManager*)locMan
{
    LOGME
    
    if (isUpdating) return;

    _sysLocationMan = locMan;
    _sysLocationMan.desiredAccuracy = kCLLocationAccuracyBest;
    _sysLocationMan.distanceFilter = 500;
    _sysLocationMan.headingFilter = 1;
    _sysLocationMan.delegate = (id<CLLocationManagerDelegate>) [RCLocationManager sharedInstance];
    
    [_sysLocationMan startUpdatingLocation];
    isUpdating = YES;
}

- (void)stopLocationUpdates
{
    LOGME
    if (isUpdating) [_sysLocationMan stopUpdatingLocation];
    
    //don't release sysLocationMan if we might be waiting for authorization. this would cause the "allow" dialog to disappear
    if ([CLLocationManager authorizationStatus] == kCLAuthorizationStatusAuthorized || ![self shouldAttemptLocationAuthorization]) _sysLocationMan = nil;
    
    isUpdating = NO;
}

- (void) startHeadingUpdates
{
    LOGME
    if ([CLLocationManager headingAvailable])
    {
        _sysLocationMan.delegate = (id<CLLocationManagerDelegate>) [RCLocationManager sharedInstance];
        [_sysLocationMan startUpdatingHeading];
        shouldStopAutomatically = NO;
    }
    else
    {
        DLog(@"Heading data not available");
    }
}

- (void) stopHeadingUpdates
{
    LOGME
    [_sysLocationMan stopUpdatingHeading];
    shouldStopAutomatically = YES;
}

- (BOOL) shouldAttemptLocationAuthorization
{
    if (![CLLocationManager locationServicesEnabled])
    {
        DLog(@"Location services disabled");
        return NO;
    }
    
    if ([CLLocationManager authorizationStatus] == kCLAuthorizationStatusNotDetermined)
    {
        return YES;
    }
    else
    {
        DLog(@"Location permission explictly denied or restricted");
        return NO;
    }
}

- (BOOL) isLocationAuthorized
{
    return [CLLocationManager locationServicesEnabled] && [CLLocationManager authorizationStatus] == kCLAuthorizationStatusAuthorized;
}

- (BOOL)isUpdatingLocation
{
    return isUpdating;
}

/**
 @returns Returns nil if location has not been determined
 */
- (CLLocation*)getStoredLocation
{
    return _location;
}

- (void)locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray *)locations
{
    [self updateStoredLocation:locations.lastObject];
    if ([self.delegate respondsToSelector:@selector(locationManager:didUpdateLocations:)]) [self.delegate locationManager:manager didUpdateLocations:locations];
}

- (void) locationManager:(CLLocationManager *)manager didUpdateHeading:(CLHeading *)newHeading
{
    if ([self.delegate respondsToSelector:@selector(locationManager:didUpdateHeading:)]) [self.delegate locationManager:manager didUpdateHeading:newHeading];
}

- (void) locationManager:(CLLocationManager *)manager didFailWithError:(NSError *)error
{
    LOGME
}

- (void)updateStoredLocation:(CLLocation*)newLocation
{
    _location = newLocation;
    
    //DLog(@"Location: %+.4f, %+.4f, %.0fm", _location.coordinate.latitude, _location.coordinate.longitude, _location.horizontalAccuracy);
    
    NSTimeInterval howRecent = [_location.timestamp timeIntervalSinceNow];
    
    // If it's a relatively recent event, turn off updates to save power
    if (abs(howRecent) < 15.0) {
        if(_location.horizontalAccuracy <= 65)
        {
            if (shouldStopAutomatically) [self stopLocationUpdates];
        }
    }
}

- (void)handleTerminate
{
    [self stopLocationUpdates];
    [self stopHeadingUpdates];
}

@end
