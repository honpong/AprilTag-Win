//
//  TMPreferences.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 6/5/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface TMPreferences : UITableViewController

@property (weak, nonatomic) IBOutlet UISegmentedControl *unitsControl;
@property (weak, nonatomic) IBOutlet UISwitch *locationSwitch;

- (IBAction)handleUnitsControl:(id)sender;
- (IBAction)handleLocationSwitch:(id)sender;

@end
