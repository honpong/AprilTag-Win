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
}

- (void)viewDidUnload {
    [self setEmailBox:nil];
    [self setPasswordBox:nil];
    [self setActionTypeButton:nil];
    [self setLoginButton:nil];
    [super viewDidUnload];
}

- (IBAction)handleLoginButton:(id)sender
{
    HUD = [[MBProgressHUD alloc] initWithView:self.navigationController.view];
	[self.navigationController.view addSubview:HUD];
	HUD.labelText = @"Thinking";
	
	[HUD show:YES];
    
    RCUser *user = [USER_MANAGER getStoredUser];
    
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
         
         UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Whoops, login failed :("
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
