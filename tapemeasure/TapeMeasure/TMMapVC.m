//
//  TMMapVC.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 12/11/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMMapVC.h"
#import <RCCore/RCCore.h>
#import "RCCore/RCGeocoder.h"

@interface TMMapVC ()

@end

@implementation TMMapVC

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
    
    self.mapView.delegate = self;
    
    //make sure we have a fresh location to work with
    if([LOCATION_MANAGER isLocationExplicitlyAllowed]) [LOCATION_MANAGER startLocationUpdates];
    
    [self setMapCenter];

    //add pin annotation
//    MKPointAnnotation *point = [[MKPointAnnotation alloc] init];
//    point.title = self.theMeasurement.location.locationName;
//    [point setCoordinate:center];
//    [self.mapView addAnnotation:point];
    
    //make pin label appear
//    [self.mapView selectAnnotation:point animated:YES];
    
    self.locationTextField.text = self.theMeasurement.location.locationName;
    self.addressLabel.text = self.theMeasurement.location.address;
    
    self.locationTextField.delegate = self;
}

- (void) setMapCenter
{
    double zoomLevel = 1000; //meters

    if(self.theMeasurement.location)
    {
        CLLocationCoordinate2D center = CLLocationCoordinate2DMake(self.theMeasurement.location.latititude, self.theMeasurement.location.longitude);
        [self.mapView setRegion:MKCoordinateRegionMakeWithDistance(center, zoomLevel, zoomLevel) animated:YES];
    }
    else
    {
        CLLocation *storedLocation = [LOCATION_MANAGER getStoredLocation];

        if(storedLocation)
        {
            CLLocationCoordinate2D center = CLLocationCoordinate2DMake(storedLocation.coordinate.latitude, storedLocation.coordinate.longitude);
            [self.mapView setRegion:MKCoordinateRegionMakeWithDistance(center, zoomLevel, zoomLevel) animated:YES];
        }
    }
    
    [self setPinLocation];
}

- (void) setPinLocation
{
    int pinBaseOffsetX = 16;
    int pinBaseOffsetY = 30;
    
    int pinX = (self.mapView.frame.size.width / 2) - pinBaseOffsetX;
    int pinY = (self.mapView.frame.size.height / 2) - pinBaseOffsetY;
    
    CGRect frame = CGRectMake(pinX, pinY, self.mapPin.frame.size.width, self.mapPin.frame.size.height);
    [self.mapPin setFrame:frame];
    
    self.mapPin.hidden = NO;
}

- (void)viewWillAppear:(BOOL)animated
{
    [self.navigationController setToolbarHidden:YES animated:animated];
    [super viewWillAppear:animated];
}

- (void)viewDidAppear:(BOOL)animated
{
    [TMAnalytics logEvent:@"View.Map"];
    [super viewDidAppear:animated];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)viewDidUnload {
    [self setMapView:nil];
    [self setLocationTextField:nil];
    [self setCenterButton:nil];
    [self setAddressLabel:nil];
    [self setMapPin:nil];
    [super viewDidUnload];
}

- (MKAnnotationView *)mapView:(MKMapView *)mapView viewForAnnotation:(id <MKAnnotation>)annotation
{
    // If it's the user location, just return nil.
    if ([annotation isKindOfClass:[MKUserLocation class]]) return nil;
     
    // Handle any custom annotations.
    if ([annotation isKindOfClass:[MKPointAnnotation class]])
    {
        // Try to dequeue an existing pin view first.
        MKPinAnnotationView* pinView = (MKPinAnnotationView*)[mapView dequeueReusableAnnotationViewWithIdentifier:@"CustomPinAnnotationView"];
        
        if (!pinView)
        {
            // If an existing pin view was not available, create one.
            pinView = [[MKPinAnnotationView alloc] initWithAnnotation:annotation reuseIdentifier:@"CustomPinAnnotation"];
            pinView.pinColor = MKPinAnnotationColorRed;
            pinView.animatesDrop = YES;
            pinView.canShowCallout = YES;
            
            // Add a detail disclosure button to the callout.
            UIButton* rightButton = [UIButton buttonWithType: UIButtonTypeDetailDisclosure];
            [rightButton addTarget:self action:@selector(handleDetailDisclosureTap) forControlEvents:UIControlEventTouchUpInside];
            
            pinView.rightCalloutAccessoryView = rightButton;
            
        }
        else
            pinView.annotation = annotation;
        
        return pinView;
    }
    return nil;    
}

