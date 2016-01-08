//
//  AppDelegate.m
//  RCCapture
//
//  Created by Brian on 9/27/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import "AppDelegate.h"

#define PREF_DEVICE_PARAMS @"DeviceCalibration"
#define PREF_IS_CALIBRATED @"PREF_IS_CALIBRATED"

#import "ReplayViewController.h"

@implementation AppDelegate
{
    UIViewController * mainView;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    NSDictionary *appDefaults = [NSDictionary dictionaryWithObjectsAndKeys:
                                 [NSNumber numberWithBool:NO], PREF_IS_CALIBRATED,
                                 nil];
    [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];

    mainView = self.window.rootViewController;

    // determine if calibration has been done
    BOOL isCalibrated = [[NSUserDefaults standardUserDefaults] boolForKey:PREF_IS_CALIBRATED]; // gets set to YES when calibration completes
    BOOL hasStoredCalibrationData = [[RCSensorFusion sharedInstance] hasCalibrationData]; // checks if calibration data can be retrieved

    if (!isCalibrated || !hasStoredCalibrationData)
    {
        if (!hasStoredCalibrationData) [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_IS_CALIBRATED]; // ensures calibration is not marked as finished until it's completely finished
        [self startFromCalibration];
    }

    return YES;
}

- (void) startFromHome
{
    self.window.rootViewController = mainView;
}

- (void) startFromReplay
{
    UIStoryboard * mainStoryBoard = [UIStoryboard storyboardWithName:@"Main" bundle:nil];
    ReplayViewController * rc = (ReplayViewController *)[mainStoryBoard instantiateViewControllerWithIdentifier:@"ReplayView"];
    self.window.rootViewController = rc;
}

- (void) startFromCalibration
{
    UIStoryboard* calStoryboard = [UIStoryboard storyboardWithName:@"Calibration" bundle:[NSBundle mainBundle]];
    RCCalibration1 * vc = [calStoryboard instantiateViewControllerWithIdentifier:@"Calibration1"];
    vc.calibrationDelegate = self;
    vc.modalPresentationStyle = UIModalPresentationFullScreen;
    self.window.rootViewController = vc;
}

- (void) startFromCapture
{
    UIStoryboard * mainStoryBoard = [UIStoryboard storyboardWithName:@"Main" bundle:nil];
    UIViewController * vc = [mainStoryBoard instantiateViewControllerWithIdentifier:@"CaptureView"];
    self.window.rootViewController = vc;
}

- (void) startFromLive
{
    UIStoryboard * mainStoryBoard = [UIStoryboard storyboardWithName:@"Main" bundle:nil];
    UIViewController * rc = [mainStoryBoard instantiateViewControllerWithIdentifier:@"LiveView"];
    self.window.rootViewController = rc;
}

+ (NSURL *) timeStampedURLWithSuffix:(NSString *)suffix
{
    NSURL * documentURL = [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] lastObject];

    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setDateFormat:@"yyyy-MM-dd_HH-mm-ss"];

    NSDate *date = [NSDate date];
    NSString * formattedDateString = [dateFormatter stringFromDate:date];
    NSMutableString * filename = [[NSMutableString alloc] initWithString:formattedDateString];
    [filename appendString:suffix];
    NSURL *fileUrl = [documentURL URLByAppendingPathComponent:filename];

    return fileUrl;
}

#pragma mark - RCCalibrationDelegate


- (void)startMotionSensors
{
    [[RCMotionManager sharedInstance] startMotionCapture];
}

- (void)stopMotionSensors
{
    [[RCMotionManager sharedInstance] stopMotionCapture];
}

- (void) calibrationDidFinish:(UIViewController*)lastViewController
{
    // Save calibration data
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:PREF_IS_CALIBRATED]; // set a flag to indicate calibration completed

    NSString * vendorId = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
    NSDictionary * calibrationData = [[NSUserDefaults standardUserDefaults] objectForKey:PREF_DEVICE_PARAMS];
    NSMutableDictionary * dict = [calibrationData mutableCopy];
    [dict setObject:vendorId forKey:@"id"];
    
    NSError * error;
    NSData * jsonData = [NSJSONSerialization dataWithJSONObject:dict
                                                        options:NSJSONWritingPrettyPrinted
                                                          error:&error];
    if (!jsonData) {
        NSLog(@"Error serializing calibration data: %@", error);
    }
    
    NSURL * calibrationUrl = [AppDelegate timeStampedURLWithSuffix:@"-calibration.json"];
    BOOL success = [[NSFileManager defaultManager] createFileAtPath:calibrationUrl.path contents:jsonData attributes:nil];
    if(!success)
        NSLog(@"Error writing to path %@", calibrationUrl.path);
    
    [self startFromHome];
}

#pragma mark -

@end
