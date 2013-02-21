//
//  TMLocationManagerFactory.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/18/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCLocationManagerFactory.h"

@interface RCLocationManagerImpl : NSObject <RCLocationManager>
{
    CLLocationManager *_sysLocationMan;
    CLLocation *_location;
    NSString *_address;
    BOOL isUpdating;
}

@end

@implementation RCLocationManagerImpl

- (id)init
{
    self = [super init];
    
    if (self)
    {
//        [[NSNotificationCenter defaultCenter] addObserver:self
//                                                 selector:@selector(handleResume)
//                                                     name:UIApplicationDidBecomeActiveNotification
//                                                   object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handlePause)
                                                     name:UIApplicationDidEnterBackgroundNotification
                                                   object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handleTerminate)
                                                     name:UIApplicationWillTerminateNotification
                                                   object:nil];
        isUpdating = NO;
    }
    
    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

/** @returns YES if location services are enabled and the app is authorized to use location */
- (BOOL)startLocationUpdates
{
    NSLog(@"startLocationUpdates");
    
    if (![self shouldAttemptLocationAuthorization]) return NO;
    
    if (isUpdating) return YES;
    
    if (_sysLocationMan == nil) _sysLocationMan = [[CLLocationManager alloc] init];
    
    _sysLocationMan.desiredAccuracy = kCLLocationAccuracyBest;
    _sysLocationMan.distanceFilter = 500;
    _sysLocationMan.delegate = (id<CLLocationManagerDelegate>)[RCLocationManagerFactory getLocationManagerInstance];
    
    [_sysLocationMan startUpdatingLocation];
    isUpdating = YES;
    return YES;
}

- (void)stopLocationUpdates
{
    NSLog(@"stopLocationUpdates");
    if (isUpdating && _sysLocationMan) [_sysLocationMan stopUpdatingLocation];
    
    //don't release sysLocationMan if we might be waiting for authorization. this would cause the "allow" dialog to disappear
    if ([CLLocationManager authorizationStatus] == kCLAuthorizationStatusAuthorized || ![self shouldAttemptLocationAuthorization]) _sysLocationMan = nil; 
    
    isUpdating = NO;
}

- (BOOL)shouldAttemptLocationAuthorization
{
    if (![CLLocationManager locationServicesEnabled])
    {
        NSLog(@"Location services disabled");
        return NO;
    }
    
    if ([CLLocationManager authorizationStatus] == kCLAuthorizationStatusDenied)
    {
        NSLog(@"Location permission explictly denied");
        return NO;
    }
    
    return YES;
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

/**
 @returns Returns nil if location has not been determined
 */
- (NSString*)getStoredLocationAddress
{
    return _address;
}

//for iOS 6
- (void)locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray *)locations
{
    [self updateStoredLocation:locations.lastObject];
}

//for iOS 5
- (void)locationManager:(CLLocationManager *)manager didUpdateToLocation:(CLLocation *)newLocation fromLocation:(CLLocation *)oldLocation
{
    [self updateStoredLocation:newLocation];
}

- (void)updateStoredLocation:(CLLocation*)newLocation
{
    _location = newLocation;
    
    NSLog(@"Location: %+.4f, %+.4f, %.0fm", _location.coordinate.latitude, _location.coordinate.longitude, _location.horizontalAccuracy);
    
    NSDate* eventDate = _location.timestamp;
    NSTimeInterval howRecent = [eventDate timeIntervalSinceNow];
    
    // If it's a relatively recent event, turn off updates to save power
    if (abs(howRecent) < 15.0) {
        if(_location.horizontalAccuracy <= 65)
        {
            [self reverseGeocode];
            [self stopLocationUpdates];
        }
    }
}

- (void)reverseGeocode
{
    CLGeocoder *geocoder = [[CLGeocoder alloc] init];
    
    [geocoder reverseGeocodeLocation:_location completionHandler:
        ^(NSArray *placemarks, NSError *error)
        {
            if (error)
            {
                NSLog(@"Geocode failed with error: %@", error);
                return;
            }
            
            if(placemarks && placemarks.count > 0)
            {
                //do something
                CLPlacemark *topResult = [placemarks objectAtIndex:0];
                
                _address = [topResult getFormattedAddress];
            }
        }
    ];
}

//- (void)handleResume
//{
//    [self startLocationUpdates]; //doesn't work because must be called on UI thread
//}

- (void)handlePause
{
    [self stopLocationUpdates];
}

- (void)handleTerminate
{
    [self stopLocationUpdates];
}

@end

@implementation RCLocationManagerFactory

static id<RCLocationManager> instance;

+ (id<RCLocationManager>)getLocationManagerInstance
{
    if (instance == nil)
    {
        instance = [[RCLocationManagerImpl alloc] init];
    }
    
    return instance;
}

/** for testing. you can set this factory to return a mock object. */
+ (void)setLocationManagerInstance:(id<RCLocationManager>)mockObject
{
    instance = mockObject;
}

@end
