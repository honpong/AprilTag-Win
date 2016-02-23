//
//  TMLoginVC.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 2/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMLoginVC.h"
#import "TMCreateAccountVC.h"
#import "TMAnalytics.h"
#import "TMConstants.h"
#import <RCCore/RCCore.h>

@implementation TMLoginVC

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
    
    fieldArray = @[self.emailBox, self.passwordBox];
    for (UITextField *field in fieldArray) field.delegate = self;
}

- (void)viewDidAppear:(BOOL)animated
{
    [TMAnalytics logEvent:@"View.Login"];
    [super viewDidAppear:animated];
}

- (void)viewDidUnload {
    [self setEmailBox:nil];
    [self setPasswordBox:nil];
    [self setActionTypeButton:nil];
    [self setLoginButton:nil];
    [super viewDidUnload];
}

//handles next and go buttons on keyboard
- (BOOL) textFieldShouldReturn:(UITextField *) textField {
    BOOL didResign = [textField resignFirstResponder];
    if (!didResign) return NO;
    
    NSUInteger index = [fieldArray indexOfObject:textField];
    if (index == NSNotFound || index + 1 == fieldArray.count)
    {
        [self login];
        return NO;
    }
    
    id nextField = fieldArray[index + 1];
    activeField = nextField;
    [nextField becomeFirstResponder];
    
    return NO;
}

- (IBAction)handleLoginButton:(id)sender
{
    [self login];
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
                                                        message:@"The password must be at least 6 characters long"
                                                       delegate:nil
                                              cancelButtonTitle:@"OK"
                                              otherButtonTitles:nil];
        
        [alert show];
        return NO;
    }
    
    return YES;
}

- (void)login
{
    [self.view endEditing:YES]; //hides keyboard
    
    if (![self isInputValid]) return;
    
    HUD = [[MBProgressHUD alloc] initWithView:self.navigationController.view];
	[self.navigationController.view addSubview:HUD];
	HUD.labelText = @"Thinking";
	
	[HUD show:YES];
    
    RCUser *user = [RCUser getStoredUser];
    
    user.username = self.emailBox.text; //we use email as username
    user.password = self.passwordBox.text;
    
     __weak TMLoginVC* weakSelf = self;
    [USER_MANAGER
     loginWithUsername:user.username
     withPassword:user.password
     onSuccess:^()
     {
         [HUD hide:YES];
         [user saveUser];
         
         [TMAnalytics logEvent:@"User.Login"];
         
         if ([TMMeasurement getCount])
         {
             UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Pardon me"
                                                             message:@"I see you've taken some measurements. Nice! Would you like to add them to your account? If not, I'll just delete them for you."
                                                            delegate:weakSelf
                                                   cancelButtonTitle:@"No"
                                                   otherButtonTitles:@"Yes", nil];
             alert.tag = AlertAddToAccount;
             [alert show];
         }
         else
         {
             [weakSelf afterLogin];
         }
     }
     onFailure:^(NSInteger statusCode)
     {
         [HUD hide:YES];
         DLog(@"Login form failure callback");
         
         NSString *msg;
         if (statusCode == 401)
         {
             msg = @"Whoops, username or password incorrect";
         }
         else
         {
             msg = @"Whoops, server error. Try again.";
         }
         
         UIAlertView *alert = [[UIAlertView alloc] initWithTitle:msg
                                                         message:nil
                                                        delegate:nil
                                               cancelButtonTitle:@"OK"
                                               otherButtonTitles:nil];
         
         alert.tag = AlertLoginFailure;
         [alert show];
     }
     ];
}

- (void)afterLogin
{
    [[NSUserDefaults standardUserDefaults] setInteger:0 forKey:PREF_LAST_TRANS_ID];
    [self performSegueWithIdentifier:@"toHistory" sender:self];
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    if (alertView.tag == AlertAddToAccount)
    {
        if (buttonIndex == 0) //NO
        {
            [DATA_MANAGER deleteAllData];
        }
        else if (buttonIndex == 1) //YES
        {
            [DATA_MANAGER markAllPendingUpload];
        }
        
        [self afterLogin];
    }
}

- (IBAction)handleActionTypeButton:(id)sender
{
    if (self.actionTypeButton.selectedSegmentIndex == 0)
    {
        // when going to next view controller, remove self from nav stack
        TMCreateAccountVC* createController = [self.storyboard instantiateViewControllerWithIdentifier:@"CreateAccount"];
        UINavigationController *navController = self.navigationController;
        [navController popViewControllerAnimated:NO];
        [navController pushViewController:createController animated:YES];
    }
}
@end
