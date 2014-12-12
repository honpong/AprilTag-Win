
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

#import "AppDelegate.h"
#import "CATConstants.h"
#import "RCSensorDelegate.h"
#import "RCLocationManager.h"
#import "RC3DK.h"
#import "RCDebugLog.h"
#import "RCMotionManager.h"
#import "RCAVSessionManager.h"

#if TARGET_IPHONE_SIMULATOR
#define SKIP_CALIBRATION YES // skip calibration when running on emulator because it cannot calibrate
#else
#define SKIP_CALIBRATION NO
#endif

@implementation AppDelegate
{
    UIAlertView *locationAlert;
    UIViewController* mainViewController;
    id<RCSensorDelegate> mySensorDelegate;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
        // Register the preference defaults early.
        NSString* locale = [[NSLocale currentLocale] localeIdentifier];
        Units defaultUnits = [locale isEqualToString:@"en_US"] ? UnitsImperial : UnitsMetric;
        
        if ([NSUserDefaults.standardUserDefaults objectForKey:PREF_UNITS] == nil)
        {
            [NSUserDefaults.standardUserDefaults setInteger:defaultUnits forKey:PREF_UNITS];
            [NSUserDefaults.standardUserDefaults synchronize];
        }
        
        NSDictionary *appDefaults = @{PREF_UNITS: [NSNumber numberWithInt:defaultUnits],
                                      PREF_IS_CALIBRATED: @NO};
        
        [NSUserDefaults.standardUserDefaults registerDefaults:appDefaults];
    });
    
    [SENSOR_FUSION setLicenseKey:RC3DK_LICENSE_KEY];
    mySensorDelegate = [SensorDelegate sharedInstance];
    
    mainViewController = self.window.rootViewController;
        
    BOOL calibratedFlag = [NSUserDefaults.standardUserDefaults boolForKey:PREF_IS_CALIBRATED];
    BOOL hasCalibration = [SENSOR_FUSION hasCalibrationData];
    
    [RCAVSessionManager requestCameraAccessWithCompletion:^(BOOL granted){
        if(!granted)
        {
            dispatch_async(dispatch_get_main_queue(), ^{
                UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"No camera access."
                                                                message:@"This app cannot function without camera access. Turn it on in Settings/Privacy/Camera."
                                                               delegate:nil
                                                      cancelButtonTitle:@"OK"
                                                      otherButtonTitles:nil];
                [alert show];
            });
        }
    }];
    
    if (![LOCATION_MANAGER isLocationDisallowed])
    {
        if ([LOCATION_MANAGER isLocationExplicitlyAllowed])
        {
            [LOCATION_MANAGER startLocationUpdates];
        }
        else
        {
            [LOCATION_MANAGER requestLocationAccessWithCompletion:^(BOOL granted)
             {
                 if(granted)
                 {
                     [LOCATION_MANAGER startLocationUpdates];
                 }
             }];
        }        
    }
    
    if (SKIP_CALIBRATION || (calibratedFlag && hasCalibration) )
    {
        [self gotoCaptureScreen];
    }
    else
    {
        [self gotoCalibration];
    }
    
    return YES;
}

- (void) gotoCaptureScreen
{
    self.window.rootViewController = mainViewController;
}

- (void) gotoCalibration
{
    RCCalibration1 * vc = [RCCalibration1 instantiateViewController];
    vc.calibrationDelegate = self;
    vc.sensorDelegate = mySensorDelegate;
    vc.modalPresentationStyle = UIModalPresentationFullScreen;
    self.window.rootViewController = vc;
}

#pragma mark RCCalibrationDelegate methods

- (void) calibrationDidFinish:(UIViewController*)lastViewController
{
    LOGME
    [NSUserDefaults.standardUserDefaults setBool:YES forKey:PREF_IS_CALIBRATED];
    
    [lastViewController presentViewController:mainViewController animated:YES completion:nil];
}

#pragma mark - RCCalibrationDelegate

- (void) calibrationScreenDidAppear:(NSString *)screenName
{
    // log to analytics if desired
}

- (void) calibrationDidFail:(NSError *)error
{
    DLog(@"Calibration failed: %@", error);
    // TODO: implement
}

#pragma mark -

- (void)applicationWillResignActive:(UIApplication *)application
{
    LOGME
    if (MOTION_MANAGER.isCapturing) [MOTION_MANAGER stopMotionCapture];
}

@end

