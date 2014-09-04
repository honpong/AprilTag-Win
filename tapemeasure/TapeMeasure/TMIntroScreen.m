//
//  TMIntroScreen.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 9/4/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "TMIntroScreen.h"
#import "RCCalibration1.h"

@interface TMIntroScreen ()

@end

@implementation TMIntroScreen

- (void)viewDidLoad
{
    [super viewDidLoad];
}

- (IBAction)handleNextButton:(id)sender
{
    RCCalibration1 * calibration1 = [RCCalibration1 instantiateViewController];
    calibration1.calibrationDelegate = self.calibrationDelegate;
    calibration1.sensorDelegate = self.sensorDelegate;
    calibration1.modalPresentationStyle = UIModalPresentationFullScreen;
    [self presentViewController:calibration1 animated:YES completion:nil];
}

@end
