//
//  TMViewController.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <QuartzCore/QuartzCore.h>
#import <CoreImage/CoreImage.h>
#import <ImageIO/ImageIO.h>
#import "EAGLView.h"
#import <RCCore/RCMotionCap.h>
#import <RCCore/RCVideoCap.h>
#import "RCCore/cor.h"
#import "TMMeasurement.h"
#import "TMOptionsVC.h"

@protocol OptionsDelegate;
@class TMAppDelegate;

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

    NSOperationQueue *queueAll;
    
    TMMeasurement *newMeasurement;
    
    bool useLocation;
    
    TMAppDelegate *appDel;
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
@property (readonly, strong, nonatomic) NSManagedObjectContext *managedObjectContext;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *btnSave;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *locationButton;
@property (nonatomic) MeasurementType type;

@property (strong, nonatomic) EAGLContext *context;

@end
