//
//  MPPreferencesController.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface MPPreferencesController : UITableViewController

@property (weak, nonatomic) IBOutlet UISegmentedControl *unitsControl;
@property (weak, nonatomic) IBOutlet UIButton *backButton;

- (IBAction)handleUnitsControl:(id)sender;
- (IBAction)handleBackButton:(id)sender;

@end
