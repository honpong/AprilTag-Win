//
//  MPIntroScreen.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@protocol RCCalibrationDelegate, RCSensorDelegate;

@interface MPIntroScreen : UIViewController

@property (weak, nonatomic) IBOutlet UIButton *nextButton;
@property (weak, nonatomic) IBOutlet UILabel *textLabel;
@property (weak, nonatomic) id<RCCalibrationDelegate> calibrationDelegate;

- (IBAction)handleNextButton:(id)sender;

@end
