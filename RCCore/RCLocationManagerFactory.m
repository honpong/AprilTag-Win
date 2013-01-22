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
    CLLocationManager *systemLocationManager;
    CLLocation *location;
    NSString *address;
}

//- (void)locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray *)locations;
//- (void)locationManager:(CLLocationManager *)manager didUpdateToLocation:(CLLocation *)newLocation fromLocation:(CLLocation *)oldLocation;

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
    NSLog(@"startLocationUpdates");
    
    systemLocationManager = [[CLLocationManager alloc] init];
    systemLocationManager.desiredAccuracy = kCLLocationAccuracyBest;
    systemLocationManager.distanceFilter = 500;
    systemLocationManager.delegate = (id<CLLocationManagerDelegate>)[RCLocationManagerFactory getLocationManagerInstance];
            
    [systemLocationManager startUpdatingLocation];
}

- (void)stopLocationUpdates
{
    NSLog(@"stopLocationUpdates");
    [systemLocationManager stopUpdatingLocation];
    systemLocationManager = nil;
}

/**
 @returns Returns nil if location has not been determined
 */
- (CLLocation*)getStoredLocation
{
    return location;
}

/**
 @returns Returns nil if location has not been determined
 */
- (NSString*)getStoredLocationAddress
{
    return address;
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
    location = newLocation;
    
    NSLog(@"Location: %+.4f, %+.4f, %.0fm", location.coordinate.latitude, location.coordinate.longitude, location.horizontalAccuracy);
    
    NSDate* eventDate = location.timestamp;
    NSTimeInterval howRecent = [eventDate timeIntervalSinceNow];
    
    // If it's a relatively recent event, turn off updates to save power
    if (abs(howRecent) < 15.0) {
        if(location.horizontalAccuracy <= 65)
        {
            [self reverseGeocode];
            [self stopLocationUpdates];
        }
    }
}

- (void)reverseGeocode
{
    CLGeocoder *geocoder = [[CLGeocoder alloc] init];
    
    [geocoder reverseGeocodeLocation:location completionHandler:^(NSArray *placemarks, NSError *error) {
        if (error){
            NSLog(@"Geocode failed with error: %@", error);
            return;
        }
        if(placemarks && placemarks.count > 0)
        {
            //do something
            CLPlacemark *topResult = [placemarks objectAtIndex:0];
            
            address = [RCLocationManagerFactory getFormattedAddress:topResult];
        }
    }];
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

+ (NSString*)getFormattedAddress:(CLPlacemark*)place
{
    NSString *result = @"";
    
    if ([place administrativeArea]) result = [place administrativeArea];
    if ([place locality]) result = [NSString stringWithFormat:@"%@, %@", [place locality], result];
    if ([place thoroughfare]) result = [NSString stringWithFormat:@"%@, %@", [place thoroughfare], result];
    if ([place subThoroughfare]) result = [NSString stringWithFormat:@"%@ %@", [place subThoroughfare], result];
    
    return result;
}
@end
