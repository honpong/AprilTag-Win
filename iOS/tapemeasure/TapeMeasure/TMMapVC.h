//
//  TMMapVC.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 12/11/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import <MapKit/MapKit.h>
#import "TMMeasurement+TMMeasurementExt.h"
#import "TMMeasurement+TMMeasurementSync.h"
#import "TMLocation+TMLocationExt.h"
#import "TMLocation+TMLocationSync.h"
#import "TMSyncable+TMSyncableExt.h"
#import "TMSyncable+TMSyncableSync.h"
#import "TMAppDelegate.h"
#import "TMDataManagerFactory.h"
#import "TMViewController.h"

@interface TMMapVC : TMViewController <MKMapViewDelegate, UITextFieldDelegate>

@property (strong, nonatomic) TMMeasurement *theMeasurement;
@property (weak, nonatomic) IBOutlet MKMapView *mapView;
@property (weak, nonatomic) IBOutlet UITextField *locationTextField;
@property (weak, nonatomic) IBOutlet UIButton *centerButton;
@property (weak, nonatomic) IBOutlet UILabel *addressLabel;
@property (weak, nonatomic) IBOutlet UIImageView *mapPin;

- (IBAction)handleCenterButton:(id)sender;
- (IBAction)handleSaveButton:(id)sender;
@end
