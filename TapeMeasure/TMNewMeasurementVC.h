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
#import "RCCore/RCAVSessionManagerFactory.h"
#import "RCCore/RCCorvisManagerFactory.h"
#import "RCCore/RCMotionCapManagerFactory.h"
#import "RCCore/RCVideoCapManagerFactory.h"
#import "RCCore/RCLocationManagerFactory.h"
#import "TMDataManagerFactory.h"
#import "TMMeasurement+TMMeasurementExt.h"
#import "TMSyncable+TMSyncableSync.h"
#import "MBProgressHUD.h"
#import "TMServerOpsFactory.h"
#import "RCCore/RCConstants.h"
#import "TMTickMarksLayerDelegate.h"
#import "TMVideoPreview.h"
#import "RCCore/RCDistanceLabel.h"
#import "TMAugmentedRealityView.h"
#import "TM2DTapeView.h"

@interface TMNewMeasurementVC : TMViewController <OptionsDelegate>

- (void)handlePause;
- (void)handleResume;
- (IBAction)handleSaveButton:(id)sender;
- (void)startMeasuring;
- (void)stopMeasuring;
void TMNewMeasurementVCUpdateMeasurement(void *self, float x, float stdx, float y, float stdy, float z, float stdz, float path, float stdpath, float rx, float stdrx, float ry, float stdry, float rz, float stdrz);

@property (weak, nonatomic) IBOutlet UIImageView *statusIcon;
@property (weak, nonatomic) IBOutlet RCDistanceLabel *distanceLabel;
@property (weak, nonatomic) IBOutlet UILabel *lblInstructions;
@property (nonatomic) IBOutlet TMAugmentedRealityView *arView;
@property (weak, nonatomic) IBOutlet UIView *instructionsBg;
@property (weak, nonatomic) IBOutlet TM2DTapeView *tapeView2D;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *btnSave;
@property (nonatomic) MeasurementType type;

@end
