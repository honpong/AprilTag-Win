//
//  TMOptionsVCViewController.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/1/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMMeasurement.h"
#import <UIKit/UIKit.h>

@protocol OptionsDelegate <NSObject>

- (void) didChangeOptions;
- (void) didDismissOptions;

@end

@interface TMOptionsVC : UIViewController <UIGestureRecognizerDelegate>

@property (weak, nonatomic) id delegate;
@property (weak, nonatomic) IBOutlet UISegmentedControl *btnFractional;
@property (weak, nonatomic) IBOutlet UISegmentedControl *btnUnits;
@property (weak, nonatomic) IBOutlet UISegmentedControl *btnScale;
@property (weak, nonatomic) IBOutlet UIView *containerView;

@property (nonatomic, strong) TMMeasurement *theMeasurement;

- (IBAction)handleFractionalButton:(id)sender;
- (IBAction)handleUnitsButton:(id)sender;
- (IBAction)handleScaleButton:(id)sender;

@end
