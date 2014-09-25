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
#import "RCVideoFrameProvider.h"

/**
 This is a video preview view that can be used as an alternative to a AVCaptureVideoPreviewLayer. You can pass individual
 video frames to this class, and it will display them as they are received. One advantage of this design is that you can 
 pass in video frames that you receive via [RCSensorFusionDelegate sensorFusionDidUpdateData:], which will ensure that the video
 and any augmented reality overlays you draw on top of it stay in sync.
 */
@interface RCVideoPreview : UIView <RCVideoFrameDelegate>

/**
 Pass CMSampleBufferRef video frames to this method.
 */
- (void) displaySampleBuffer:(CMSampleBufferRef)sampleBuffer;
/**
 Sets the orientation of the video preview.
 */
- (void) setVideoOrientation:(AVCaptureVideoOrientation)orientation;
/**
 Fades the image to white - 0 represents normal display, and 1 represents all white.
 */
- (void) setWhiteness:(float)whiteness;
/**
 Scales the video preview.
 */
- (void)setHorizontalScale:(float)hScale withVerticalScale:(float)vScale;

@end
