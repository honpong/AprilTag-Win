//
//  RCVideoPreview.m
//  RCCore
//
//  Created by Ben Hirashima on 7/24/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCVideoPreview.h"
#import <CoreMedia/CoreMedia.h>
#import "RCGLManagerFactory.h"
#import <GLKit/GLKit.h>
#import "RCGLShaderProgram.h"

#define MULTISAMPLE

#pragma mark - Shader source

static const char *vertSrc = SHADER_STRINGIFY(
attribute vec4 position;
attribute mediump vec4 texture_coordinate;
varying mediump vec2 coordinate;
uniform mat4 camera_screen_rotation;
                 
void main()
{
    gl_Position = camera_screen_rotation * position;
    coordinate = texture_coordinate.xy;
});

static const GLchar *fragSrc = SHADER_STRINGIFY(
uniform sampler2D videoFrameY;
uniform sampler2D videoFrameUV;
uniform lowp float whiteness;

varying highp vec2 coordinate;

void main()
{
    mediump vec3 yuv;
    lowp vec3 rgb;
    
    yuv.x = texture2D(videoFrameY, coordinate).r;
    yuv.yz = texture2D(videoFrameUV, coordinate).rg - vec2(0.5, 0.5);
    
    // Using BT.709 which is the standard for HDTV
    rgb = mat3(     1,       1,      1,
                    0, -.18732, 1.8556,
              1.57481, -.46813,      0) * yuv * (1. - whiteness) + whiteness;
    
    gl_FragColor = vec4(rgb, 1);
});

#pragma mark - RCVideoPreview

@implementation RCVideoPreview
{
    int renderBufferWidth;
	int renderBufferHeight;
    float textureCropScaleX;
    float textureCropScaleY;

	CVOpenGLESTextureCacheRef videoTextureCache;
    
	GLuint frameBufferHandle;
    GLuint sampleFramebuffer;
	GLuint colorBufferHandle;
    GLuint sampleColorBuffer;
    GLuint depthRenderBuffer;
    
    CVOpenGLESTextureRef lumaTexture;
    CVOpenGLESTextureRef chromaTexture;
    
    RCSensorFusionData *dataQueued;
    
    size_t textureWidth, textureHeight;
    
    float xScale;
    float yScale;
    float whiteness;
    RCGLShaderProgram *yuvTextureProgram;
}

@synthesize delegate, orientation;

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
    self.orientation = UIInterfaceOrientationPortrait; // default
    
    [EAGLContext setCurrentContext:[[RCGLManagerFactory getInstance] oglContext]];
    
    [self createBuffers];
    
    yuvTextureProgram = [[RCGLShaderProgram alloc] init];
    [yuvTextureProgram buildWithVertexSource:vertSrc withFragmentSource:fragSrc];
    glUseProgram(yuvTextureProgram.program);
    glUniform1i([yuvTextureProgram getUniformLocation:@"videoFrameY"], 0);
    glUniform1i([yuvTextureProgram getUniformLocation:@"videoFrameUV"], 1);
    glUniform1f([yuvTextureProgram getUniformLocation:@"whiteness"], 0.);
    
    xScale = 1.;
    yScale = 1.;
    
    // Use 2x scale factor on Retina displays.
    self.contentScaleFactor = [[UIScreen mainScreen] scale];
    
    // Initialize OpenGL ES 2
    CAEAGLLayer* eaglLayer = (CAEAGLLayer *)self.layer;
    eaglLayer.opaque = YES;
    eaglLayer.drawableProperties = @{kEAGLDrawablePropertyRetainedBacking: @NO,
                                     kEAGLDrawablePropertyColorFormat: kEAGLColorFormatRGBA8};
    
    //  Create a new CVOpenGLESTexture cache
    CVReturn errCache = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, NULL, [[RCGLManagerFactory getInstance] oglContext], NULL, &videoTextureCache);
    if (errCache) {
        NSLog(@"Error at CVOpenGLESTextureCacheCreate %d", errCache);
    }
    
    delegate = nil;
    dataQueued = nil;
}

+ (Class)layerClass
{
    return [CAEAGLLayer class];
}

- (void) layoutSubviews
{
    [self destroyBuffers];
    [self createBuffers];
}

