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
    
}

- (IBAction)handleActionTypeButton:(id)sender
{
    if (self.actionTypeButton.selectedSegmentIndex == 0)
    {
        [self performSegueWithIdentifier:@"toCreateAccount" sender:self];
    }
}
@end
