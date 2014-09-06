//
//  TMLocationIntro.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 8/28/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "TMLocationIntro.h"
#import "TMResultsVC.h"
#import "RCCalibration1.h"
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
    NSNumber* timestamp = [NSNumber numberWithDouble:[[NSDate date] timeIntervalSince1970]];
    [NSUserDefaults.standardUserDefaults setObject:timestamp forKey:PREF_LOCATION_NAG_TIMESTAMP];
    
    if(![CLLocationManager locationServicesEnabled] || [CLLocationManager authorizationStatus] == kCLAuthorizationStatusNotDetermined)
    {
        waitingForLocationAuthorization = true;
        LOCATION_MANAGER.delegate = self;
        [LOCATION_MANAGER startLocationUpdates]; // will show dialog asking user to authorize location. gotoCalibration triggered by didChangeAuthorizationStatus delegate function
    }
    else
    {
        [self gotoNextScreen];
    }
}

- (IBAction)handleLaterButton:(id)sender
{
    NSNumber* timestamp = [NSNumber numberWithDouble:[[NSDate date] timeIntervalSince1970]];
    [NSUserDefaults.standardUserDefaults setObject:timestamp forKey:PREF_LOCATION_NAG_TIMESTAMP];
    if (!waitingForLocationAuthorization) [self gotoNextScreen];
}

- (IBAction)handleNeverButton:(id)sender
{
    [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_SHOW_LOCATION_EXPLANATION];
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
        [self.laterButton setTitle:@"Remind me later" forState:UIControlStateNormal];
        [self.neverButton setTitle:@"Skip" forState:UIControlStateNormal];
        self.introLabel.text = @"I see that you have location services disabled. If you allow this app to use your location, it can make your measurements more accurate by adjusting for differences in gravity across the earth. This is optional, but recommended";
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
    if(status == kCLAuthorizationStatusAuthorized)
    {
        [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_SHOW_LOCATION_EXPLANATION];
        [LOCATION_MANAGER startLocationUpdates];
        LOCATION_MANAGER.delegate = nil;
    }
    else if(status == kCLAuthorizationStatusDenied || status == kCLAuthorizationStatusRestricted)
    {
        [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_SHOW_LOCATION_EXPLANATION];
    }
    if([CLLocationManager locationServicesEnabled] && (status == kCLAuthorizationStatusAuthorized || status == kCLAuthorizationStatusDenied || status == kCLAuthorizationStatusRestricted))
    {
        if (status != kCLAuthorizationStatusAuthorized) [LOCATION_MANAGER stopLocationUpdates];
        [self gotoNextScreen];
        waitingForLocationAuthorization = false;
    }
}

- (void) gotoNextScreen
{
    if ([self.presentingViewController isKindOfClass:[TMIntroScreen class]])
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
    TMResultsVC* vc = [self.storyboard instantiateViewControllerWithIdentifier:@"Results"];
    vc.theMeasurement = self.theMeasurement;
    [self.navigationController pushViewController:vc animated:YES];
}

- (void) gotoCalibration
{
    RCCalibration1 * calibration1 = [RCCalibration1 instantiateViewController];
    calibration1.calibrationDelegate = self.calibrationDelegate;
    calibration1.sensorDelegate = self.sensorDelegate;
    calibration1.modalPresentationStyle = UIModalPresentationFullScreen;
    [self presentViewController:calibration1 animated:YES completion:nil];
}

@end
