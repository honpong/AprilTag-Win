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

- (BOOL) prefersStatusBarHidden { return YES; }

- (NSUInteger) supportedInterfaceOrientations { return UIInterfaceOrientationMaskPortrait; }

- (IBAction)handleNextButton:(id)sender
{
    TMLocationIntro* vc = [self.storyboard instantiateViewControllerWithIdentifier:@"LocationIntro"];
    vc.calibrationDelegate = self.calibrationDelegate;
    vc.sensorDelegate = self.sensorDelegate;
    [self presentViewController:vc animated:YES completion:nil];
}

@end
