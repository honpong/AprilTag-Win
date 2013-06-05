//
//  TMVideoPreview.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMVideoPreview.h"
#import <QuartzCore/CAEAGLLayer.h>
#include "ShaderUtilities.h"

enum {
    ATTRIB_VERTEX,
    ATTRIB_TEXTUREPOSITON,
    ATTRIB_PERPINDICULAR,
    NUM_ATTRIBUTES
};

@implementation TMVideoPreview

CVOpenGLESTextureRef lumaTexture;
CVOpenGLESTextureRef chromaTexture;
size_t textureWidth = 640;
size_t textureHeight = 480;
float textureScale = 1.;
CGRect normalizedSamplingRect;


+ (Class)layerClass
{
    return [CAEAGLLayer class];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Camera image orientation on screen is fixed
    // with respect to the physical camera orientation.
    
    if (interfaceOrientation == UIInterfaceOrientationPortrait)
        return YES;
    else
        return NO;
}

- (const GLchar *)readFile:(NSString *)name
{
    NSString *path;
    const GLchar *source;
    
    path = [[NSBundle mainBundle] pathForResource:name ofType: nil];
    source = (GLchar *)[[NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil] UTF8String];
    
    return source;
}

- (void)checkGLError
{
    GLenum err = glGetError();
    if(err != GL_NO_ERROR) {
        NSLog(@"OpenGL Error %d!\n", err);
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
    
    [oglContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)self.layer];
    
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &renderBufferWidth);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &renderBufferHeight);
    
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorBufferHandle);
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        NSLog(@"Failure with standard framebuffer generation");
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
        NSLog(@"Failure with multisample framebuffer generation");
		success = NO;
	}
#endif
    //  Create a new CVOpenGLESTexture cache
    CVReturn err = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, NULL, oglContext, NULL, &videoTextureCache);
    if (err) {
        NSLog(@"Error at CVOpenGLESTextureCacheCreate %d", err);
        success = NO;
    }
    
    return success;
}

- (bool)loadShaders
{
    // Load vertex and fragment shaders
    const GLchar *vertSrc = [self readFile:@"yuvtorgb.vsh"];
    const GLchar *fragSrc = [self readFile:@"yuvtorgb.fsh"];

    // attributes
    GLint attribLocation[NUM_ATTRIBUTES] = {
        ATTRIB_VERTEX, ATTRIB_TEXTUREPOSITON, ATTRIB_PERPINDICULAR
    };
    GLchar *attribName[NUM_ATTRIBUTES] = {
        "position", "textureCoordinate", "perpindicular"
    };

    glueCreateProgram(vertSrc, fragSrc,
                      2, (const GLchar **)&attribName[0], attribLocation,
                      0, 0, 0, // we don't need to get uniform locations in this example
                      &yuvTextureProgram);
    
    if (!yuvTextureProgram)
        return false;

    glUseProgram(yuvTextureProgram);
    // Get uniform locations.
    glUniform1i(glGetUniformLocation(yuvTextureProgram, "videoFrameY"), 0);
    glUniform1i(glGetUniformLocation(yuvTextureProgram, "videoFrameUV"), 1);

    //tape ----------------
    // Load vertex and fragment shaders
    vertSrc = [self readFile:@"tape.vsh"];
    fragSrc = [self readFile:@"tape_imperial.fsh"];
    
    glueCreateProgram(vertSrc, fragSrc,
                      3, (const GLchar **)&attribName[0], attribLocation,
                      0, 0, 0, // we don't need to get uniform locations in this example
                      &tapeProgram);
    
    if (!tapeProgram)
        return false;

    return true;
}

- (bool)setupGL
{
    oglContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    if(!oglContext) {
        NSLog(@"Failed to create OpenGL ES context");
        return false;
    }
    [EAGLContext setCurrentContext:oglContext];
    
    if(![self loadShaders]) {
        NSLog(@"Failed to load OpenGL ES shaders");
        return false;
    }
    return true;
}

