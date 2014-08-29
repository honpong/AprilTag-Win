//
//  TMLocationIntro.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 8/28/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "TMLocationIntro.h"
@import CoreLocation;

@interface TMLocationIntro ()

@end

@implementation TMLocationIntro

- (id) initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void) viewDidLoad
{
    [super viewDidLoad];
}

- (void) viewDidAppear:(BOOL)animated
{
    if ([CLLocationManager locationServicesEnabled])
    {
        self.introLabel.text = @"Welcome to Endless Tape Measure. When you tap Next, we will ask for permission to use your current location. This is optional, but it makes your measurements more accurate by adjusting for differences in gravity across the planet.";
    }
    else
    {
        self.introLabel.text = @"Welcome to Endless Tape Measure. I see that you have location services disabled. If you allow this app to use your location, it can make your measurements more accurate. This is completely optional. You can enable location in the Settings app, under 'Privacy'.";
    }
}

- (IBAction) handleNextButton:(id)sender
{
    if ([self.delegate respondsToSelector:@selector(nextButtonTapped)])
    {
        [self.delegate nextButtonTapped];
    }
}

@end
