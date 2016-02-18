//
//  MPLocationIntro.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPLocationIntro.h"
#import "RCCalibration1.h"
#import <CoreLocation/CoreLocation.h>
#import "MPIntroScreen.h"

static const CLLocationDegrees latitude = 35.;
static const CLLocationDegrees startingLongitude = 43.;

@interface MPLocationIntro ()

@end

@implementation MPLocationIntro
{
    NSString* originalTextFromStoryboard;
    NSString* originalNextButtonText;
    NSString* originalLaterButtonText;
    NSString* originalNeverButtonText;
    
    NSTimer* mapTimer;
}

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        
    }
    return self;
}

- (BOOL) prefersStatusBarHidden { return YES; }

- (NSUInteger) supportedInterfaceOrientations { return UIInterfaceOrientationMaskPortrait; }

- (void) viewDidLoad
{
    [super viewDidLoad];
    
    [self.navigationController setNavigationBarHidden:YES animated:NO];
    
    originalTextFromStoryboard = self.introLabel.text;
    originalNextButtonText = self.nextButton.titleLabel.text;
    originalLaterButtonText = self.laterButton.titleLabel.text;
    originalNeverButtonText = self.neverButton.titleLabel.text;
    
    [self setIntroText];
    
    self.mapView.alpha = 0;
}

- (void) viewDidAppear:(BOOL)animated
{
    [MPAnalytics logEvent:@"View.LocationIntro"];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleResume)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handlePause)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
    
    [self setIntroText];
    
    [self.mapView setVisibleMapRect:MKMapRectWorld animated:NO];
    CLLocationCoordinate2D center = CLLocationCoordinate2DMake(latitude, startingLongitude);
    [self.mapView setCenterCoordinate:center animated:YES];
    [self.mapView fadeInWithDuration:.5 andWait:0];
    [self startMapAnimation];
}

- (void) viewWillDisappear:(BOOL)animated
{
    [self stopMapAnimation];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void) handleResume
{
    [self setIntroText];
    [self startMapAnimation];
}

- (void) handlePause
{
    [self stopMapAnimation];
}

- (IBAction) handleNextButton:(id)sender
{
    [MPAnalytics logEvent:@"Choice.LocationIntro" withParameters:@{ @"Button" : @"Allow" }];
    
    NSNumber* timestamp = [NSNumber numberWithDouble:[[NSDate date] timeIntervalSince1970]];
    [NSUserDefaults.standardUserDefaults setObject:timestamp forKey:PREF_LOCATION_NAG_TIMESTAMP];
    
    [LOCATION_MANAGER requestLocationAccessWithCompletion:^(BOOL granted)
     {
         [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_SHOW_LOCATION_NAG];
         
         if(granted)
         {
             [MPAnalytics logEvent:@"Permission.Location" withParameters:@{ @"Allowed" : @"Yes" }];
             [NSUserDefaults.standardUserDefaults setBool:YES forKey:PREF_USE_LOCATION];
             [LOCATION_MANAGER startLocationUpdates];
         }
         else
         {
             [MPAnalytics logEvent:@"Permission.Location" withParameters:@{ @"Allowed" : @"No" }];
         }
         
         [self gotoNextScreen];
     }];
}

- (IBAction)handleLaterButton:(id)sender
{
    [MPAnalytics logEvent:@"Choice.LocationIntro" withParameters:@{ @"Button" : @"Later" }];
    
    NSNumber* timestamp = [NSNumber numberWithDouble:[[NSDate date] timeIntervalSince1970]];
    [NSUserDefaults.standardUserDefaults setObject:timestamp forKey:PREF_LOCATION_NAG_TIMESTAMP];
    [self gotoNextScreen];
}

- (IBAction)handleNeverButton:(id)sender
{
    [MPAnalytics logEvent:@"Choice.LocationIntro" withParameters:@{ @"Button" : @"Never" }];
    
    [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_SHOW_LOCATION_NAG];
    [self gotoNextScreen];
}

- (void) setIntroText
{
    if ([CLLocationManager locationServicesEnabled])
    {
        [self.nextButton setTitle:originalNextButtonText forState:UIControlStateNormal];
        [self.laterButton setTitle:originalLaterButtonText forState:UIControlStateNormal];
        [self.neverButton setTitle:originalNeverButtonText forState:UIControlStateNormal];
        self.introLabel.text = originalTextFromStoryboard;
    }
    else
    {
        [self.nextButton setTitle:@"Turn on location services" forState:UIControlStateNormal];
        [self.laterButton setTitle:@"Later" forState:UIControlStateNormal];
        [self.neverButton setTitle:@"Skip" forState:UIControlStateNormal];
        self.introLabel.text = @"I see that you have location services disabled. If you allow this app to use your location, it can make your measurements more accurate by adjusting for differences in gravity across the earth. This is optional, but recommended";
    }
    
    [self.introLabel sizeToFit];
}

- (void) startMapAnimation
{
    mapTimer = [NSTimer timerWithTimeInterval:0.0333 target:self selector:@selector(moveMap) userInfo:nil repeats:YES];
    
    [[NSRunLoop mainRunLoop] addTimer:mapTimer forMode:NSDefaultRunLoopMode];
}

- (void) moveMap
{
    CLLocationDegrees longitude = self.mapView.centerCoordinate.longitude;
    longitude = longitude + .1;
    if (longitude >= 180.) longitude = -180.;
    
    CLLocationCoordinate2D center = CLLocationCoordinate2DMake(latitude, longitude);
    
    [self.mapView setCenterCoordinate:center animated:NO];
}

- (void) stopMapAnimation
{
    [mapTimer invalidate];
}

- (void) gotoNextScreen
{
    if ([self.presentingViewController isKindOfClass:[MPIntroScreen class]])
    {
        [self gotoCalibration];
    }
    else
    {
        [self gotoMeasurementResult];
    }
}

- (void) gotoMeasurementResult
{
//    TMResultsVC* vc = [self.storyboard instantiateViewControllerWithIdentifier:@"Results"];
//    vc.theMeasurement = self.theMeasurement;
//    [self.navigationController pushViewController:vc animated:YES];
}

- (void) gotoCalibration
{
    UIStoryboard* calStoryboard = [UIStoryboard storyboardWithName:@"Calibration" bundle:[NSBundle mainBundle]];
    RCCalibration1 * calibration1 = [calStoryboard instantiateViewControllerWithIdentifier:@"Calibration1"];
    calibration1.calibrationDelegate = self.calibrationDelegate;
    calibration1.modalPresentationStyle = UIModalPresentationFullScreen;
    [self presentViewController:calibration1 animated:YES completion:nil];
}

@end
