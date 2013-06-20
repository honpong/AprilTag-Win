//
//  TMViewController.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import <GLKit/GLKit.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <QuartzCore/QuartzCore.h>
#import <CoreImage/CoreImage.h>
#import <ImageIO/ImageIO.h>
#import "TMMeasurement.h"
#import "TMOptionsVC.h"
#import <CoreGraphics/CoreGraphics.h>
#import "TMMeasurement.h"
#import "TMResultsVC.h"
#import "TMLocation+TMLocationExt.h"
#import <CoreLocation/CoreLocation.h>
#import "RCCore/RCAVSessionManager.h"
#import "RCCore/RCMotionManager.h"
#import "RCCore/RCVideoCapManagerFactory.h"
#import "RCCore/RCLocationManager.h"
#import "TMDataManagerFactory.h"
#import "TMMeasurement+TMMeasurementExt.h"
#import "TMSyncable+TMSyncableSync.h"
#import "MBProgressHUD.h"
#import "TMServerOpsFactory.h"
#import "TMVideoPreview.h"
#import "RCCore/RCDistanceLabel.h"
#import "TMAugmentedRealityView.h"
#import "TM2DTapeView.h"
#import "RCCore/RCMeasurementManager.h"
#import "RCCore/RCMeasurementManagerDelegate.h"

@interface TMNewMeasurementVC : TMViewController <OptionsDelegate, RCMeasurementManagerDelegate>

- (void)handlePause;
- (void)handleResume;
- (IBAction)handleSaveButton:(id)sender;
- (void)startMeasuring;
- (void)stopMeasuring;

@property (weak, nonatomic) IBOutlet UIImageView *statusIcon;
@property (weak, nonatomic) IBOutlet RCDistanceLabel *distanceLabel;
@property (weak, nonatomic) IBOutlet UILabel *lblInstructions;
@property (nonatomic) IBOutlet TMAugmentedRealityView *arView;
@property (weak, nonatomic) IBOutlet UIView *instructionsBg;
@property (weak, nonatomic) IBOutlet TM2DTapeView *tapeView2D;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *btnSave;
@property (nonatomic) MeasurementType type;

@end
