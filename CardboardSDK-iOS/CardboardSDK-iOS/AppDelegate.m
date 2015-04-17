//
//  AppDelegate.m
//  CardboardSDK-iOS
//
//

#import "AppDelegate.h"

#import "CardboardSDK/CardboardSDK.h"
#import "TreasureViewController.h"

#import "RC3DK.h"

//#import "CardboardUnity.h"
//void _unity_getFrameParameters(float *frameParameters);


@interface AppDelegate ()

@end

#define PREF_IS_CALIBRATED @"PREF_IS_CALIBRATED"
#define PREF_SHOW_LOCATION_EXPLANATION @"RC_SHOW_LOCATION_EXPLANATION"

@implementation AppDelegate
{
    TreasureViewController *cardboardViewController;
    RCLocationManager * locationManager;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    self.window.backgroundColor = [UIColor whiteColor];
    [self.window makeKeyAndVisible];

    // Google's treasure example
    cardboardViewController = [TreasureViewController new];
    
//    float *frameParameters = calloc(80, sizeof(float));
//    _unity_getFrameParameters(frameParameters);
    
    // set defaults for some prefs
    NSDictionary *appDefaults = [NSDictionary dictionaryWithObjectsAndKeys:
                                 [NSNumber numberWithBool:NO], PREF_IS_CALIBRATED,
                                 [NSNumber numberWithBool:YES], PREF_SHOW_LOCATION_EXPLANATION,
                                 nil];
    [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];
    
    locationManager = [RCLocationManager sharedInstance];
    
    // determine if calibration has been done
    BOOL isCalibrated = [[NSUserDefaults standardUserDefaults] boolForKey:PREF_IS_CALIBRATED]; // gets set to YES when calibration completes
    BOOL hasStoredCalibrationData = [[RCSensorFusion sharedInstance] hasCalibrationData]; // checks if calibration data can be retrieved
    
    // if calibration hasn't been done, or can't be retrieved, start calibration
    if (!isCalibrated || !hasStoredCalibrationData)
    {
        if (!hasStoredCalibrationData) [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_IS_CALIBRATED]; // ensures calibration is not marked as finished until it's completely finished
        [self gotoCalibration];
    }
    else
    {
        [self gotoMainViewController];
    }
    
    return YES;
}

- (void) gotoMainViewController
{
    self.window.rootViewController = cardboardViewController;
}


- (void) gotoCalibration
{
    UIStoryboard* calStoryboard = [UIStoryboard storyboardWithName:@"Calibration" bundle:[NSBundle mainBundle]];
    RCCalibration1* vc = [calStoryboard instantiateViewControllerWithIdentifier:@"Calibration1"];
    vc.calibrationDelegate = self;
    vc.modalPresentationStyle = UIModalPresentationFullScreen;
    self.window.rootViewController = vc;
}

#pragma mark RCCalibrationDelegate methods

- (void)startMotionSensors
{
    [[RCSensorManager sharedInstance] startMotionSensors];
}

- (void)stopMotionSensors
{
    [[RCSensorManager sharedInstance] stopAllSensors];
}

- (void) calibrationDidFinish:(UIViewController*)lastViewController
{
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:PREF_IS_CALIBRATED]; // set a flag to indicate calibration completed
    [self gotoMainViewController];
}

@end
