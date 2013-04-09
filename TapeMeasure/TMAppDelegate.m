//
//  TMAppDelegate.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMAppDelegate.h"

@implementation TMAppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_LOW, 0), ^
    {
        // Register the preference defaults early.
        NSDictionary *appDefaults = [NSDictionary dictionaryWithObjectsAndKeys:
                                     [NSNumber numberWithInt:UnitsMetric], PREF_UNITS,
                                     [NSNumber numberWithBool:YES], PREF_ADD_LOCATION,
                                     [NSNumber numberWithBool:YES], PREF_SHOW_LOCATION_EXPLANATION,
                                     [NSNumber numberWithInt:0], PREF_LAST_TRANS_ID,
                                     nil];
        
        [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];
        
        NSSetUncaughtExceptionHandler(&uncaughtExceptionHandler);
        [Flurry startSession:FLURRY_KEY];
    });
    
    return YES;
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
    // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
    NSLog(@"applicationDidBecomeActive");
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
    NSLog(@"applicationWillResignActive");
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    // Saves changes in the application's managed object context before the application terminates.
    NSLog(@"applicationWillTerminate");
}

void uncaughtExceptionHandler(NSException *exception)
{
    [Flurry logError:@"UncaughtException" message:exception.debugDescription exception:exception];
}

@end
