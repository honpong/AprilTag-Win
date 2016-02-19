//
//  RCVideoPreview.h
//  RCCore
//
//  Created by Ben Hirashima on 7/24/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES2/glext.h>
#import <CoreVideo/CVOpenGLESTextureCache.h>
#import <AVFoundation/AVFoundation.h>
#import <GLKit/GLKit.h>
#import "RC3DK.h"

@protocol RCVideoPreviewDelegate <NSObject>

@optional

/** This will be called after drawing the background video, before the GL render buffer is presented.
 @param data The RCSensorFusionData object from [RCSensorFusionDelegate sensorFusionDidUpdateData:]
 @param cameraToScreen A matrix incorporating the perspective transform (as defined by data.cameraParameters), and any rotation necessary to display the scene in the current device orientation. The camera reference frame is right-handed, with positive x to the right, positive y down, and positive z pointing forward out of the camera.
 
 @note The transformation from the fixed world reference frame to the camera reference frame is specified by data.cameraTransformation. See RCSensorFusionData documentation for the specification of the world reference frame.
 */
- (void) renderWithSensorFusionData:(RCSensorFusionData *)data withCameraToScreenMatrix:(GLKMatrix4)cameraToScreen;

/** This will be called after the GL render buffer is presented.
 @param data The RCSensorFusionData object from [RCSensorFusionDelegate sensorFusionDidUpdateData:]
 @param cameraToScreen A matrix incorporating the perspective transform (as defined by data.cameraParameters), and any rotation necessary to display the scene in the current device orientation. The camera reference frame is right-handed, with positive x to the right, positive y down, and positive z pointing forward out of the camera.
 
 @note The transformation from the fixed world reference frame to the camera reference frame is specified by data.cameraTransformation. See RCSensorFusionData documentation for the specification of the world reference frame.
 */
- (void) didRenderWithSensorFusionData:(RCSensorFusionData *)data withCameraToScreenMatrix:(GLKMatrix4)cameraToScreen;

@end

/**
 This is an OpenGL video preview view that can be used as an alternative to a AVCaptureVideoPreviewLayer. You can pass individual
 video frames to this class, and it will display them as they are received. One advantage of this design is that you can 
 pass in video frames that you receive via [RCSensorFusionDelegate sensorFusionDidUpdateData:], which will ensure that the video
 and any augmented reality overlays you draw on top of it stay in sync.
 */
@interface RCVideoPreview : UIView <RCVideoFrameDelegate>

/**
 The delegate will have an opportunity to render on top of each frame.
 */
@property (nonatomic) id<RCVideoPreviewDelegate> delegate;

/**
 The orientation of the video, I.E. portrait/landscape. Defaults to portrait.
 */
@property (nonatomic) UIInterfaceOrientation orientation;

/**
 Pass CMSampleBufferRef video frames to this method. Will not cause the delegate's render method to be called.
 */
- (void)receiveVideoFrame:(CMSampleBufferRef)sampleBuffer;
/**
 Displays the image in data.sampleBuffer. Use this method to trigger the delegate's render method, using the motion and camera data specified in this update.
 */
- (void) displaySensorFusionData:(RCSensorFusionData *)data;
/**
 Fades the image to white - 0 represents normal display, and 1 represents all white.
 */
- (void) setWhiteness:(float)whiteness;
/**
 Scales the video preview.
 */
- (void)setHorizontalScale:(float)hScale withVerticalScale:(float)vScale;

@end