-(id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self != nil) {
		// Use 2x scale factor on Retina displays.
		self.contentScaleFactor = [[UIScreen mainScreen] scale];
        
        // Initialize OpenGL ES 2
        CAEAGLLayer* eaglLayer = (CAEAGLLayer *)self.layer;
        eaglLayer.opaque = YES;
        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking,
                                        kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat,
                                        nil];
        [self setupGL];
        normalizedSamplingRect = CGRectMake(0., 0., 1., 1.);
    }
	
    return self;
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
    if (!glueValidateProgram(yuvTextureProgram)) {
        NSLog(@"Failed to validate program: %d", yuvTextureProgram);
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

- (void)beginFrame
{
	if (frameBufferHandle == 0) {
		BOOL success = [self initializeBuffers];
		if ( !success ) {
			NSLog(@"Problem initializing OpenGL buffers.");
		}
	}
#ifdef MULTISAMPLE
    glBindFramebuffer(GL_FRAMEBUFFER, sampleFramebuffer);
#else
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferHandle);
#endif
    // Set the view port to the entire view
    glViewport(0, 0, renderBufferWidth, renderBufferHeight);

    glClear(GL_COLOR_BUFFER_BIT);
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
    [oglContext presentRenderbuffer:GL_RENDERBUFFER];
    [self checkGLError];
}

- (void)displayPixelBuffer:(CVImageBufferRef)pixelBuffer
{
    glUseProgram(yuvTextureProgram);

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
        NSLog(@"Error at CVOpenGLESTextureCacheCreateTextureFromImage %d", err);
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
        NSLog(@"Error at CVOpenGLESTextureCacheCreateTextureFromImage %d", err);
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

- (void)displayTapeWithMeasurement:(float[3])measurement withStart:(float[3])start withCameraMatrix:(float[16])camera withFocalCenterRadial:(float[5])focalCenterRadial
{
    glUseProgram(tapeProgram);
    float projection[16];
    float near = .01, far = 1000.;

    [self getPerspectiveMatrix:projection withFocalLength:focalCenterRadial[0] withNear:near withFar:far];
    
    glUniformMatrix4fv(glGetUniformLocation(tapeProgram, "projection_matrix"), 1, false, projection);
    glUniformMatrix4fv(glGetUniformLocation(tapeProgram, "camera_matrix"), 1, false, camera);
    glUniform4f(glGetUniformLocation(tapeProgram, "measurement"), measurement[0], measurement[1], measurement[2], 1.);
    
    // Validate program before drawing. This is a good check, but only really necessary in a debug build.
    // DEBUG macro must be defined in your debug configurations if that's not already the case.
#if defined(DEBUG)
    if (!glueValidateProgram(tapeProgram)) {
        NSLog(@"Failed to validate program: %d", tapeProgram);
        return;
    }
#endif

    GLfloat tapeVertices[] = {
        start[0], start[1], start[2],
        start[0], start[1], start[2],
        start[0] + measurement[0], start[1] + measurement[1], start[2] + measurement[2],
        start[0] + measurement[0], start[1] + measurement[1], start[2] + measurement[2]
    };
    float total_meas = sqrt(measurement[0] * measurement[0] + measurement[1] * measurement[1] + measurement[2] * measurement[2]);
    GLfloat tapeTexCoord[] = {
        0., 0., total_meas, total_meas
    };
    GLfloat tapePerpindicular[] = {
        .01, -.01, .01, -.01
    };
	glVertexAttribPointer(ATTRIB_VERTEX, 3, GL_FLOAT, 0, 0, tapeVertices);
    glEnableVertexAttribArray(ATTRIB_VERTEX);
	glVertexAttribPointer(ATTRIB_TEXTUREPOSITON, 1, GL_FLOAT, 0, 0, tapeTexCoord);
    glEnableVertexAttribArray(ATTRIB_TEXTUREPOSITON);
	glVertexAttribPointer(ATTRIB_PERPINDICULAR, 1, GL_FLOAT, 0, 0, tapePerpindicular);
    glEnableVertexAttribArray(ATTRIB_PERPINDICULAR);
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
	CGFloat videoOrientationAngleOffset = [self angleOffsetFromPortraitOrientationToOrientation:[VIDEOCAP_MANAGER videoOrientation]];
	
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
	
    if (yuvTextureProgram) {
        glDeleteProgram(yuvTextureProgram);
        yuvTextureProgram = 0;
    }
	
    [self cleanUpTextures];

    if (videoTextureCache) {
        CFRelease(videoTextureCache);
        videoTextureCache = 0;
    }
    
    if(tapeProgram) {
        glDeleteProgram(tapeProgram);
        tapeProgram = 0;
    }
    
    if(oglContext) {
        oglContext = nil;
    }
//    [super dealloc];
}

@end

