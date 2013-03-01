//
//  TMLoginVC.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 2/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMLoginVC.h"

@interface TMLoginVC ()

@end

@implementation TMLoginVC

typedef enum {
    AlertAddToAccount = 100, AlertLoginFailure = 200
} AlertId;

MBProgressHUD *HUD;
NSArray *fieldArray;
id activeField;

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
        [navigationArray removeObjectAtIndex: navigationArray.count - 2];  // remove create account from nav array, so back button goes to history instead
        self.navigationController.viewControllers = navigationArray;
    }
    
    fieldArray = [NSArray arrayWithObjects: self.emailBox, self.passwordBox, nil];
    for (UITextField *field in fieldArray) field.delegate = self;
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
    
    id nextField = [fieldArray objectAtIndex:index + 1];
    activeField = nextField;
    [nextField becomeFirstResponder];
    
    return NO;
}

- (IBAction)handleLoginButton:(id)sender
{
    [self login];
}

- (void)login
{
    HUD = [[MBProgressHUD alloc] initWithView:self.navigationController.view];
	[self.navigationController.view addSubview:HUD];
	HUD.labelText = @"Thinking";
	
	[HUD show:YES];
    
    RCUser *user = [RCUser getStoredUser];
    
    user.username = [self.emailBox.text stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceCharacterSet]]; //we use email as username
    user.password = self.passwordBox.text;
    //TODO: get full name?
    
    //TODO: validate input
    
    [USER_MANAGER
     loginWithUsername:user.username
     withPassword:user.password
     onSuccess:^()
     {
         [HUD hide:YES];
         [user saveUser];
         
         if ([TMMeasurement getMeasurementCount])
         {
             UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Pardon me"
                                                             message:@"I see you've taken some measurements. Nice! Would you like to add them to your account? If not, I'll just delete them for you."
                                                            delegate:self
                                                   cancelButtonTitle:@"No"
                                                   otherButtonTitles:@"Yes", nil];
             alert.tag = AlertAddToAccount;
             [alert show];
         }
         else
         {
             [self afterLogin];
         }
     }
     onFailure:^(int statusCode)
     {
         [HUD hide:YES];
         NSLog(@"Login form failure callback");
         
         NSString *msg;
         if (statusCode == 200)
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
         //TODO: show detailed message to user
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
            [TMMeasurement deleteAllMeasurements];
        }
        else if (buttonIndex == 1) //YES
        {
            [TMMeasurement markAllPendingUpload];
        }
        
        [self afterLogin];
    }
}

- (IBAction)handleActionTypeButton:(id)sender
{
    if (self.actionTypeButton.selectedSegmentIndex == 0)
    {
        [self performSegueWithIdentifier:@"toCreateAccount" sender:self];
    }
}
@end
