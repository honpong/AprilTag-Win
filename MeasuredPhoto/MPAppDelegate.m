//
//  MPAppDelegate.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/26/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPAppDelegate.h"

@implementation MPAppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
       // Register the preference defaults early.
       NSDictionary *appDefaults = [NSDictionary dictionaryWithObjectsAndKeys:
                                    [NSNumber numberWithInt:UnitsImperial], PREF_UNITS,
                                    [NSNumber numberWithBool:YES], PREF_ADD_LOCATION,
                                    [NSNumber numberWithBool:YES], PREF_SHOW_LOCATION_EXPLANATION,
                                    [NSNumber numberWithInt:0], PREF_LAST_TRANS_ID,
                                    nil];
       
       [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];
   });
    
    return YES;
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    LOGME
    [[NSUserDefaults standardUserDefaults] synchronize];
    
    if ([LOCATION_MANAGER isLocationAuthorized])
    {
        // location already authorized. go ahead.
        LOCATION_MANAGER.delegate = self;
        [LOCATION_MANAGER startLocationUpdates];
    }
    else if([self shouldShowLocationExplanation])
    {
        // show explanation, then ask for authorization. if they authorize, then start updating location.
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Location"
                                                        message:@"If you allow the app to use your location, we can improve the accuracy of your measurements by adjusting for altitude and how far you are from the equator. If you don't want us to save your location data after the measurement, you can turn that off in the settings."
                                                       delegate:self
                                              cancelButtonTitle:@"Continue"
                                              otherButtonTitles:nil];
        [alert show];
    }
    else
    {
        // location denied. continue without it.
        [self startMotionOnlySensorFusion];
    }
}
							
- (void)applicationWillResignActive:(UIApplication *)application
{
    LOGME
    if (MOTION_MANAGER.isCapturing) [MOTION_MANAGER stopMotionCapture];
    if (SENSOR_FUSION.isSensorFusionRunning) [SENSOR_FUSION stopSensorFusion];
}

- (BOOL)shouldShowLocationExplanation
{
    if ([CLLocationManager locationServicesEnabled])
    {
        return [CLLocationManager authorizationStatus] == kCLAuthorizationStatusNotDetermined;
    }
    else
    {
        return [[NSUserDefaults standardUserDefaults] boolForKey:PREF_SHOW_LOCATION_EXPLANATION];
    }
}

- (void) alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    if (buttonIndex == 0) //the only button
    {
        [[NSUserDefaults standardUserDefaults] setBool:NO forKey:PREF_SHOW_LOCATION_EXPLANATION];
        [[NSUserDefaults standardUserDefaults] synchronize];
        
        if([LOCATION_MANAGER shouldAttemptLocationAuthorization])
        {
            LOCATION_MANAGER.delegate = self;
            [LOCATION_MANAGER startLocationUpdates]; // will show dialog asking user to authorize location
        }
    }
}

- (void) startMotionOnlySensorFusion
{
    LOGME
    [SENSOR_FUSION startSensorFusionWithLocation:[LOCATION_MANAGER getStoredLocation] withStaticCalibration:false];
    [MOTION_MANAGER startMotionCapture];
}

- (void) locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray *)locations
{
    LOCATION_MANAGER.delegate = nil;
    [self startMotionOnlySensorFusion];
}

@end
