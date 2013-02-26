//
//  TMAccountCredentialsVC.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 2/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMCreateAccountVC.h"

@interface TMCreateAccountVC ()

@end

@implementation TMCreateAccountVC

MBProgressHUD *HUD;

- (id)initWithStyle:(UITableViewStyle)style
{
    self = [super initWithStyle:style];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    NSMutableArray *navigationArray = [[NSMutableArray alloc] initWithArray: self.navigationController.viewControllers];
    
    if(navigationArray.count > 2)
    {
        [navigationArray removeObjectAtIndex: navigationArray.count - 2];  // remove login from nav array, so back button goes to history instead
        self.navigationController.viewControllers = navigationArray;
    }
}

- (void)viewDidUnload
{
    [self setLoginButton:nil];
    [self setActionTypeButton:nil];
    [self setPasswordAgainCell:nil];
    [self setEmailBox:nil];
    [self setPasswordBox:nil];
    [self setPasswordAgainBox:nil];
    [self setPasswordAgainLabel:nil];
    [self setFirstNameBox:nil];
    [self setFirstNameLabel:nil];
    [self setLastNameBox:nil];
    [self setLastNameLabel:nil];
    [super viewDidUnload];
}

- (IBAction)handleActionTypeButton:(id)sender
{
    if (self.actionTypeButton.selectedSegmentIndex == 1)
    {
        [self performSegueWithIdentifier:@"toLogin" sender:self];
    }
}

- (IBAction)handleLoginButton:(id)sender
{
//    UIActivityIndicatorView *spinner = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhite];
//    spinner.frame = CGRectMake(0.0, 0.0, 40.0, 40.0);
//	spinner.center = self.view.center;
//    spinner.backgroundColor = [UIColor darkGrayColor];
//    CGAffineTransform transform = CGAffineTransformMakeScale(1.5f, 1.5f);
//    spinner.transform = transform;
//    spinner.hidesWhenStopped = YES;
//	[self.tableView addSubview: spinner];
//    [spinner startAnimating];
    
    HUD = [[MBProgressHUD alloc] initWithView:self.navigationController.view];
	[self.navigationController.view addSubview:HUD];
	
//	HUD.delegate = self;
	HUD.labelText = @"Chewing";
	
	[HUD show:YES];
    
    RCUser *user = [USER_MANAGER getStoredUser];
    
    user.username = [self.emailBox.text stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceCharacterSet]]; //we use email as username
    user.password = self.passwordBox.text;
    user.firstName = [self.firstNameBox.text stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceCharacterSet]];
    user.lastName = [self.lastNameBox.text stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceCharacterSet]];
    
    if ([USER_MANAGER isLoggedIn])
    {
        [USER_MANAGER
         updateUser:user
         onSuccess:^()
         {
             [HUD hide:YES];
             [user saveUser];
             [self performSegueWithIdentifier:@"toHistory" sender:self];
         }
         onFailure:^(int statusCode)
         {
             [HUD hide:YES];
             NSLog(@"Update user failure callback");
             //TODO: show message to user
         }
         ];
    }
    
}

- (void)updateUserAccount
{
    RCUser *user = [USER_MANAGER getStoredUser];
    
    user.username = [self.emailBox.text stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceCharacterSet]]; //we use email as username
    user.password = self.passwordBox.text;
    user.firstName = [self.firstNameBox.text stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceCharacterSet]];
    user.lastName = [self.lastNameBox.text stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceCharacterSet]];
    
    if ([USER_MANAGER isLoggedIn])
    {
        [USER_MANAGER
         updateUser:user
         onSuccess:^()
         {
             [self performSegueWithIdentifier:@"toHistory" sender:self];
         }
         onFailure:^(int statusCode)
         {
             NSLog(@"Update user failure callback");
             //TODO: show message to user
         }
         ];
    }
}
@end
