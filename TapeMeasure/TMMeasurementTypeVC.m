//
//  TMMeasurementTypeVCViewController.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/30/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMMeasurementTypeVC.h"

@interface TMMeasurementTypeVC ()

@end

@implementation TMMeasurementTypeVC

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
    
    shouldEndAVSession = YES;
}

- (void)viewWillAppear:(BOOL)animated
{
    [self.navigationController setToolbarHidden:YES animated:animated];
    [super viewWillAppear:animated];
}

- (void)viewDidAppear:(BOOL)animated
{
    [TMAnalytics logEvent:@"View.ChooseType"];
    [super viewDidAppear:animated];
    [SESSION_MANAGER startSession];
}

- (void)viewWillDisappear:(BOOL)animated
{
    if (shouldEndAVSession) [SESSION_MANAGER endSession];
}

- (void)viewDidUnload {
    [self setScrollView:nil];
    [super viewDidUnload];
}

- (IBAction)handlePointToPoint:(id)sender
{
    type = TypePointToPoint;
    [self performSegueWithIdentifier:@"toNew" sender:self];
}

- (IBAction)handleTotalPath:(id)sender
{
    type = TypeTotalPath;
    [self performSegueWithIdentifier:@"toNew" sender:self];
}

- (IBAction)handleHorizontal:(id)sender
{
    type = TypeHorizontal;
    [self performSegueWithIdentifier:@"toNew" sender:self];
}

- (IBAction)handleVertical:(id)sender
{
    type = TypeVertical;
    [self performSegueWithIdentifier:@"toNew" sender:self];
}

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if([[segue identifier] isEqualToString:@"toNew"])
    {
        shouldEndAVSession = NO;
        
        newVC = [segue destinationViewController];
        newVC.type = type;
    }
}

@end
