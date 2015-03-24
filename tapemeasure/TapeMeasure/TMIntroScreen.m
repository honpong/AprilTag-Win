//
//  TMIntroScreen.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 9/4/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "TMIntroScreen.h"
#import "TMLocationIntro.h"

@interface TMIntroScreen ()

@end

@implementation TMIntroScreen

- (void)viewDidLoad
{
    [super viewDidLoad];
}

- (void) viewDidAppear:(BOOL)animated
{
    [TMAnalytics logEvent:@"View.IntroScreen"];
}

- (BOOL) prefersStatusBarHidden { return YES; }

- (NSUInteger) supportedInterfaceOrientations { return UIInterfaceOrientationMaskPortrait; }

- (IBAction)handleNextButton:(id)sender
{
    [RCAVSessionManager requestCameraAccessWithCompletion:^(BOOL granted){
        if(granted)
        {
            dispatch_async(dispatch_get_main_queue(), ^{
                [TMAnalytics logEvent:@"Permission.Camera" withParameters:@{ @"Allowed" : @"Yes" }];
                
                TMLocationIntro* vc = [self.storyboard instantiateViewControllerWithIdentifier:@"LocationIntro"];
                vc.calibrationDelegate = self.calibrationDelegate;
                [self presentViewController:vc animated:YES completion:nil];
            });
        }
        else
        {
            dispatch_async(dispatch_get_main_queue(), ^{
                [TMAnalytics logEvent:@"Permission.Camera" withParameters:@{ @"Allowed" : @"No" }];
                
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
