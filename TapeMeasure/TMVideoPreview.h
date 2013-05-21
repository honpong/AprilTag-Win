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
    GLuint passThroughProgram;
}

- (void)displayPixelBuffer:(CVImageBufferRef)pixelBuffer;
- (void)setTransformFromCurrentVideoOrientationToOrientation:(AVCaptureVideoOrientation)orientation;

@end

