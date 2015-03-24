//
//  TMIntroScreen.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 9/4/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "TMViewController.h"

@protocol RCCalibrationDelegate, RCSensorDelegate;

@interface TMIntroScreen : TMViewController

@property (weak, nonatomic) IBOutlet UIButton *nextButton;
@property (weak, nonatomic) IBOutlet UILabel *textLabel;
@property (weak, nonatomic) id<RCCalibrationDelegate> calibrationDelegate;

- (IBAction)handleNextButton:(id)sender;

@end
