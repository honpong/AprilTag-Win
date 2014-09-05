//
//  TMLocationIntro.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 8/28/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "TMLocationIntro.h"
#import "TMResultsVC.h"
@import CoreLocation;

const CLLocationDegrees latitude = 35.;
const CLLocationDegrees startingLongitude = 43.;

@interface TMLocationIntro ()

@end

@implementation TMLocationIntro
{
    NSString* originalTextFromStoryboard;
    NSString* originalNextButtonText;
    NSString* originalLaterButtonText;
    NSString* originalNeverButtonText;
    bool waitingForLocationAuthorization;
}

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handleResume)
                                                     name:UIApplicationDidBecomeActiveNotification
                                                   object:nil];
    }
    return self;
}

- (BOOL) prefersStatusBarHidden { return YES; }

- (NSUInteger) supportedInterfaceOrientations { return UIInterfaceOrientationMaskPortrait; }

- (void) viewDidLoad
{
    [super viewDidLoad];
    
    waitingForLocationAuthorization = false;
    
    [self.navigationController setNavigationBarHidden:YES animated:NO];
    
    originalTextFromStoryboard = self.introLabel.text;
    originalNextButtonText = self.nextButton.titleLabel.text;
    originalLaterButtonText = self.laterButton.titleLabel.text;
    originalNeverButtonText = self.neverButton.titleLabel.text;
    
    [self.mapView setVisibleMapRect:MKMapRectWorld animated:NO];
    [self.mapView setCenterCoordinate:CLLocationCoordinate2DMake(latitude, startingLongitude) animated:NO];
    
    [self setIntroText];
}

- (void) viewDidAppear:(BOOL)animated
{
    [self setIntroText];
    [self animateMap];
}

- (void) handleResume
{
    [self setIntroText];
}

- (IBAction) handleNextButton:(id)sender
{
    [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_SHOW_LOCATION_EXPLANATION];
    
    if([LOCATION_MANAGER shouldAttemptLocationAuthorization])
    {
        waitingForLocationAuthorization = true;
        LOCATION_MANAGER.delegate = self;
        [LOCATION_MANAGER startLocationUpdates]; // will show dialog asking user to authorize location. gotoCalibration triggered by didChangeAuthorizationStatus delegate function
    }
    else
    {
        [self gotoMeasurementResult];
    }
}

- (IBAction)handleLaterButton:(id)sender
{
    if (!waitingForLocationAuthorization) [self gotoMeasurementResult];
}

- (IBAction)handleNeverButton:(id)sender
{
    if (!waitingForLocationAuthorization)
    {
        [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_SHOW_LOCATION_EXPLANATION];
        [self gotoMeasurementResult];
    }
}

- (void) setIntroText
{
    if ([CLLocationManager locationServicesEnabled])
    {
        self.nextButton.hidden = NO;
        [self.laterButton setTitle:originalLaterButtonText forState:UIControlStateNormal];
        [self.neverButton setTitle:originalNeverButtonText forState:UIControlStateNormal];
        self.introLabel.text = originalTextFromStoryboard;
    }
    else
    {
        self.nextButton.hidden = YES;
        [self.laterButton setTitle:@"Remind me later" forState:UIControlStateNormal];
        [self.neverButton setTitle:@"Skip" forState:UIControlStateNormal];
        self.introLabel.text = @"Congratulations on taking a measurement! I see that you have location services disabled. If you allow this app to use your location, it can make your measurements more accurate. This is completely optional. You can enable location in the Settings app, under 'Privacy'.";
    }
    
    [self.introLabel sizeToFit];
}

- (void) animateMap
{
    __block CLLocationDegrees longitude = self.mapView.centerCoordinate.longitude;
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
        while (true)
        {
            __block CLLocationCoordinate2D center = CLLocationCoordinate2DMake(latitude, longitude);
            dispatch_sync(dispatch_get_main_queue(), ^{
                [self.mapView setCenterCoordinate:center animated:NO];
            });
            
            longitude = longitude + .1;
            if (longitude >= 180.) longitude = -180.;
            
            [NSThread sleepForTimeInterval:0.0333];
        }
    });
}

#pragma mark - CLLocationManagerDelegate

- (void) locationManager:(CLLocationManager *)manager didChangeAuthorizationStatus:(CLAuthorizationStatus)status
{
    LOGME
    
    if(status == kCLAuthorizationStatusNotDetermined || !waitingForLocationAuthorization) return;
    if(status == kCLAuthorizationStatusAuthorized) [LOCATION_MANAGER startLocationUpdates];
    if(status == kCLAuthorizationStatusAuthorized || status == kCLAuthorizationStatusDenied || status == kCLAuthorizationStatusRestricted)
        [self gotoMeasurementResult];
    
    waitingForLocationAuthorization = false;
}

- (void) gotoMeasurementResult
{
    TMResultsVC* vc = [self.storyboard instantiateViewControllerWithIdentifier:@"Results"];
    vc.theMeasurement = self.theMeasurement;
    [self.navigationController pushViewController:vc animated:YES];
}

@end
