//
//  TMVideoPreview.h
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
#import "RCCore/RCVideoCapManagerFactory.h"

@interface TMVideoPreview : UIView
{
	int renderBufferWidth;
	int renderBufferHeight;
    
	CVOpenGLESTextureCacheRef videoTextureCache;
    
	EAGLContext* oglContext;
	GLuint frameBufferHandle;
	GLuint colorBufferHandle;
    GLuint yuvTextureProgram, tapeProgram;
}

- (void)beginFrame;
- (void)endFrame;
- (void)displayPixelBuffer:(CVImageBufferRef)pixelBuffer;
- (void)displayTapeWithMeasurement:(float[3])measurement withStart:(float[3])start withCameraMatrix:(float[16])camera withFocalCenterRadial:(float[5])focalCenterRadial;
- (void)setTransformFromCurrentVideoOrientationToOrientation:(AVCaptureVideoOrientation)orientation;

@end

