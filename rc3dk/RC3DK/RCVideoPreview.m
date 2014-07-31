//
//  RCVideoPreview.m
//  RCCore
//
//  Created by Ben Hirashima on 7/24/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCVideoPreview.h"
#include "ShaderUtilities.h"
#import <CoreMedia/CoreMedia.h>
#import "RCOpenGLManagerFactory.h"

@implementation RCVideoPreview
{
    float xDisp, yDisp, zDisp;
    
    int renderBufferWidth;
	int renderBufferHeight;
    
	CVOpenGLESTextureCacheRef videoTextureCache;
    
	GLuint frameBufferHandle;
    GLuint sampleFramebuffer;
	GLuint colorBufferHandle;
    GLuint sampleColorBuffer;
    
    CVOpenGLESTextureRef lumaTexture;
    CVOpenGLESTextureRef chromaTexture;
    float textureScale;
    
    CMBufferQueueRef previewBufferQueue;
    
    BOOL didReceiveFirstFrame;
}

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        [self initialize];
    }
    return self;
}

-(id)initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame])
    {
		[self initialize];
    }
    return self;
}

- (void) initialize
{
    OPENGL_MANAGER;
    
    didReceiveFirstFrame = NO;
    
    textureWidth = 640;
    textureHeight = 480;
    textureScale = 1.;
    
    // Use 2x scale factor on Retina displays.
    self.contentScaleFactor = [[UIScreen mainScreen] scale];
    
    // Initialize OpenGL ES 2
    CAEAGLLayer* eaglLayer = (CAEAGLLayer *)self.layer;
    eaglLayer.opaque = YES;
    eaglLayer.drawableProperties = @{kEAGLDrawablePropertyRetainedBacking: @NO,
                                     kEAGLDrawablePropertyColorFormat: kEAGLColorFormatRGBA8};
    
    normalizedSamplingRect = CGRectMake(0., 0., 1., 1.);
    
    xDisp = yDisp = zDisp = 0;
    
    OSStatus err = CMBufferQueueCreate(kCFAllocatorDefault, 1, CMBufferQueueGetCallbacksForUnsortedSampleBuffers(), &previewBufferQueue);
    if (err) DLog(@"ERROR creating CMBufferQueue");
}

+ (Class)layerClass
{
    return [CAEAGLLayer class];
}

- (void) animateOpen
{
    // grow video from horizontal line to full screen
    [UIView animateWithDuration: .2
                          delay: .2
                        options: UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         self.frame = self.superview.frame;
                     }
                     completion:^(BOOL finished){
                         
                     }];
}

- (void) animateClosed:(void(^)(BOOL finished))completion
{
    // shrink video into a horizontal line
    [UIView animateWithDuration: .2
                          delay: 0
                        options: UIViewAnimationOptionCurveEaseOut
                     animations:^{
                         self.frame = CGRectMake(0, self.superview.frame.size.height / 2, self.superview.frame.size.width, 2.);
                     }
                     completion:^(BOOL finished){
                         
                         // line shrinks to dot in center
                         [UIView animateWithDuration: .1
                                               delay: 0
                                             options: UIViewAnimationOptionCurveEaseOut
                                          animations:^{
                                              self.frame = CGRectMake(self.superview.frame.size.width / 2, self.superview.frame.size.height / 2, 2., 2.);
                                          }
                                          completion:^(BOOL finished){
                                              completion(finished);
                                          }];
                         
                         
                     }];
}

- (void) displaySampleBuffer:(CMSampleBufferRef)sampleBuffer
{
    sampleBuffer = (CMSampleBufferRef)CFRetain(sampleBuffer);
    
    // Enqueue it for preview.  This is a shallow queue, so if image processing is taking too long,
    // we'll drop this frame for preview (this keeps preview latency low).
    OSStatus err = CMBufferQueueEnqueue(previewBufferQueue, sampleBuffer);
    if ( !err )
    {
        __weak RCVideoPreview* weakSelf = self;
        
        dispatch_async(dispatch_get_main_queue(), ^{
            CMSampleBufferRef sbuf = (CMSampleBufferRef)CMBufferQueueDequeueAndRetain(previewBufferQueue);
            if (!didReceiveFirstFrame)
            {
                didReceiveFirstFrame = YES;
                [self animateOpen];
            }
            
            if (sbuf) {
                CVImageBufferRef pixBuf = CMSampleBufferGetImageBuffer(sbuf);
                [weakSelf pixelBufferReadyForDisplay:pixBuf];
                CFRelease(sbuf);
            }
        });
    }
    else
    {
        DLog(@"ERROR enqueueing video frame for preview. Code %i", (int)err);
    }
    
    CFRelease(sampleBuffer);
}

