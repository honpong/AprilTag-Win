//
//  TMLocationIntro.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 8/28/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "TMMeasurement+TMMeasurementExt.h"
@import MapKit;

@protocol RCCalibrationDelegate, RCSensorDelegate;

@protocol TMLocationIntroDelegate <NSObject>

- (void) nextButtonTapped;

@end

@interface TMLocationIntro : UIViewController

@property (weak, nonatomic) id<TMLocationIntroDelegate> delegate;
@property (weak, nonatomic) IBOutlet UIButton *nextButton;
@property (weak, nonatomic) IBOutlet UIButton *laterButton;
@property (weak, nonatomic) IBOutlet UIButton *neverButton;
@property (weak, nonatomic) IBOutlet UILabel *introLabel;
@property (weak, nonatomic) IBOutlet MKMapView *mapView;
@property (nonatomic) TMMeasurement* theMeasurement;
@property (weak, nonatomic) id<RCCalibrationDelegate> calibrationDelegate;

- (IBAction)handleNextButton:(id)sender;
- (IBAction)handleLaterButton:(id)sender;
- (IBAction)handleNeverButton:(id)sender;

@end
