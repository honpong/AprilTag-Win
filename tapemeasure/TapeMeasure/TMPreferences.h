//
//  TMPreferences.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 6/5/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface TMPreferences : UIViewController <UIGestureRecognizerDelegate>

@property (weak, nonatomic) IBOutlet UISegmentedControl *unitsControl;
@property (weak, nonatomic) IBOutlet UISwitch *saveLocationSwitch;
@property (weak, nonatomic) IBOutlet UISwitch *allowLocationSwitch;
@property (weak, nonatomic) IBOutlet UILabel *saveLocationLabel;
@property (weak, nonatomic) IBOutlet UIView *containerView;

- (IBAction)handleUnitsControl:(id)sender;
- (IBAction)handleSaveLocationSwitch:(id)sender;
- (IBAction)handleAllowLocationSwitch:(id)sender;

@end
