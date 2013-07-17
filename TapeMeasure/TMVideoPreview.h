//
//  TMAugmentedRealityView.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES2/glext.h>
#import <CoreVideo/CVOpenGLESTextureCache.h>
#import <AVFoundation/AVFoundation.h>
#import <RC3DK/RC3DK.h>
#import "TMOpenGLManagerFactory.h"

@interface TMVideoPreview : UIView <RCVideoFrameDelegate>

- (bool)beginFrame;
- (void)endFrame;
- (void)displayPixelBuffer:(CVImageBufferRef)pixelBuffer;
- (void)displayTapeWithMeasurement:(RCTranslation *)measurement withStart:(RCPoint *)start withViewTransform:(RCTransformation *)viewTransform withCameraParameters:(RCCameraParameters *)cameraParameters;
- (void)setTransformFromCurrentVideoOrientationToOrientation:(AVCaptureVideoOrientation)orientation;

@end