- (void)pixelBufferReadyForDisplay:(CVPixelBufferRef)pixelBuffer
{
	// Don't make OpenGLES calls while in the background.
	if ( [UIApplication sharedApplication].applicationState != UIApplicationStateBackground )
    {
        if([self beginFrame]) {
            [self displayPixelBuffer:pixelBuffer];
            [self endFrame];
        }
    }
}

- (void)checkGLError
{
    GLenum err = glGetError();
    if(err != GL_NO_ERROR) {
        DLog(@"OpenGL Error %d!\n", err);
        //assert(0);
    }
}

- (BOOL)initializeBuffers
{
	BOOL success = YES;
	
	glDisable(GL_DEPTH_TEST);
    
    glGenFramebuffers(1, &frameBufferHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferHandle);
    
    glGenRenderbuffers(1, &colorBufferHandle);
    glBindRenderbuffer(GL_RENDERBUFFER, colorBufferHandle);
    
    [[OPENGL_MANAGER oglContext] renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)self.layer];
    
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &renderBufferWidth);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &renderBufferHeight);
    
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorBufferHandle);
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        DLog(@"Failure with standard framebuffer generation");
		success = NO;
	}
#ifdef MULTISAMPLE
    glGenFramebuffers(1, &sampleFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, sampleFramebuffer);
    
    glGenRenderbuffers(1, &sampleColorBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, sampleColorBuffer);
    glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER, 4, GL_RGBA8_OES, renderBufferWidth, renderBufferHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, sampleColorBuffer);
    
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        DLog(@"Failure with multisample framebuffer generation");
		success = NO;
	}
#endif
    //  Create a new CVOpenGLESTexture cache
    CVReturn err = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, NULL, [OPENGL_MANAGER oglContext], NULL, &videoTextureCache);
    if (err) {
        DLog(@"Error at CVOpenGLESTextureCacheCreate %d", err);
        success = NO;
    }
    
    return success;
}

- (void)renderTextureWithSquareVertices:(const GLfloat*)squareVertices textureVertices:(const GLfloat*)textureVertices
{
    // Update attribute values.
	glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, squareVertices);
	glEnableVertexAttribArray(ATTRIB_VERTEX);
	glVertexAttribPointer(ATTRIB_TEXTUREPOSITON, 2, GL_FLOAT, 0, 0, textureVertices);
	glEnableVertexAttribArray(ATTRIB_TEXTUREPOSITON);
    
    // Update uniform values if there are any
    
    // Validate program before drawing. This is a good check, but only really necessary in a debug build.
    // DEBUG macro must be defined in your debug configurations if that's not already the case.
#if defined(DEBUG)
    if (!glueValidateProgram([OPENGL_MANAGER yuvTextureProgram])) {
        DLog(@"Failed to validate program: %d", [OPENGL_MANAGER yuvTextureProgram]);
        return;
    }
#endif
	
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

