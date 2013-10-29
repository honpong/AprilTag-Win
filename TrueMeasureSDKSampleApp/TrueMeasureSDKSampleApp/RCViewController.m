//
//  RCViewController.m
//  TrueMeasureSDKSampleApp
//
//  Created by Ben Hirashima on 10/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCViewController.h"
#import <TrueMeasureSDK/TrueMeasureSDK.h>

#define API_KEY @"D3bed93A58f8A25FDF7Cbc4da0634D" // Put your API key here

@interface RCViewController ()

@end

@implementation RCViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
}

- (IBAction)handleButtonPress:(id)sender
{
    if ([TMMeasuredPhoto getHighestInstalledApiVersion] >= kTMApiVersion)
    {
        [TMMeasuredPhoto requestMeasuredPhoto:API_KEY];
    }
    else
    {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Error"
                                                        message:@"TrueMeasure does not appear to be installed, or there is an older version installed. Get it or update it from the AppStore."
                                                       delegate:self
                                              cancelButtonTitle:nil
                                              otherButtonTitles:@"OK", nil];
        [alert show];
    }
}

- (void) setMeasuredPhoto:(TMMeasuredPhoto *)measuredPhoto
{
    [self.imageView setImage:[UIImage imageWithData:measuredPhoto.imageData]];
}

@end
