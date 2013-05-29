//
//  TMOptionsVCViewController.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/1/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMMeasurement.h"

@protocol OptionsDelegate <NSObject>

- (void) didDismissOptions;

@end

@interface TMOptionsVC : UIViewController

@property (weak, nonatomic) id delegate;
@property (weak, nonatomic) IBOutlet UISegmentedControl *btnFractional;
@property (weak, nonatomic) IBOutlet UISegmentedControl *btnUnits;
@property (weak, nonatomic) IBOutlet UISegmentedControl *btnScale;

@property (nonatomic, strong) TMMeasurement *theMeasurement;

- (IBAction)handleFractionalButton:(id)sender;
- (IBAction)handleUnitsButton:(id)sender;
- (IBAction)handleScaleButton:(id)sender;

@end
