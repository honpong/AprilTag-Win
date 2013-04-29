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
#import "RCCore/RCDistanceFormatter.h"
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
#import "ARView.h"

@interface TMNewMeasurementVC : TMViewController <AVCaptureVideoDataOutputSampleBufferDelegate, OptionsDelegate>
{
@private
	float distanceMeasured;
	NSTimer *repeatingTimer;
	
	float lastAccel;
	int lastBump;
		
    CIContext *ciContext;
    GLuint _renderBuffer;
    EAGLContext *context;

    NSOperationQueue *queueAll;
    
    TMMeasurement *newMeasurement;
    
    BOOL useLocation;
    BOOL locationAuthorized;
    crosshairsLayerDelegate *crosshairsDelegate;
    targetLayerDelegate *targetDelegate;
    CALayer *targetLayer, *crosshairsLayer;
}

- (IBAction)handleButtonTap:(id)sender;
- (void)handlePause;
- (void)handleResume;
- (IBAction)handlePageCurl:(id)sender;
- (IBAction)handleSaveButton:(id)sender;
- (IBAction)handleLocationButton:(id)sender;
- (void)startMeasuring;
- (void)stopMeasuring;
void TMNewMeasurementVCUpdateMeasurement(void *self, float x, float stdx, float y, float stdy, float z, float stdz, float path, float stdpath);

@property (weak, nonatomic) IBOutlet ARView *arView;
@property (weak, nonatomic) IBOutlet UILabel *lblDistance;
@property (weak, nonatomic) IBOutlet UILabel *lblInstructions;
@property (weak, nonatomic) IBOutlet UIView *videoPreviewView;
@property (weak, nonatomic) IBOutlet UIImageView *imageView;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *btnPageCurl;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *btnBegin;
@property (weak, nonatomic) IBOutlet UIView *instructionsBg;
@property (weak, nonatomic) IBOutlet UIView *distanceBg;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *btnSave;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *locationButton;
@property (nonatomic) MeasurementType type;
@property BOOL isCapturingData;
@property BOOL isMeasuring;
@property BOOL isProcessingData;
@property BOOL isMeasurementComplete;
@property BOOL isMeasurementCanceled;

@end
