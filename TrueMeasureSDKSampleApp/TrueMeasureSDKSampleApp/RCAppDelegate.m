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

- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation
{
    if ([sourceApplication isEqualToString:kTMTrueMeasureBundleId]) // Make sure the request actually came from TrueMeasure. Very important for security!
    {
        if ([url.host isEqualToString:kTMUrlActionMeasuredPhoto]) // Check if this is a measured photo URL by checking the "host" part of the URL
        {
            NSError* error;
            TMMeasuredPhoto* measuredPhoto = [TMMeasuredPhoto retrieveMeasuredPhotoWithUrl:url withError:&error]; // Make sure you pass a pointer to the error, not the error object itself
            
            if (error) // If an error occurred, error will be non-nil.
            {
                UIAlertView *alert = [[UIAlertView alloc] initWithTitle:[NSString stringWithFormat:@"Error %i", (int)error.code]
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
        }
    }

    return NO;
}

@end
