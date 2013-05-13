//
//  TMMapVC.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 12/11/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMMapVC.h"

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
    [LOCATION_MANAGER startLocationUpdates];
    
    _location = (TMLocation*)self.theMeasurement.location;

    [self setMapCenter];

    //add pin annotation
//    MKPointAnnotation *point = [[MKPointAnnotation alloc] init];
//    point.title = self.location.locationName;
//    [point setCoordinate:center];
//    [self.mapView addAnnotation:point];
    
    //make pin label appear
//    [self.mapView selectAnnotation:point animated:YES];
    
    self.locationTextField.text = self.location.locationName;
    self.addressLabel.text = self.location.address;
    
    self.locationTextField.delegate = self;
}

- (void) setMapCenter
{
    double zoomLevel = 1000; //meters

    if(self.location)
    {
        CLLocationCoordinate2D center = CLLocationCoordinate2DMake(self.location.latititude, self.location.longitude);
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
    NSLog(@"regionDidChangeAnimated");
    
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
    NSLog(@"Tap");
}

- (void)mapView:(MKMapView *)mapView annotationView:(MKAnnotationView *)view calloutAccessoryControlTapped:(UIControl *)control
{
    NSLog(@"Tap2");
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
        NSLog(@"Current location unknown");
    }
}

- (IBAction)handleSaveButton:(id)sender
{
    managedObjectContext = [DATA_MANAGER getManagedObjectContext];
    
    if(!self.theMeasurement.location)
    {
        _location = [TMLocation getNewLocation];
//        [self.location addMeasurementObject:self.theMeasurement];
        [self.location insertIntoDb];
        self.theMeasurement.location = self.location;
    }
    
    if (self.location.locationName != self.locationTextField.text || self.location.address != self.addressLabel.text)
    {
        self.location.locationName = self.locationTextField.text;
        self.location.address = self.addressLabel.text;
        self.location.syncPending = YES;
    }
        
    double newLat = self.mapView.centerCoordinate.latitude;
    double newLong = self.mapView.centerCoordinate.longitude;
    
    if (self.location.latititude != newLat || self.location.longitude != newLong) //if user moved the pin
    {
        self.location.latititude = newLat;
        self.location.longitude = newLong;
        self.location.accuracyInMeters = 0; //assume user put the pin in the exact right place
        self.location.syncPending = YES;
        self.location.dbid = 0; //indicates this is a new location
    }
    
    if (self.location.syncPending)
    {
        self.theMeasurement.syncPending = YES;
        [DATA_MANAGER saveContext];
        
        [TMAnalytics logEvent:@"Measurement.EditLocation"];
        
        [self uploadLocation];
    }
    
    [self.navigationController popViewControllerAnimated:YES];
}

- (void)uploadLocation
{
    __weak TMMapVC* weakSelf = self;
    if (self.location.dbid)
    {
        [self.location
         putToServer:^(int transId) { [weakSelf uploadMeasurement]; }
         onFailure:^(int statusCode) { }
         ];
    }
    else
    {
        [self.location
         postToServer:^(int transId)
         {
             weakSelf.theMeasurement.locationDbid = weakSelf.location.dbid;
             [DATA_MANAGER saveContext];
             [weakSelf uploadMeasurement];
         }
         onFailure:^(int statusCode) { }
         ];
    }
}

- (void)uploadMeasurement
{
    [self.theMeasurement
     putToServer:^(int transId) { }
     onFailure:^(int statusCode) { }
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
    CLGeocoder *geocoder = [[CLGeocoder alloc] init];
    
    __weak TMMapVC* weakSelf = self;
    [geocoder reverseGeocodeLocation:theLocation completionHandler:^(NSArray *placemarks, NSError *error) {
        if (error){
            NSLog(@"Geocode failed with error: %@", error);
            return;
        }
        if(placemarks && placemarks.count > 0)
        {
            CLPlacemark *topResult = [placemarks objectAtIndex:0];
            
            weakSelf.addressLabel.text = [topResult getFormattedAddress];
        }
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
