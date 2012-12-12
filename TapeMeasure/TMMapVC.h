//
//  TMMapVC.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 12/11/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MapKit/MapKit.h>

@class TMLocation;
@class MKMapView;

@interface TMMapVC : UIViewController <MKMapViewDelegate, UITextFieldDelegate>

@property (strong, nonatomic) TMLocation *location;
@property (weak, nonatomic) IBOutlet MKMapView *mapView;
@property (weak, nonatomic) IBOutlet UITextField *locationTextField;
@property (weak, nonatomic) IBOutlet UIButton *centerButton;
@property (weak, nonatomic) IBOutlet UILabel *addressLabel;

- (IBAction)handleCenterButton:(id)sender;
- (IBAction)handleSaveButton:(id)sender;
@end
