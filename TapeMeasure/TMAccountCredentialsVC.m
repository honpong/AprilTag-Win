//
//  TMAccountCredentialsVC.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 2/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMAccountCredentialsVC.h"

@interface TMAccountCredentialsVC ()

@end

@implementation TMAccountCredentialsVC

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

    
}

- (void)viewDidUnload {
    [self setCreateAccountButton:nil];
    [self setLoginButton:nil];
    [super viewDidUnload];
}

- (IBAction)handleCreateAccountButton:(id)sender {
}

- (IBAction)handleLoginButton:(id)sender {
}
@end
