//
//  MPPreferencesController.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface MPPreferencesController : UIViewController <UIGestureRecognizerDelegate>

@property (weak, nonatomic) IBOutlet UISegmentedControl *unitsControl;
@property (weak, nonatomic) IBOutlet UISwitch *allowLocationSwitch;
@property (weak, nonatomic) IBOutlet UIView *containerView;

- (IBAction)handleUnitsControl:(id)sender;
- (IBAction)handleAllowLocationSwitch:(id)sender;

@end
