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
    void (^authorizationHandler)(BOOL granted);
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
        authorizationHandler = nil;
        _sysLocationMan = [[CLLocationManager alloc] init];
        _sysLocationMan.desiredAccuracy = kCLLocationAccuracyBest;
        _sysLocationMan.distanceFilter = 500;
        _sysLocationMan.headingFilter = 1;
        _sysLocationMan.delegate = self;
    }
    
    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void) requestLocationAccessWithCompletion:(void (^)(BOOL granted))handler
{
    if(![CLLocationManager locationServicesEnabled])
    {
        authorizationHandler = handler;
        //If location is turned off globally, this will pop up the dialog that sends the user to preferences. Explicitly asking permission does not do this.
        [_sysLocationMan startUpdatingLocation];
    }
    else if([self isLocationDisallowed])
    {
        //We already tried to get permission and failed
        handler(false);
    }
    else
    {
        if([CLLocationManager authorizationStatus] == kCLAuthorizationStatusNotDetermined)
        {
            //ask for approval
            authorizationHandler = handler;
            if([_sysLocationMan respondsToSelector:@selector(requestWhenInUseAuthorization)])
                [_sysLocationMan requestWhenInUseAuthorization]; // NSLocationWhenInUseUsageDescription key in Info.plist required when calling this method.
            else
                [_sysLocationMan startUpdatingLocation];
        }
        else
        {
            //not disallowed or undetermined, so allowed
            handler(true);
        }
    }
}

- (void)startLocationUpdates
{
    LOGME
    if (isUpdating || [self isLocationDisallowed]) return;
    isUpdating = YES;
    [_sysLocationMan startUpdatingLocation]; // if location permission dialog doesn't appear, make sure you have the NSLocationWhenInUseUsageDescription in the plist for iOS >= 8
}

- (void)stopLocationUpdates
{
    LOGME
    if (isUpdating) [_sysLocationMan stopUpdatingLocation];
    isUpdating = NO;
}

- (void) startHeadingUpdates
{
    LOGME
    if ([CLLocationManager headingAvailable])
    {
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

#ifndef __IPHONE_8_0
#define kCLAuthorizationStatusAuthorizedAlways kCLAuthorizationStatusAuthorized
#endif

- (BOOL) isLocationDisallowed
{
    if(![CLLocationManager locationServicesEnabled]) return true;
    CLAuthorizationStatus status = [CLLocationManager authorizationStatus];
    switch(status)
    {
        case kCLAuthorizationStatusNotDetermined:
        case kCLAuthorizationStatusAuthorizedAlways: //This is the same as kCLAuthorizationStatusAuthorized from iOS < 8
#ifdef __IPHONE_8_0
        case kCLAuthorizationStatusAuthorizedWhenInUse:
#endif
            return false;
        case kCLAuthorizationStatusDenied:
        case kCLAuthorizationStatusRestricted:
            DLog(@"Location permission explictly denied or restricted");
            return true;
    }
}

- (BOOL) isLocationExplicitlyAllowed
{
    if(![CLLocationManager locationServicesEnabled]) return false;
    CLAuthorizationStatus status = [CLLocationManager authorizationStatus];
    switch(status)
    {
        case kCLAuthorizationStatusAuthorizedAlways: //This is the same as kCLAuthorizationStatusAuthorized from iOS < 8
#ifdef __IPHONE_8_0
        case kCLAuthorizationStatusAuthorizedWhenInUse:
#endif
            return true;
        case kCLAuthorizationStatusDenied:
        case kCLAuthorizationStatusRestricted:
            DLog(@"Location permission explictly denied or restricted");
        case kCLAuthorizationStatusNotDetermined:
            return false;
    }
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

- (void) locationManager:(CLLocationManager *)manager didChangeAuthorizationStatus:(CLAuthorizationStatus)status
{
    if(authorizationHandler == nil) return;
    if(status != kCLAuthorizationStatusNotDetermined)
    {
        authorizationHandler([self isLocationExplicitlyAllowed]);
        authorizationHandler = nil;
    }    
}

- (void) locationManager:(CLLocationManager *)manager didFailWithError:(NSError *)error
{
    DLog(@"%@", error);
}

- (void)updateStoredLocation:(CLLocation*)newLocation
{
    _location = newLocation;
    
    //DLog(@"Location: %+.4f, %+.4f, %.0fm", _location.coordinate.latitude, _location.coordinate.longitude, _location.horizontalAccuracy);
    
    NSTimeInterval howRecent = [_location.timestamp timeIntervalSinceNow];
    
    // If it's a relatively recent event, turn off updates to save power
    if (fabs(howRecent) < 15.0) {
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
