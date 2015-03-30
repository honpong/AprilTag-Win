//
//  MPLocationIntro.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MapKit/MapKit.h>

@protocol RCCalibrationDelegate, RCSensorDelegate;

@protocol MPLocationIntroDelegate <NSObject>

- (void) nextButtonTapped;

@end

@interface MPLocationIntro : UIViewController

@property (weak, nonatomic) id<MPLocationIntroDelegate> delegate;
@property (weak, nonatomic) IBOutlet UIButton *nextButton;
@property (weak, nonatomic) IBOutlet UIButton *laterButton;
@property (weak, nonatomic) IBOutlet UIButton *neverButton;
@property (weak, nonatomic) IBOutlet UILabel *introLabel;
@property (weak, nonatomic) IBOutlet MKMapView *mapView;
@property (weak, nonatomic) id<RCCalibrationDelegate> calibrationDelegate;

- (IBAction)handleNextButton:(id)sender;
- (IBAction)handleLaterButton:(id)sender;
- (IBAction)handleNeverButton:(id)sender;

@end
