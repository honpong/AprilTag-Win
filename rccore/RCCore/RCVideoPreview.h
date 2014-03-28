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
#import "RCOpenGLManagerFactory.h"

@interface RCVideoPreview : UIView
{
    // these are accessible to subclasses
    size_t textureWidth;
    size_t textureHeight;
    CGRect normalizedSamplingRect;
}

- (void) displaySampleBuffer:(CMSampleBufferRef)sampleBuffer;
- (void) setTransformFromCurrentVideoOrientationToOrientation:(AVCaptureVideoOrientation)orientation;

@end
