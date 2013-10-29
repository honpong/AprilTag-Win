//
//  RCAppDelegate.m
//  TrueMeasureSDKSampleApp
//
//  Created by Ben Hirashima on 10/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCAppDelegate.h"
#import <TrueMeasureSDK/TrueMeasureSDK.h>
#import "RCViewController.h"

@implementation RCAppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    // Override point for customization after application launch.
    return YES;
}
							
- (void)applicationWillResignActive:(UIApplication *)application
{
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
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

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}

- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation
{
    if ([sourceApplication isEqualToString:kTMTrueMeasureBundleId]) // Make sure the request actually came from TrueMeasure
    {
        NSError* error;
        TMMeasuredPhoto* measuredPhoto = [TMMeasuredPhoto retrieveFromUrl:url withError:&error]; // Make sure you pass a pointer to the error, not the error object itself
        
        if (error) // If an error occurred, error will be non-nil.
        {
            UIAlertView *alert = [[UIAlertView alloc] initWithTitle:[NSString stringWithFormat:@"Error %i", error.code]
                                                           message:error.localizedDescription
                                                          delegate:self
                                                 cancelButtonTitle:nil
                                                 otherButtonTitles:@"OK", nil];
            [alert show];
            return NO;
        }
        
        if (measuredPhoto)
        {
            RCViewController* vc = (RCViewController*)self.window.rootViewController;
            [vc setMeasuredPhoto:measuredPhoto];
            
            return YES;
        }
        else
        {
            return NO;
        }
    }
    else
    {
        return NO;
    }
}

@end
