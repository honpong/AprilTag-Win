//
//  AppDelegate.m
//  RC3DKSampleApp
//
//  Created by Ben Hirashima on 7/19/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "AppDelegate.h"
#import <RC3DK/RC3DK.h>
#import "ViewController.h"

#define PREF_SHOW_LOCATION_EXPLANATION @"RC_SHOW_LOCATION_EXPLANATION"

@implementation AppDelegate
{
    UIViewController * mainView;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    // Override point for customization after application launch.

    if (![[RCSensorFusion sharedInstance] hasCalibrationData])
    {
        NSLog(@"Starting calibration");
        mainView = self.window.rootViewController;
        UIViewController * vc = [Calibration1 instantiateViewControllerWithDelegate:self];
        self.window.rootViewController = vc;
    }

    return YES;
}

- (void)calibrationDidFinish
{
    NSLog(@"Calibration finished");
    self.window.rootViewController = mainView;

}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later. 
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
    // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}
- (void)applicationWillTerminate:(UIApplication *)application
{
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    LOGME
    [[NSUserDefaults standardUserDefaults] synchronize];

    [self startMotionOnlySensorFusion];

    if ([[LocationManager sharedInstance] isLocationAuthorized])
    {
        // location already authorized. go ahead.
        [self startMotionOnlySensorFusion];
        [LocationManager sharedInstance].delegate = self;
        [[LocationManager sharedInstance] startLocationUpdates];
    }
    else if([self shouldShowLocationExplanation])
    {
        // show explanation, then ask for authorization. if they authorize, then start updating location.
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Location"
                                                        message:@"If you allow the app to use your location, we can improve the accuracy of your measurements by adjusting for altitude and how far you are from the equator."
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
    if ([MotionManager sharedInstance].isCapturing) [[MotionManager sharedInstance] stopMotionCapture];
    if ([RCSensorFusion sharedInstance].isSensorFusionRunning) [[RCSensorFusion sharedInstance] stopSensorFusion];
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

        if([[LocationManager sharedInstance] shouldAttemptLocationAuthorization])
        {
            [LocationManager sharedInstance].delegate = self;
            [[LocationManager sharedInstance] startLocationUpdates]; // will show dialog asking user to authorize location
        }
    }
}

- (void) startMotionOnlySensorFusion
{
    LOGME
    [[RCSensorFusion sharedInstance] startInertialOnlyFusion];
    [[RCSensorFusion sharedInstance] setLocation:[[LocationManager sharedInstance] getStoredLocation]];
    [[MotionManager sharedInstance] startMotionCapture];
}

- (void) locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray *)locations
{
    [LocationManager sharedInstance].delegate = nil;
    if(![[RCSensorFusion sharedInstance] isSensorFusionRunning]) [self startMotionOnlySensorFusion];
    [[RCSensorFusion sharedInstance] setLocation:[[LocationManager sharedInstance] getStoredLocation]];
}

@end