- (void) displaySensorFusionData:(RCSensorFusionData *)data
{
    @synchronized(self)
    {
        if(!dataQueued) //no previous frame - enqueue it and send the render block.
        {
            dataQueued = data;
            __weak RCVideoPreview* weakSelf = self;
            
            dispatch_async(dispatch_get_main_queue(), ^{
                RCSensorFusionData *localData;
                @synchronized(self)
                {
                    localData = dataQueued;
                    dataQueued = nil;
                }
                CVPixelBufferRef pixBuf = (CMSampleBufferGetImageBuffer(localData.sampleBuffer)); //This can be a zombie object if the pixel buffer was already invalidated (finalized)
                if(pixBuf)
                {
                    pixBuf = (CVPixelBufferRef)CFRetain(pixBuf);
                    if(CMSampleBufferIsValid(localData.sampleBuffer)) //We put this after retaining the pixel buffer to make sure that invalidate doesn't get called between the check and our retain
                    {
                        if([weakSelf beginFrame]) {
                            [weakSelf displayPixelBuffer:pixBuf];
                            //call AR delegate
                            GLKMatrix4 perspective = [weakSelf getPerspectiveMatrixWithCameraParameters:localData.cameraParameters withNear:.01 withFar:100.];
                            GLKMatrix4 camera_screen = [weakSelf getScreenRotationForOrientation:self.orientation];
                            
                            if([weakSelf.delegate respondsToSelector:@selector(renderWithSensorFusionData:withCameraToScreenMatrix:)])
                                [weakSelf.delegate renderWithSensorFusionData:localData withCameraToScreenMatrix:GLKMatrix4Multiply(camera_screen, perspective)];
                            [weakSelf endFrame];
                            if([weakSelf.delegate respondsToSelector:@selector(didRenderWithSensorFusionData:withCameraToScreenMatrix:)])
                                [weakSelf.delegate didRenderWithSensorFusionData:localData withCameraToScreenMatrix:GLKMatrix4Multiply(camera_screen, perspective)];
                            
                            
                        }
                    }
                    CFRelease(pixBuf);
                }

            });
        }
        else //A frame was already enqueued - drop it and replace with the new one. The render block is already waiting to execute.
        {
            dataQueued = data;
        }
    }
}

- (void)receiveVideoFrame:(CMSampleBufferRef)sampleBuffer
{
    [self displaySensorFusionData:[[RCSensorFusionData alloc] initWithTransformation:nil withCameraTransformation:nil withCameraParameters:nil withTotalPath:nil withFeatures:nil withSampleBuffer:sampleBuffer withTimestamp:0 withOriginQRCode:nil]];
}

- (void)setWhiteness:(float)w
{
    whiteness = w;
}

- (void)setHorizontalScale:(float)hScale withVerticalScale:(float)vScale
{
    switch([[UIDevice currentDevice] orientation])
    {
        case UIInterfaceOrientationLandscapeLeft:
        case UIInterfaceOrientationLandscapeRight:
            xScale = hScale;
            yScale = vScale;
            break;
            
        case UIInterfaceOrientationPortrait:
        case UIInterfaceOrientationPortraitUpsideDown:
        default:
            xScale = vScale;
            yScale = hScale;
            break;
    }
    if(xScale == 0.) xScale = 1. / self.bounds.size.width;
    if(yScale == 0.) yScale = 1. / self.bounds.size.height;
}

- (void)checkGLError
{
#ifdef DEBUG
    GLenum err = glGetError();
    if(err != GL_NO_ERROR) {
        NSLog(@"OpenGL Error %d!\n", err);
        glInsertEventMarkerEXT(0, "com.apple.GPUTools.event.debug-frame");
    }
#endif
}

- (BOOL)createBuffers
{
	BOOL success = YES;
    
    glGenFramebuffers(1, &frameBufferHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferHandle);
    
    glGenRenderbuffers(1, &colorBufferHandle);
    glBindRenderbuffer(GL_RENDERBUFFER, colorBufferHandle);
    
    [[[RCGLManagerFactory getInstance] oglContext] renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)self.layer];
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorBufferHandle);
    
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &renderBufferWidth);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &renderBufferHeight);

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
    
    glGenRenderbuffers(1, &depthRenderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBuffer);
    glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT16, renderBufferWidth, renderBufferHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderBuffer);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        NSLog(@"Failure with multisample framebuffer generation");
		success = NO;
	}
#else
    glGenRenderbuffers(1, &depthRenderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, renderBufferWidth, renderBufferHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderBuffer);

#endif
    return success;
}

