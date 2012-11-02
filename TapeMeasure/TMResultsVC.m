//
//  TMResultsVC.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMResultsVC.h"
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>

@interface TMResultsVC ()

@end

@implementation TMResultsVC

- (void)viewDidLoad
{
    [super viewDidLoad];
	
    //give button rounded corners
	CALayer *layer = [self.upgradeBtn layer];
    [layer setMasksToBounds:YES];
    [layer setCornerRadius:10.0];
    [layer setBorderWidth:1.0];
    [layer setBorderColor:[[UIColor grayColor] CGColor]];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)viewDidUnload {
    [self setUpgradeBtn:nil];
    [super viewDidUnload];
}
@end
