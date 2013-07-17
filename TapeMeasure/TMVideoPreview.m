//
//  TMAugmentedRealityView.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMVideoPreview.h"
#import <QuartzCore/CAEAGLLayer.h>

@implementation TMVideoPreview
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
    size_t textureWidth;
    size_t textureHeight;
    float textureScale;
    CGRect normalizedSamplingRect;
}

-(id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self != nil)
    {
		textureWidth = 640;
        textureHeight = 480;
        textureScale = 1.;
        
        // Use 2x scale factor on Retina displays.
		self.contentScaleFactor = [[UIScreen mainScreen] scale];
        
        // Initialize OpenGL ES 2
        CAEAGLLayer* eaglLayer = (CAEAGLLayer *)self.layer;
        eaglLayer.opaque = YES;
        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking,
                                        kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat,
                                        nil];

        normalizedSamplingRect = CGRectMake(0., 0., 1., 1.);
        
        xDisp = yDisp = zDisp = 0;
        
        [VIDEO_MANAGER setDelegate:self];
    }
	
    return self;
}

+ (Class)layerClass
{
    return [CAEAGLLayer class];
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

/*
withTapeTransform:(RCTransformation *)tapeTransform withViewTransform:(RCTransformation *)viewTransform withCameraParameters:(RCCameraParameters *)cameraParameters
{
    // Don't make OpenGLES calls while in the background.
	if ( [UIApplication sharedApplication].applicationState != UIApplicationStateBackground )
    {
        [self beginFrame];
        float measurement[3], camera[16], focalCenterRadial[5], start[3];
        measurement[0] = tapeTransform.translation.x;
        measurement[1] = tapeTransform.translation.y;
        measurement[2] = tapeTransform.translation.z;
        [SENSOR_FUSION getCurrentCameraMatrix:camera withFocalCenterRadial:focalCenterRadial withVirtualTapeStart:start]; // TODO: eliminate this call
        [self displayTapeWithMeasurement:measurement withStart:start withCameraMatrix:camera withFocalCenterRadial:focalCenterRadial];
        
        // TODO: revisit horizontal / vertical tapes
        float total = sqrt(measurement[0] * measurement[0] + measurement[1] * measurement[1] + measurement[2] * measurement[2]);
         float horz = sqrt(measurement[0] * measurement[0] + measurement[1] * measurement[1]);
         float vert = fabsf(measurement[2]);
         if(total > .1 && vert / total > .15 && horz / total > .15) { //when one measurement is < 15% of total, the other is >= 99% of hypoteneuse
         float temp_meas[3];
         temp_meas[0] = measurement[0];
         temp_meas[1] = measurement[1];
         temp_meas[2] = 0.;
         [self displayTapeWithMeasurement:temp_meas withStart:start withCameraMatrix:camera withFocalCenterRadial:focalCenterRadial];
         start[0] += measurement[0];
         start[1] += measurement[1];
         temp_meas[0] = 0.;
         temp_meas[1] = 0.;
         temp_meas[2] = measurement[2];
         [self displayTapeWithMeasurement:temp_meas withStart:start withCameraMatrix:camera withFocalCenterRadial:focalCenterRadial];
         }
        [self endFrame];
    }
}
*/

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Camera image orientation on screen is fixed
    // with respect to the physical camera orientation.
    
    if (interfaceOrientation == UIInterfaceOrientationPortrait)
        return YES;
    else
        return NO;
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
                                                       textureWidth,
                                                       textureHeight,
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
                                                       textureWidth/2,
                                                       textureHeight/2,
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
}

- (void) getPerspectiveMatrix:(float[16])mout withFocalLength:(float)focalLength withNear:(float)near withFar:(float)far
{
	mout[0] = 2. * focalLength / textureWidth / normalizedSamplingRect.size.width;
	mout[1] = 0.0f;
	mout[2] = 0.0f;
	mout[3] = 0.0f;
	
	mout[4] = 0.0f;
	mout[5] = -2. * focalLength / textureHeight / normalizedSamplingRect.size.height;
	mout[6] = 0.0f;
	mout[7] = 0.0f;
	
	mout[8] = 0.0f;
	mout[9] = 0.0f;
	mout[10] = (far+near) / (far-near);
	mout[11] = 1.0f;
	
	mout[12] = 0.0f;
	mout[13] = 0.0f;
	mout[14] = -2 * far * near /  (far-near);
	mout[15] = 0.0f;
}

- (void)displayTapeWithMeasurement:(RCTranslation *)measurement withStart:(RCPoint *)start withViewTransform:(RCTransformation *)viewTransform withCameraParameters:(RCCameraParameters *)cameraParameters
{
    glUseProgram([OPENGL_MANAGER tapeProgram]);
    float projection[16];
    float near = .01, far = 1000.;

    [self getPerspectiveMatrix:projection withFocalLength:cameraParameters.focalLength withNear:near withFar:far];
    
    float camera[16];
    [viewTransform getOpenGLMatrix:camera];
    
    glUniformMatrix4fv(glGetUniformLocation([OPENGL_MANAGER tapeProgram], "projection_matrix"), 1, false, projection);
    glUniformMatrix4fv(glGetUniformLocation([OPENGL_MANAGER tapeProgram], "camera_matrix"), 1, false, camera);
    glUniform4f(glGetUniformLocation([OPENGL_MANAGER tapeProgram], "measurement"), measurement.x, measurement.y, measurement.z, 1.);

    RCPoint *end = [measurement transformPoint:start];
    GLfloat tapeVertices[] = {
        start.x, start.y, start.z,
        start.x, start.y, start.z,
        end.x, end.y, end.z,
        end.x, end.y, end.z
    };
    RCScalar *length = [measurement getDistance];
    GLfloat tapeTexCoord[] = {
        0., 0., length.scalar, length.scalar
    };
    GLfloat tapePerpindicular[] = {
        .02, -.02, .02, -.02
    };
	glVertexAttribPointer(ATTRIB_VERTEX, 3, GL_FLOAT, 0, 0, tapeVertices);
    glEnableVertexAttribArray(ATTRIB_VERTEX);
	glVertexAttribPointer(ATTRIB_TEXTUREPOSITON, 1, GL_FLOAT, 0, 0, tapeTexCoord);
    glEnableVertexAttribArray(ATTRIB_TEXTUREPOSITON);
	glVertexAttribPointer(ATTRIB_PERPINDICULAR, 1, GL_FLOAT, 0, 0, tapePerpindicular);
    glEnableVertexAttribArray(ATTRIB_PERPINDICULAR);

    // Validate program before drawing. This is a good check, but only really necessary in a debug build.
    // DEBUG macro must be defined in your debug configurations if that's not already the case.
#if defined(DEBUG)
    if (!glueValidateProgram([OPENGL_MANAGER tapeProgram])) {
        DLog(@"Failed to validate program: %d", [OPENGL_MANAGER tapeProgram]);
        return;
    }
#endif
    
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
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

- (void)setTransformFromCurrentVideoOrientationToOrientation:(AVCaptureVideoOrientation)orientation
{
	CGAffineTransform transform = CGAffineTransformIdentity;
    
	// Calculate offsets from an arbitrary reference orientation (portrait)
	CGFloat orientationAngleOffset = [self angleOffsetFromPortraitOrientationToOrientation:orientation];
	CGFloat videoOrientationAngleOffset = [self angleOffsetFromPortraitOrientationToOrientation:[VIDEO_MANAGER videoOrientation]];
	
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

