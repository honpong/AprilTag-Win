//
//  TMViewController.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreMotion/CoreMotion.h>
#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <QuartzCore/QuartzCore.h>
#import <CoreImage/CoreImage.h>
#import <ImageIO/ImageIO.h>
#import "EAGLView.h"

@interface TMViewController : UIViewController <AVCaptureVideoDataOutputSampleBufferDelegate>
{
@private
	int distanceMeasured;
	NSTimer *repeatingTimer;
	bool isMeasuring;
	
	float lastAccel;
	int lastBump;
	
	CMMotionManager *motionMan;
	
	AVCaptureSession *session;
	
    CIContext *ciContext;
    GLuint _renderBuffer;

    NSOperationQueue *queueAll;
}

- (IBAction)startRepeatingTimer:sender;
- (IBAction)handleButtonTap:(id)sender;
- (void)handlePause;
- (void)handleResume;

@property (weak, nonatomic) IBOutlet UILabel *lblDistance;
@property (weak, nonatomic) IBOutlet UILabel *lblInstructions;
@property (weak, nonatomic) IBOutlet UIButton *btnBegin;
@property (weak, nonatomic) IBOutlet EAGLView *videoPreviewView;

@property (strong, nonatomic) EAGLContext *context;

@end