- (void) destroyBuffers
{
    if (frameBufferHandle) {
        glDeleteFramebuffers(1, &frameBufferHandle);
        frameBufferHandle = 0;
    }
    
    if (colorBufferHandle) {
        glDeleteRenderbuffers(1, &colorBufferHandle);
        colorBufferHandle = 0;
    }
    
    if(sampleFramebuffer) {
        glDeleteFramebuffers(1, &sampleFramebuffer);
        sampleFramebuffer = 0;
    }
    
    if(depthRenderBuffer) {
        glDeleteFramebuffers(1, &depthRenderBuffer);
        depthRenderBuffer = 0;
        
    }
    
    if(sampleColorBuffer) {
        glDeleteRenderbuffers(1, &sampleColorBuffer);
        sampleColorBuffer = 0;
    }
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
    
    [EAGLContext setCurrentContext:[[RCGLManagerFactory getInstance] oglContext]];
	if (frameBufferHandle == 0) {
		BOOL success = [self createBuffers];
		if ( !success ) {
			NSLog(@"Problem initializing OpenGL buffers.");
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
    
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glClearColor(0., 0., 0., 1.);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
    
    EAGLContext *context = [[RCGLManagerFactory getInstance] oglContext];
    //context may be nil if gl manager goes away before we do
    if(context) [context presentRenderbuffer:GL_RENDERBUFFER];
    [self checkGLError];
}

- (void)displayPixelBuffer:(CVImageBufferRef)pixelBuffer
{
    if (pixelBuffer == nil) return;
    
    glDepthMask(GL_FALSE);
    
    pixelBuffer = (CVImageBufferRef)CFRetain(pixelBuffer);
    
    glUseProgram(yuvTextureProgram.program);
    
    glUniform1f([yuvTextureProgram getUniformLocation:@"whiteness"], whiteness);
    
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
                                                       (GLsizei)textureWidth/2,
                                                       (GLsizei)textureHeight/2,
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

    GLKMatrix4 camera_screen = [self getScreenRotationForOrientation:self.orientation];
    
    glUniformMatrix4fv([yuvTextureProgram getUniformLocation:@"camera_screen_rotation"], 1, false, camera_screen.m);
    
    GLfloat squareVertices[] = {
        -xScale, -yScale,
        xScale, -yScale,
        -xScale,  yScale,
        xScale,  yScale,
    };
    
    int rotatedBufferWidth;
    int rotatedBufferHeight;
    float renderBufferScale;

    // Preserve aspect ratio; fill layer bounds
    switch(self.orientation)
    {
        case UIInterfaceOrientationPortrait:
        case UIInterfaceOrientationPortraitUpsideDown:
            rotatedBufferWidth = renderBufferHeight;
            rotatedBufferHeight = renderBufferWidth;
            break;
        default:
        case UIInterfaceOrientationUnknown:
        case UIInterfaceOrientationLandscapeLeft:
        case UIInterfaceOrientationLandscapeRight:
            rotatedBufferWidth = renderBufferWidth;
            rotatedBufferHeight = renderBufferHeight;
            break;
    }
    
    float scaleWidth = rotatedBufferWidth / (float) textureWidth;
    float scaleHeight = rotatedBufferHeight / (float) textureHeight;
    textureCropScaleX = 1.;
    textureCropScaleY = 1.;
    if(scaleHeight > scaleWidth)
    {
        //We can fit more vertically than horizontally so crop width
        renderBufferScale = scaleHeight;
        textureCropScaleX = scaleWidth / renderBufferScale;
    }
    else
    {
        //We can fit more horizontally than vertically so crop height
        renderBufferScale = scaleWidth;
        textureCropScaleY = scaleHeight / renderBufferScale;
    }
    
    // The texture vertices are set up such that we flip the texture vertically.
    // This is so that our top left origin buffers match OpenGL's bottom left texture coordinate system.
    GLfloat passThroughTextureVertices[] = {
        (1.0 - textureCropScaleX)/2.0, (1.0 + textureCropScaleY)/2.0, // top left
        (1.0 + textureCropScaleX)/2.0, (1.0 + textureCropScaleY)/2.0, // top right
        (1.0 - textureCropScaleX)/2.0, (1.0 - textureCropScaleY)/2.0, // bottom left
        (1.0 + textureCropScaleX)/2.0, (1.0 - textureCropScaleY)/2.0, // bottom right
    };
    
    // Draw the texture on the screen with OpenGL ES 2
    // Update attribute values.
    glEnableVertexAttribArray([yuvTextureProgram getAttribLocation:@"position"]);
    glEnableVertexAttribArray([yuvTextureProgram getAttribLocation:@"texture_coordinate"]);

    glVertexAttribPointer([yuvTextureProgram getAttribLocation:@"position"], 2, GL_FLOAT, 0, 0, squareVertices);
    glVertexAttribPointer([yuvTextureProgram getAttribLocation:@"texture_coordinate"], 2, GL_FLOAT, 0, 0, passThroughTextureVertices);
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray([yuvTextureProgram getAttribLocation:@"position"]);
    glDisableVertexAttribArray([yuvTextureProgram getAttribLocation:@"texture_coordinate"]);
    
    glDepthMask(GL_TRUE);
    CFRelease(pixelBuffer);
    [self checkGLError];

}

- (GLKMatrix4) getScreenRotationForOrientation:(UIInterfaceOrientation)orientation_
{
    GLKMatrix4 res;
    memset(&res, 0, sizeof(res));
    res.m[10] = 1.;
    res.m[15] = 1.;
    switch(orientation_)
    {
        case UIInterfaceOrientationPortrait:
            res.m[1] = -1.;
            res.m[4] = 1.;
            break;
        case UIInterfaceOrientationPortraitUpsideDown:
            res.m[1] = 1.;
            res.m[4] = -1.;
            break;
        case UIInterfaceOrientationLandscapeLeft:
            res.m[0] = -1.;
            res.m[5] = -1.;
            break;
        default:
        case UIInterfaceOrientationUnknown:
        case UIInterfaceOrientationLandscapeRight:
            res.m[0] = 1.;
            res.m[5] = 1.;
            break;
    }
    return res;
}

/*
 Our camera is defined with positive x to the right, positive y down, and positive z forward
 This is rotated from the usual OpenGL camera, which has positive x to the right, positive y up, and positive z back
 (Note both are in fact right-handed systems).
 
 GL normalized device coordinates are left handed, with positive x to the right, positive y up, and positive z forward
 GL typically moves from right-handed camera to left-handed NDC by negating the z axis in the perspective matrix. Instead, we have to negate the y value.
 
 To derive the perspective matrix, we start with the following three known equations:
 
 Our projection model: x_image = focal_length * kr * (x / z) + center_x

 We follow the OpenCV convention where a pixel's center is at the pixel coordinate, so our image range (for 640x480) is [-.5, 639.5], [-.5, 479.5]. OpenGL texture coordinates range over [0,1]

 So, x_texture = (x_image + .5) / textureWidth
 
 We crop and scale the texture such that x_ndc = (x_texture - 1/2) * 2 / textureCropScaleX

 Substituting gives us:
 
 x_ndc = ((x_image + .5) / textureWidth - 1/2) * 2 / textureCropScaleX
 
 x_ndc = (((focal_length * kr * x/z + center_x) + .5) / textureWidth - 1/2) * 2 / textureCropScaleX

 x_ndc = focal_length * kr * 2 / textureWidth / textureCropScaleX * (x/z) + (center_x + .5  - textureWidth / 2) * 2 / textureWidth / textureCropScaleX

 We ignore radial distortion and drop the kr term here; better to undistort the image instead.
 */

- (GLKMatrix4) getPerspectiveMatrixWithCameraParameters:(RCCameraParameters *)cameraParameters withNear:(float)near withFar:(float)far
{
    GLKMatrix4 ret;
    memset(&ret, 0, sizeof(ret));
    ret.m[0] = cameraParameters.focalLength * 2. / textureWidth / textureCropScaleX;
    ret.m[5] = -cameraParameters.focalLength * 2. / textureHeight / textureCropScaleY;
    ret.m[8] = (cameraParameters.opticalCenterX + .5 - textureWidth / 2.) * 2. / textureWidth / textureCropScaleX;
    ret.m[9] = -(cameraParameters.opticalCenterY + .5 - textureHeight / 2.) * 2. / textureHeight / textureCropScaleY;
    ret.m[10] = (far+near) / (far-near);
    ret.m[11] = 1.0f;
    ret.m[14] = -2 * far * near /  (far-near);
    return ret;
}

- (void)dealloc
{
    [self destroyBuffers];
    
    [self cleanUpTextures];
    
    if (videoTextureCache) {
        CFRelease(videoTextureCache);
        videoTextureCache = 0;
    }
    
    //[super dealloc]; - not needed with ARC
}

@end
