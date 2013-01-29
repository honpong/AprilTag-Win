//
//  TMLocationManagerFactory.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/18/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCLocationManagerFactory.h"
#import "CLPlacemark+RCPlacemark.h"

@interface RCLocationManagerImpl : NSObject <RCLocationManager>
{
    CLLocationManager *_sysLocationMan;
    CLLocation *_location;
    NSString *_address;
}

@end

@implementation RCLocationManagerImpl

- (id)init
{
    self = [super init];
    
    if (self)
    {
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handlePause)
                                                     name:UIApplicationWillResignActiveNotification
                                                   object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handleTerminate)
                                                     name:UIApplicationWillTerminateNotification
                                                   object:nil];       
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
    NSLog(@"startLocationUpdates");
    
    _sysLocationMan = locMan;
    _sysLocationMan.desiredAccuracy = kCLLocationAccuracyBest;
    _sysLocationMan.distanceFilter = 500;
    _sysLocationMan.delegate = (id<CLLocationManagerDelegate>)[RCLocationManagerFactory getLocationManagerInstance];
            
    [_sysLocationMan startUpdatingLocation];
}

- (void)stopLocationUpdates
{
    NSLog(@"stopLocationUpdates");
    [_sysLocationMan stopUpdatingLocation];
    _sysLocationMan = nil;
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

//for testing. you can set this factory to return a mock object.
+ (void)setLocationManagerInstance:(id<RCLocationManager>)mockObject
{
    instance = mockObject;
}

@end
