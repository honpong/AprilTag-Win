//
//  TMAccountCredentialsVC.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 2/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCCore/RCUserManager.h"
#import "RCCore/RCUser.h"
#import "MBProgressHUD.h"
#import "TMTableViewController.h"

@interface TMCreateAccountVC : TMTableViewController <UITextFieldDelegate>
{
    MBProgressHUD *HUD;
    NSArray *fieldArray;
    id activeField;
}

@property (weak, nonatomic) IBOutlet UIButton *loginButton;
@property (weak, nonatomic) IBOutlet UISegmentedControl *actionTypeButton;
@property (weak, nonatomic) IBOutlet UITableViewCell *passwordAgainCell;
@property (weak, nonatomic) IBOutlet UITextField *emailBox;
@property (weak, nonatomic) IBOutlet UITextField *passwordBox;
@property (weak, nonatomic) IBOutlet UITextField *passwordAgainBox;
@property (weak, nonatomic) IBOutlet UILabel *passwordAgainLabel;
@property (weak, nonatomic) IBOutlet UITextField *firstNameBox;
@property (weak, nonatomic) IBOutlet UILabel *firstNameLabel;
@property (weak, nonatomic) IBOutlet UITextField *lastNameBox;
@property (weak, nonatomic) IBOutlet UILabel *lastNameLabel;

- (IBAction)handleCreateAccountButton:(id)sender;
- (IBAction)handleActionTypeButton:(id)sender;

@end