- (void)mapView:(MKMapView *)mapView regionDidChangeAnimated:(BOOL)animated
{
    LOGME
    
    CLLocation *newLocation = [[CLLocation alloc] initWithLatitude:mapView.centerCoordinate.latitude longitude:mapView.centerCoordinate.longitude];
    
    CLLocation *storedLocation = [LOCATION_MANAGER getStoredLocation];
    
    //if new location is close to current location, show active location icon (purple arrow)
    double dist = (storedLocation.horizontalAccuracy > 65 && storedLocation.horizontalAccuracy < 100) ? storedLocation.horizontalAccuracy : 65;
    
    if(newLocation && storedLocation && [newLocation distanceFromLocation:storedLocation] < dist)
    {
        [self.centerButton.imageView setImage:[UIImage imageNamed:@"ComposeSheetLocationArrowActive.png"]];
    }
    else
    {
        [self.centerButton.imageView setImage:[UIImage imageNamed:@"ComposeSheetLocationArrow.png"]];
    }
    
    [self reverseGeocode:newLocation];
}

- (void)handleDetailDisclosureTap
{
    DLog(@"Tap");
}

- (void)mapView:(MKMapView *)mapView annotationView:(MKAnnotationView *)view calloutAccessoryControlTapped:(UIControl *)control
{
    DLog(@"Tap2");
}

- (IBAction)handleCenterButton:(id)sender
{
    [self centerMapOnCurrentLocation];
}

- (void)centerMapOnCurrentLocation
{
    CLLocation *storedLocation = [LOCATION_MANAGER getStoredLocation];
    
    if(storedLocation)
    {
        CLLocationCoordinate2D center = CLLocationCoordinate2DMake(storedLocation.coordinate.latitude, storedLocation.coordinate.longitude);
        [self.mapView setRegion:MKCoordinateRegionMake(center, self.mapView.region.span) animated:YES];
    }
    else
    {
        DLog(@"Current location unknown");
    }
}

- (IBAction)handleSaveButton:(id)sender
{
    if(!self.theMeasurement.location)
    {
        TMLocation* newLocation = [TMLocation getNewLocation];
        [newLocation insertIntoDb];
        [self.theMeasurement setLocation:newLocation];
    }
    
    if (self.theMeasurement.location.locationName != self.locationTextField.text || self.theMeasurement.location.address != self.addressLabel.text)
    {
        self.theMeasurement.location.locationName = self.locationTextField.text;
        self.theMeasurement.location.address = self.addressLabel.text;
        self.theMeasurement.location.syncPending = YES;
    }
        
    double newLat = self.mapView.centerCoordinate.latitude;
    double newLong = self.mapView.centerCoordinate.longitude;
    
    if (self.theMeasurement.location.latititude != newLat || self.theMeasurement.location.longitude != newLong) //if user moved the pin
    {
        self.theMeasurement.location.latititude = newLat;
        self.theMeasurement.location.longitude = newLong;
        self.theMeasurement.location.accuracyInMeters = 0; //assume user put the pin in the exact right place
        self.theMeasurement.location.syncPending = YES;
        self.theMeasurement.location.dbid = 0; //indicates this is a new location
    }
    
    if (self.theMeasurement.location.syncPending)
    {
        self.theMeasurement.syncPending = YES;
        [DATA_MANAGER saveContext];
        
        [TMAnalytics logEvent:@"Measurement.EditLocation"];
        
//        [self uploadLocation];
    }
    
    [self.navigationController popViewControllerAnimated:YES];
}

- (void)uploadLocation
{
    __weak TMMapVC* weakSelf = self;
    if (self.theMeasurement.location.dbid)
    {
        [self.theMeasurement.location
         putToServer:^(NSInteger transId) { [weakSelf uploadMeasurement]; }
         onFailure:^(NSInteger statusCode) { }
         ];
    }
    else
    {
        [self.theMeasurement.location
         postToServer:^(NSInteger transId)
         {
             weakSelf.theMeasurement.locationDbid = weakSelf.theMeasurement.location.dbid;
             [DATA_MANAGER saveContext];
             [weakSelf uploadMeasurement];
         }
         onFailure:^(NSInteger statusCode) { }
         ];
    }
}

- (void)uploadMeasurement
{
    [self.theMeasurement
     putToServer:^(NSInteger transId) { }
     onFailure:^(NSInteger statusCode) { }
     ];
}

- (IBAction)handleKeyboardDone:(id)sender {
    //no need to do anything here
}

//hides keyboard when done button pressed
- (BOOL)textFieldShouldReturn:(UITextField *)textField {
    [textField resignFirstResponder];
    return NO;
}

- (void)reverseGeocode:(CLLocation*)theLocation
{
    __weak TMMapVC* weakSelf = self;
    [RCGeocoder reverseGeocodeLocation:theLocation withCompletionBlock:^(NSString *address, NSError *error)
     {
         weakSelf.addressLabel.text = address;
     }];
}

- (void) willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
    self.mapPin.hidden = YES;
}

- (void) didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation
{
    //prevents name field from being partially covered by navigation bar on orientation change
    [self.navigationController.view layoutSubviews];
    [self setPinLocation];
}
@end
