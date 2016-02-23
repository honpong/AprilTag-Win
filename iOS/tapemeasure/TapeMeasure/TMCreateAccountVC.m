//
//  TMAccountCredentialsVC.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 2/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMCreateAccountVC.h"
#import "TMLoginVC.h"
#import "TMAnalytics.h"
#import "TMConstants.h"
#import <RCCore/RCCore.h>

@implementation TMCreateAccountVC

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
    
    fieldArray = @[self.emailBox, self.passwordBox, self.passwordAgainBox, self.firstNameBox, self.lastNameBox];
    for (UITextField *field in fieldArray) field.delegate = self;
}

- (void)viewDidAppear:(BOOL)animated
{
    [TMAnalytics logEvent:@"View.CreateAccount"];
    [super viewDidAppear:animated];
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

//handles next and go buttons on keyboard
- (BOOL) textFieldShouldReturn:(UITextField *) textField {
    BOOL didResign = [textField resignFirstResponder];
    if (!didResign) return NO;
    
    NSUInteger index = [fieldArray indexOfObject:textField];
    if (index == NSNotFound || index + 1 == fieldArray.count)
    {
        [self validateAndSubmit];
        return NO;
    }
    
    id nextField = fieldArray[index + 1];
    activeField = nextField;
    [nextField becomeFirstResponder];
    
    return NO;
}

- (IBAction)handleActionTypeButton:(id)sender
{
    if (self.actionTypeButton.selectedSegmentIndex == 1)
    {
        // when going to next view controller, remove self from nav stack
        TMLoginVC* loginController = [self.storyboard instantiateViewControllerWithIdentifier:@"Login"];
        UINavigationController *navController = self.navigationController;
        [navController popViewControllerAnimated:NO];
        [navController pushViewController:loginController animated:YES];
    }
}

- (IBAction)handleCreateAccountButton:(id)sender
{
    [self validateAndSubmit];
}

- (void)validateAndSubmit
{
    [self.view endEditing:YES]; //hides keyboard
    
    if ([self isInputValid])
    {
        [self updateUserAccount];
    }
}

- (BOOL)isInputValid
{
    self.emailBox.text = [self.emailBox.text stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceCharacterSet]];
    
    if (![RCUser isValidEmail:self.emailBox.text])
    {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Whoops"
                                                        message:@"Check your email address"
                                                       delegate:nil
                                              cancelButtonTitle:@"OK"
                                              otherButtonTitles:nil];
        
        [alert show];
        return NO;
    }
    
    if (self.passwordBox.text.length < 6)
    {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Whoops"
                                                        message:@"Please choose a password with at least 6 characters"
                                                       delegate:nil
                                              cancelButtonTitle:@"OK"
                                              otherButtonTitles:nil];
        
        [alert show];
        return NO;
    }
    
    if ([self.passwordBox.text isEqualToString:self.passwordAgainBox.text])
    {
        return YES;
    }
    else
    {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Whoops"
                                                        message:@"Passwords don't match"
                                                       delegate:nil
                                              cancelButtonTitle:@"OK"
                                              otherButtonTitles:nil];
        
        [alert show];
        return NO;
    }
}

- (void)updateUserAccount
{
    HUD = [[MBProgressHUD alloc] initWithView:self.navigationController.view];
	[self.navigationController.view addSubview:HUD];
	HUD.labelText = @"Thinking";
	
	[HUD show:YES];
    
    RCUser *user = [RCUser getStoredUser];
    
    user.username = self.emailBox.text; //we use email as username
    user.password = self.passwordBox.text;
    user.firstName = [self.firstNameBox.text stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceCharacterSet]];
    user.lastName = [self.lastNameBox.text stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceCharacterSet]];
    
    if ([USER_MANAGER getLoginState] == LoginStateYes)
    {
        __weak TMCreateAccountVC* weakSelf = self;
        [USER_MANAGER
         updateUser:user
         onSuccess:^()
         {
             [TMAnalytics logEvent:@"User.CreateAccount"];
             
             [HUD hide:YES];
             [user saveUser];
             [weakSelf performSegueWithIdentifier:@"toHistory" sender:weakSelf];
         }
         onFailure:^(NSInteger statusCode)
         {
             [HUD hide:YES];
             DLog(@"Update user failure callback");
             
             UIAlertView *alert = [weakSelf getFailureAlertForStatusCode:statusCode];
             [alert show];
         }
         ];
    }
    else
    {
        [USER_MANAGER
         loginWithStoredCredentials:^{
             //do nothing
         }
         onFailure:^(NSInteger statusCode) {
             //do nothing
         }];
        
        [HUD hide:YES];
        
        UIAlertView *alert = [self getFailureAlertForStatusCode:0];
        [alert show];
    }
}

- (UIAlertView*)getFailureAlertForStatusCode:(NSInteger)statusCode
{
    NSString *title;
    NSString *msg = @"Whoops, sorry about that! Try again later.";
    
    if (statusCode >= 500 && statusCode < 600)
    {
        title = @"Server error";
    }
    else if (statusCode == 409)
    {
        title = @"Whoa now";
        msg = @"Looks like that email address has already been registered. Maybe try logging in?";
    }
    else
    {
        title = @"Something went wrong";
    }
    
    return [[UIAlertView alloc] initWithTitle:title
                                      message:msg
                                     delegate:nil
                            cancelButtonTitle:@"OK"
                            otherButtonTitles:nil];
}
@end