- (CGRect)textureSamplingRectForCroppingTextureWithAspectRatio:(CGSize)textureAspectRatio toAspectRatio:(CGSize)croppingAspectRatio
{
	CGSize cropScaleAmount = CGSizeMake(croppingAspectRatio.width / textureAspectRatio.width, croppingAspectRatio.height / textureAspectRatio.height);
	textureScale = fmax(cropScaleAmount.width, cropScaleAmount.height);
	CGSize scaledTextureSize = CGSizeMake(textureAspectRatio.width * textureScale, textureAspectRatio.height * textureScale);
	
	if ( cropScaleAmount.height > cropScaleAmount.width ) {
		normalizedSamplingRect.size.width = croppingAspectRatio.width / scaledTextureSize.width;
		normalizedSamplingRect.size.height = 1.0;
	}
	else {
		normalizedSamplingRect.size.height = croppingAspectRatio.height / scaledTextureSize.height;
		normalizedSamplingRect.size.width = 1.0;
	}
	// Center crop
	normalizedSamplingRect.origin.x = (1.0 - normalizedSamplingRect.size.width)/2.0;
	normalizedSamplingRect.origin.y = (1.0 - normalizedSamplingRect.size.height)/2.0;
	
	return normalizedSamplingRect;
}

- (void)cleanUpTextures
{
    if (lumaTexture)
    {
        CFRelease(lumaTexture);
        lumaTexture = NULL;
    }
    
    if (chromaTexture)
    {
        CFRelease(chromaTexture);
        chromaTexture = NULL;
    }
    
    // Periodic texture cache flush every frame
    CVOpenGLESTextureCacheFlush(videoTextureCache, 0);
}

- (bool)beginFrame
{
    // Don't make OpenGLES calls while in the background.
	if ( [UIApplication sharedApplication].applicationState == UIApplicationStateBackground ) return false;
    
	if (frameBufferHandle == 0) {
		BOOL success = [self initializeBuffers];
		if ( !success ) {
			DLog(@"Problem initializing OpenGL buffers.");
            return false;
		}
	}
#ifdef MULTISAMPLE
    glBindFramebuffer(GL_FRAMEBUFFER, sampleFramebuffer);
#else
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferHandle);
#endif
    // Set the view port to the entire view
    glViewport(0, 0, renderBufferWidth, renderBufferHeight);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glClear(GL_COLOR_BUFFER_BIT);
    return true;
}

- (void)endFrame
{
#ifdef MULTISAMPLE
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER_APPLE, frameBufferHandle);
    glBindFramebuffer(GL_READ_FRAMEBUFFER_APPLE, sampleFramebuffer);
    glResolveMultisampleFramebufferAPPLE();
    
    //discard sample buffer
    const GLenum discards[]  = {GL_COLOR_ATTACHMENT0};
    glDiscardFramebufferEXT(GL_READ_FRAMEBUFFER_APPLE,1,discards);
#endif
    // Present
    glBindRenderbuffer(GL_RENDERBUFFER, colorBufferHandle);
    
    EAGLContext *context = [OPENGL_MANAGER oglContext];
    //context may be nil if gl manager goes away before we do
    if(context) [context presentRenderbuffer:GL_RENDERBUFFER];
    [self checkGLError];
}

