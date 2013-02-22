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
    
}
@end
