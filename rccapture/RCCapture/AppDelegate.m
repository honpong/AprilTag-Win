//
//  AppDelegate.m
//  RCCapture
//
//  Created by Brian on 9/27/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import "AppDelegate.h"

#define PREF_DEVICE_PARAMS @"DeviceCalibration"

#import "ReplayViewController.h"

@implementation AppDelegate
{
    UIViewController * mainView;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    mainView = self.window.rootViewController;
    [self startFromHome];
    return YES;
}

- (void) startFromHome
{
    self.window.rootViewController = mainView;
}

- (void) startFromReplay
{
    UIStoryboard * mainStoryBoard = [UIStoryboard storyboardWithName:@"Main_iPhone" bundle:nil];
    ReplayViewController * rc = (ReplayViewController *)[mainStoryBoard instantiateViewControllerWithIdentifier:@"ReplayView"];
    self.window.rootViewController = rc;
}

- (void) startFromCalibration
{
    NSLog(@"Removing calibration data");
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:PREF_DEVICE_PARAMS];

    UIViewController * vc = [CalibrationStep1 instantiateViewControllerWithDelegate:self];
    self.window.rootViewController = vc;
}

- (void) startFromCapture
{
    UIStoryboard * mainStoryBoard = [UIStoryboard storyboardWithName:@"Main_iPhone" bundle:nil];
    ReplayViewController * rc = (ReplayViewController *)[mainStoryBoard instantiateViewControllerWithIdentifier:@"CaptureView"];
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

- (void)calibrationDidFinish
{
    // Save calibration data

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

- (void)applicationDidBecomeActive:(UIApplication *)application
{
}

- (void)applicationWillResignActive:(UIApplication *)application
{
}

@end