- (void)displayPixelBuffer:(CVImageBufferRef)pixelBuffer
{
    if (pixelBuffer == nil) return;
    
    pixelBuffer = (CVImageBufferRef)CFRetain(pixelBuffer);
    
    glUseProgram([OPENGL_MANAGER yuvTextureProgram]);
    
    CVReturn err;
    if (videoTextureCache == NULL)
        return;
    
    //cleanup before each frame so we don't force a premature flush of opengl pipeline
    [self cleanUpTextures];
	
    textureWidth = CVPixelBufferGetWidth(pixelBuffer);
	textureHeight = CVPixelBufferGetHeight(pixelBuffer);
    
    // CVOpenGLESTextureCacheCreateTextureFromImage will create GLES texture
    // optimally from CVImageBufferRef.
    
    // Y-plane
    lumaTexture = NULL;
    glActiveTexture(GL_TEXTURE0);
    err = CVOpenGLESTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
                                                       videoTextureCache,
                                                       pixelBuffer,
                                                       NULL,
                                                       GL_TEXTURE_2D,
                                                       GL_RED_EXT,
                                                       (GLsizei)textureWidth,
                                                       (GLsizei)textureHeight,
                                                       GL_RED_EXT,
                                                       GL_UNSIGNED_BYTE,
                                                       0,
                                                       &lumaTexture);
    if (err)
    {
        DLog(@"Error at CVOpenGLESTextureCacheCreateTextureFromImage %d", err);
    }
    
    glBindTexture(CVOpenGLESTextureGetTarget(lumaTexture), CVOpenGLESTextureGetName(lumaTexture));
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // UV-plane
    chromaTexture = NULL;
    glActiveTexture(GL_TEXTURE1);
    err = CVOpenGLESTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
                                                       videoTextureCache,
                                                       pixelBuffer,
                                                       NULL,
                                                       GL_TEXTURE_2D,
                                                       GL_RG_EXT,
                                                       (GLsizei)textureWidth/2,
                                                       (GLsizei)textureHeight/2,
                                                       GL_RG_EXT,
                                                       GL_UNSIGNED_BYTE,
                                                       1,
                                                       &chromaTexture);
    if (err)
    {
        DLog(@"Error at CVOpenGLESTextureCacheCreateTextureFromImage %d", err);
    }
    
    glBindTexture(CVOpenGLESTextureGetTarget(chromaTexture), CVOpenGLESTextureGetName(chromaTexture));
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
    static const GLfloat squareVertices[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        -1.0f,  1.0f,
        1.0f,  1.0f,
    };
    
	// The texture vertices are set up such that we flip the texture vertically.
	// This is so that our top left origin buffers match OpenGL's bottom left texture coordinate system.
	CGRect textureSamplingRect = [self textureSamplingRectForCroppingTextureWithAspectRatio:CGSizeMake(textureWidth, textureHeight) toAspectRatio:CGSizeMake(renderBufferWidth, renderBufferHeight)];
	GLfloat textureVertices[] = {
		CGRectGetMinX(textureSamplingRect), CGRectGetMaxY(textureSamplingRect),
		CGRectGetMaxX(textureSamplingRect), CGRectGetMaxY(textureSamplingRect),
		CGRectGetMinX(textureSamplingRect), CGRectGetMinY(textureSamplingRect),
		CGRectGetMaxX(textureSamplingRect), CGRectGetMinY(textureSamplingRect),
	};
    
    // Draw the texture on the screen with OpenGL ES 2
    [self renderTextureWithSquareVertices:squareVertices textureVertices:textureVertices];
    CFRelease(pixelBuffer);
}

- (CGFloat)angleOffsetFromPortraitOrientationToOrientation:(AVCaptureVideoOrientation)orientation
{
	CGFloat angle = 0.0;
	
	switch (orientation) {
		case AVCaptureVideoOrientationPortrait:
			angle = 0.0;
			break;
		case AVCaptureVideoOrientationPortraitUpsideDown:
			angle = M_PI;
			break;
		case AVCaptureVideoOrientationLandscapeRight:
			angle = -M_PI_2;
			break;
		case AVCaptureVideoOrientationLandscapeLeft:
			angle = M_PI_2;
			break;
		default:
			break;
	}
    
	return angle;
}

- (void)setVideoOrientation:(AVCaptureVideoOrientation)orientation
{
	CGAffineTransform transform = CGAffineTransformIdentity;
    
	// Calculate offsets from an arbitrary reference orientation (portrait)
	CGFloat orientationAngleOffset = [self angleOffsetFromPortraitOrientationToOrientation:orientation];
	CGFloat videoOrientationAngleOffset = [self angleOffsetFromPortraitOrientationToOrientation:AVCaptureVideoOrientationLandscapeRight];
	
	// Find the difference in angle between the passed in orientation and the current video orientation
	CGFloat angleOffset = orientationAngleOffset - videoOrientationAngleOffset;
	transform = CGAffineTransformMakeRotation(angleOffset);
    self.transform = transform;
}

- (void)dealloc
{
	if (frameBufferHandle) {
        glDeleteFramebuffers(1, &frameBufferHandle);
        frameBufferHandle = 0;
    }
	
    if (colorBufferHandle) {
        glDeleteRenderbuffers(1, &colorBufferHandle);
        colorBufferHandle = 0;
    }
    
    [self cleanUpTextures];
    
    if (videoTextureCache) {
        CFRelease(videoTextureCache);
        videoTextureCache = 0;
    }
    
    //    [super dealloc];
}

@end
