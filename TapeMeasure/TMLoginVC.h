//
//  TMLoginVC.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 2/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface TMLoginVC : UITableViewController
@property (weak, nonatomic) IBOutlet UITextField *emailBox;
@property (weak, nonatomic) IBOutlet UITextField *passwordBox;
@property (weak, nonatomic) IBOutlet UISegmentedControl *actionTypeButton;
@property (weak, nonatomic) IBOutlet UIButton *loginButton;

- (IBAction)handleLoginButton:(id)sender;
- (IBAction)handleActionTypeButton:(id)sender;
@end
