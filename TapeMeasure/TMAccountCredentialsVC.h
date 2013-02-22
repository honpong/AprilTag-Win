//
//  TMAccountCredentialsVC.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 2/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface TMAccountCredentialsVC : UITableViewController
@property (weak, nonatomic) IBOutlet UIButton *createAccountButton;
@property (weak, nonatomic) IBOutlet UIButton *loginButton;

- (IBAction)handleCreateAccountButton:(id)sender;
- (IBAction)handleLoginButton:(id)sender;
@end
