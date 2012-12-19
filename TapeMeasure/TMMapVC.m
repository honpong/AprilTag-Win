//
//  TMMapVC.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 12/11/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMMapVC.h"
#import "TMMeasurement.h"
#import "TMLocation.h"
#import <UIKit/UIKit.h>
#import <MapKit/MapKit.h>
#import "TMAppDelegate.h"

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
    appDel = (TMAppDelegate*)[[UIApplication sharedApplication] delegate];
    [appDel startLocationUpdates];
    
    _location = (TMLocation*)self.theMeasurement.location;
	
    double zoomLevel = 1000; //meters
    
    if(self.location)
    {
        CLLocationCoordinate2D center = CLLocationCoordinate2DMake(self.location.latititude.doubleValue, self.location.longitude.doubleValue);
        [self.mapView setRegion:MKCoordinateRegionMakeWithDistance(center, zoomLevel, zoomLevel) animated:YES];
    }
    else if(appDel.location)
    {
        CLLocationCoordinate2D center = CLLocationCoordinate2DMake(appDel.location.coordinate.latitude, appDel.location.coordinate.longitude);
        [self.mapView setRegion:MKCoordinateRegionMakeWithDistance(center, zoomLevel, zoomLevel) animated:YES];
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
    
    appDel = (TMAppDelegate*)[[UIApplication sharedApplication] delegate];
    
    //if new location is close to current location, show active location icon (purple arrow)
    double dist = (appDel.location.horizontalAccuracy > 65 && appDel.location.horizontalAccuracy < 100) ? appDel.location.horizontalAccuracy : 65;
    
    if(newLocation && appDel.location && [newLocation distanceFromLocation:appDel.location] < dist)
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
    if(appDel.locationAddress)
    {
        CLLocationCoordinate2D center = CLLocationCoordinate2DMake(appDel.location.coordinate.latitude, appDel.location.coordinate.longitude);
        [self.mapView setRegion:MKCoordinateRegionMake(center, self.mapView.region.span) animated:YES];
    }
    else
    {
        NSLog(@"Current location unknown");
    }
}

- (IBAction)handleSaveButton:(id)sender
{
    if(!self.theMeasurement.location)
    {
        NSEntityDescription *entity = [NSEntityDescription entityForName:ENTITY_LOCATION inManagedObjectContext:appDel.managedObjectContext];
        _location = (TMLocation*)[[NSManagedObject alloc] initWithEntity:entity insertIntoManagedObjectContext:appDel.managedObjectContext];
        [self.location addMeasurementObject:self.theMeasurement];
    }
    
    self.location.locationName = self.locationTextField.text;
    self.location.address = self.addressLabel.text;
    self.location.latititude = [NSNumber numberWithDouble:self.mapView.centerCoordinate.latitude];
    self.location.longitude = [NSNumber numberWithDouble:self.mapView.centerCoordinate.longitude];
    
    NSError *error;
    [appDel.managedObjectContext save:&error]; //TODO: Handle save error
    
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
            
            self.addressLabel.text = [NSString stringWithFormat:@"%@ %@, %@, %@",
                                [topResult subThoroughfare],[topResult thoroughfare],
                                [topResult locality], [topResult administrativeArea]];
        }
    }];
}
@end
