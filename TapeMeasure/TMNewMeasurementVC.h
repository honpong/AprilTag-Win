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
#import "EAGLView.h"
#import "TMMeasurement.h"
#import "TMOptionsVC.h"
#import <CoreGraphics/CoreGraphics.h>
#import "TMMeasurement.h"
#import "TMResultsVC.h"
#import "TMDistanceFormatter.h"
#import "TMOptionsVC.h"
#import "TMLocation.h"
#import <CoreLocation/CoreLocation.h>
#import "RCCore/RCAVSessionManagerFactory.h"
#import "RCCore/RCCorvisManagerFactory.h"
#import "RCCore/RCMotionCapManagerFactory.h"
#import "RCCore/RCVideoCapManagerFactory.h"
#import "RCCore/RCLocationManagerFactory.h"
#import "TMDataManagerFactory.h"

@protocol OptionsDelegate;

@interface TMNewMeasurementVC : UIViewController <AVCaptureVideoDataOutputSampleBufferDelegate, OptionsDelegate>
{
@private
	float distanceMeasured;
	NSTimer *repeatingTimer;
	bool isMeasuring;
	
	float lastAccel;
	int lastBump;
		
    CIContext *ciContext;
    GLuint _renderBuffer;
    EAGLContext *context;

    NSOperationQueue *queueAll;
    
    TMMeasurement *newMeasurement;
    
    bool useLocation;
}

- (IBAction)startRepeatingTimer:sender;
- (IBAction)handleButtonTap:(id)sender;
- (void)handlePause;
- (void)handleResume;
- (IBAction)handlePageCurl:(id)sender;
- (IBAction)handleSaveButton:(id)sender;
- (IBAction)handleLocationButton:(id)sender;

@property (weak, nonatomic) IBOutlet UILabel *lblDistance;
@property (weak, nonatomic) IBOutlet UILabel *lblInstructions;
@property (weak, nonatomic) IBOutlet EAGLView *videoPreviewView;
@property (weak, nonatomic) IBOutlet UIImageView *imageView;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *btnPageCurl;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *btnBegin;
@property (weak, nonatomic) IBOutlet UIView *instructionsBg;
@property (weak, nonatomic) IBOutlet UIView *distanceBg;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *btnSave;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *locationButton;
@property (nonatomic) MeasurementType type;

@end
