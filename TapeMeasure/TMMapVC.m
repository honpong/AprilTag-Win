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

- (void)viewWillAppear:(BOOL)animated
{
    [self.navigationController setToolbarHidden:YES animated:animated];
    [super viewWillAppear:animated];
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
        NSEntityDescription *entity = [NSEntityDescription entityForName:ENTITY_STRING_LOCATION inManagedObjectContext:managedObjectContext];
        _location = (TMLocation*)[[NSManagedObject alloc] initWithEntity:entity insertIntoManagedObjectContext:managedObjectContext];
        [self.location addMeasurementObject:self.theMeasurement];
    }
    
    double newLat = self.mapView.centerCoordinate.latitude;
    double newLong = self.mapView.centerCoordinate.longitude;
    
    self.location.locationName = self.locationTextField.text;
    self.location.address = self.addressLabel.text;
    
    if (self.location.latititude != newLat || self.location.longitude != newLong) //if user moved the pin
    {
        self.location.latititude = newLat;
        self.location.longitude = newLong;
        self.location.accuracyInMeters = 0; //assume user put the pin in the exact right place
    }
        
    NSError *error;
    [managedObjectContext save:&error]; //TODO: Handle save error
    
    if(error) NSLog(@"Error saving location");
    
    [self.navigationController popViewControllerAnimated:YES];
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
    
    [geocoder reverseGeocodeLocation:theLocation completionHandler:^(NSArray *placemarks, NSError *error) {
        if (error){
            NSLog(@"Geocode failed with error: %@", error);
            return;
        }
        if(placemarks && placemarks.count > 0)
        {
            CLPlacemark *topResult = [placemarks objectAtIndex:0];
            
            self.addressLabel.text = [topResult getFormattedAddress];
        }
    }];
}
@end
