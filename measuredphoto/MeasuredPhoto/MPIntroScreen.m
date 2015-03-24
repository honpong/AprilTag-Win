//
//  MPIntroScreen.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPIntroScreen.h"
#import "MPAnalytics.h"
#import "MPLocationIntro.h"

@interface MPIntroScreen ()

@end

@implementation MPIntroScreen

- (void)viewDidLoad {
    [super viewDidLoad];
}

- (void) viewDidAppear:(BOOL)animated
{
    [MPAnalytics logEvent:@"View.IntroScreen"];
}

- (BOOL) prefersStatusBarHidden { return YES; }

- (NSUInteger) supportedInterfaceOrientations { return UIInterfaceOrientationMaskPortrait; }

- (IBAction)handleNextButton:(id)sender
{
    [RCAVSessionManager requestCameraAccessWithCompletion:^(BOOL granted){
        if(granted)
        {
            dispatch_async(dispatch_get_main_queue(), ^{
                [MPAnalytics logEvent:@"Permission.Camera" withParameters:@{ @"Allowed" : @"Yes" }];
                
                MPLocationIntro* vc = [self.storyboard instantiateViewControllerWithIdentifier:@"LocationIntro"];
                vc.calibrationDelegate = self.calibrationDelegate;
                [self presentViewController:vc animated:YES completion:nil];
            });
        }
        else
        {
            dispatch_async(dispatch_get_main_queue(), ^{
                [MPAnalytics logEvent:@"Permission.Camera" withParameters:@{ @"Allowed" : @"No" }];
                
                UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"No camera access."
                                                                message:@"This app cannot function without camera access. Turn it on in Settings/Privacy/Camera."
                                                               delegate:nil
                                                      cancelButtonTitle:@"OK"
                                                      otherButtonTitles:nil];
                [alert show];
            });
        }
    }];
}

@end
