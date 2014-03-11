//
//  MainViewController.m
//  RCCapture
//
//  Created by Brian on 2/24/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

#import "MainViewController.h"
#import "AppDelegate.h"

@interface MainViewController ()

@end

@implementation MainViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view.
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction) calibrateClicked:(id)sender
{
    AppDelegate * app = (AppDelegate *)[[UIApplication sharedApplication] delegate];
    [app startFromCalibration];
}

- (IBAction) captureClicked:(id)sender
{
    AppDelegate * app = (AppDelegate *)[[UIApplication sharedApplication] delegate];
    [app startFromCapture];
}

- (IBAction) replayClicked:(id)sender
{
    AppDelegate * app = (AppDelegate *)[[UIApplication sharedApplication] delegate];
    [app startFromReplay];
}

- (IBAction) liveClicked:(id)sender
{
    AppDelegate * app = (AppDelegate *)[[UIApplication sharedApplication] delegate];
    [app startFromLive];
}

@end
