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

static const GLchar *vertSrc = "\n\
attribute vec4 position;\n\
attribute mediump vec4 textureCoordinate;\n\
varying mediump vec2 coordinate;\n\
uniform mat4 camera_screen_transform;\n\
\n\
void main()\n\
{\n\
gl_Position = camera_screen_transform * position;\n\
coordinate = textureCoordinate.xy;\n\
}\n\
";

static const GLchar *fragSrc = "\n\
uniform sampler2D videoFrameY;\n\
uniform sampler2D videoFrameUV;\n\
uniform lowp float whiteness;\n\
\n\
varying highp vec2 coordinate;\n\
\n\
void main()\n\
{\n\
mediump vec3 yuv;\n\
lowp vec3 rgb;\n\
\n\
yuv.x = texture2D(videoFrameY, coordinate).r;\n\
yuv.yz = texture2D(videoFrameUV, coordinate).rg - vec2(0.5, 0.5);\n\
\n\
// Using BT.709 which is the standard for HDTV\n\
rgb = mat3(      1,       1,      1,\n\
0, -.18732, 1.8556,\n\
1.57481, -.46813,      0) * yuv * (1. - whiteness) + whiteness;\n\
\n\
gl_FragColor = vec4(rgb, 1);\n\
}\n\
";

#define MULTISAMPLE TRUE

@implementation RCVideoPreview
{
    int renderBufferWidth;
	int renderBufferHeight;
    int rotatedBufferWidth;
    int rotatedBufferHeight;
    float renderBufferScale;

	CVOpenGLESTextureCacheRef videoTextureCache;
    
	GLuint frameBufferHandle;
    GLuint sampleFramebuffer;
	GLuint colorBufferHandle;
    GLuint sampleColorBuffer;
    
    CVOpenGLESTextureRef lumaTexture;
    CVOpenGLESTextureRef chromaTexture;
    
    RCSensorFusionData *dataQueued;
    
    size_t textureWidth, textureHeight;
    
    float xScale;
    float yScale;
    float whiteness;
    GLuint yuvTextureProgram;
}

@synthesize delegate;

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
    [EAGLContext setCurrentContext:[OPENGL_MANAGER oglContext]];
    [OPENGL_MANAGER createProgram:&yuvTextureProgram withVertexShader:vertSrc withFragmentShader:fragSrc];
    glUseProgram(yuvTextureProgram);
    glUniform1i(glGetUniformLocation(yuvTextureProgram, "videoFrameY"), 0);
    glUniform1i(glGetUniformLocation(yuvTextureProgram, "videoFrameUV"), 1);
    glUniform1f(glGetUniformLocation(yuvTextureProgram, "whiteness"), 0.);
    
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
    CVReturn errCache = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, NULL, [OPENGL_MANAGER oglContext], NULL, &videoTextureCache);
    if (errCache) {
        DLog(@"Error at CVOpenGLESTextureCacheCreate %d", errCache);
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
    @synchronized(dataQueued)
    {
        if(!dataQueued) //no previous frame - enqueue it and send the render block.
        {
            dataQueued = data;
            __weak RCVideoPreview* weakSelf = self;
            
            dispatch_async(dispatch_get_main_queue(), ^{
                RCSensorFusionData *localData;
                @synchronized(dataQueued)
                {
                    localData = dataQueued;
                    dataQueued = nil;
                }
                CVPixelBufferRef pixBuf = (CVPixelBufferRef)CFRetain(CMSampleBufferGetImageBuffer(localData.sampleBuffer)); //This can be a zombie object if the pixel buffer was already invalidated (finalized)
                if(CMSampleBufferIsValid(localData.sampleBuffer)) //We put this after retaining the pixel buffer to make sure that invalidate doesn't get called between the check and our retain
                {
                    if([weakSelf beginFrame]) {
                        [weakSelf displayPixelBuffer:pixBuf];
                        //call AR delegate
                        float perspective[16];
                        float camera_screen[16];
                        [weakSelf getPerspectiveMatrix:perspective withCameraParameters:localData.cameraParameters withNear:.01 withFar:100.];
                        [weakSelf getCameraScreenTransform:camera_screen forOrientation:[[UIApplication sharedApplication] statusBarOrientation]];
                        if([weakSelf.delegate respondsToSelector:@selector(renderWithSensorFusionData:withPerspectiveMatrix:withCameraScreenMatrix:)])
                            [weakSelf.delegate renderWithSensorFusionData:localData withPerspectiveMatrix:perspective withCameraScreenMatrix:camera_screen];
                        [weakSelf endFrame];
                    }
                }
                CFRelease(pixBuf);

            });
        }
        else //A frame was already enqueued - drop it and replace with the new one. The render block is already waiting to execute.
        {
            dataQueued = data;
        }
    }
}

- (void) displaySampleBuffer:(CMSampleBufferRef)sampleBuffer
{
    [self displaySensorFusionData:[[RCSensorFusionData alloc] initWithTransformation:nil withCameraTransformation:nil withCameraParameters:nil withTotalPath:nil withFeatures:nil withSampleBuffer:sampleBuffer withTimestamp:nil]];
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
        DLog(@"OpenGL Error %d!\n", err);
        glInsertEventMarkerEXT(0, "com.apple.GPUTools.event.debug-frame");
    }
#endif
}

- (BOOL)createBuffers
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
    
    if(sampleColorBuffer) {
        glDeleteRenderbuffers(1, &sampleColorBuffer);
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
    
    [EAGLContext setCurrentContext:[OPENGL_MANAGER oglContext]];
	if (frameBufferHandle == 0) {
		BOOL success = [self createBuffers];
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
    
    glUseProgram(yuvTextureProgram);
    
    glUniform1f(glGetUniformLocation(yuvTextureProgram, "whiteness"), whiteness);
    
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

    float camera_screen[16];
    UIInterfaceOrientation orientation = [[UIApplication sharedApplication] statusBarOrientation];
    [self getCameraScreenTransform:camera_screen forOrientation:orientation];
    
    glUniformMatrix4fv(glGetUniformLocation(yuvTextureProgram, "camera_screen_transform"), 1, false, camera_screen);
    
    GLfloat squareVertices[] = {
        -xScale, -yScale,
        xScale, -yScale,
        -xScale,  yScale,
        xScale,  yScale,
    };
    
    // Preserve aspect ratio; fill layer bounds
    switch(orientation)
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
    GLfloat textureCoordWidth = 1.;
    GLfloat textureCoordHeight = 1.;
    if(scaleHeight > scaleWidth)
    {
        //We can fit more vertically than horizontally so crop width
        renderBufferScale = scaleHeight;
        textureCoordWidth = scaleWidth / renderBufferScale;
    }
    else
    {
        //We can fit more vertically than horizontally so crop width
        renderBufferScale = scaleWidth;
        textureCoordHeight = scaleHeight / renderBufferScale;
    }
    
    // The texture vertices are set up such that we flip the texture vertically.
    // This is so that our top left origin buffers match OpenGL's bottom left texture coordinate system.
    GLfloat passThroughTextureVertices[] = {
        (1.0 - textureCoordWidth)/2.0, (1.0 + textureCoordHeight)/2.0, // top left
        (1.0 + textureCoordWidth)/2.0, (1.0 + textureCoordHeight)/2.0, // top right
        (1.0 - textureCoordWidth)/2.0, (1.0 - textureCoordHeight)/2.0, // bottom left
        (1.0 + textureCoordWidth)/2.0, (1.0 - textureCoordHeight)/2.0, // bottom right
    };
    
    // Draw the texture on the screen with OpenGL ES 2
    // Update attribute values.
    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, squareVertices);
    glEnableVertexAttribArray(ATTRIB_VERTEX);
    glVertexAttribPointer(ATTRIB_TEXTUREPOSITON, 2, GL_FLOAT, 0, 0, passThroughTextureVertices);
    glEnableVertexAttribArray(ATTRIB_TEXTUREPOSITON);
    
    // Update uniform values if there are any
    
    // Validate program before drawing. This is a good check, but only really necessary in a debug build.
    // DEBUG macro must be defined in your debug configurations if that's not already the case.
#if defined(DEBUG)
    if (!glueValidateProgram(yuvTextureProgram)) {
        DLog(@"Failed to validate program: %d", yuvTextureProgram);
        return;
    }
#endif
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    CFRelease(pixelBuffer);
    [self checkGLError];

}

- (void) getCameraScreenTransform:(GLfloat[16])mout forOrientation:(UIInterfaceOrientation)orientation
{
    memset(mout, 0, sizeof(GLfloat) * 16);
    mout[10] = 1.;
    mout[15] = 1.;
    switch(orientation)
    {
        case UIInterfaceOrientationPortrait:
            mout[1] = -1.;
            mout[4] = 1.;
            break;
        case UIInterfaceOrientationPortraitUpsideDown:
            mout[1] = 1.;
            mout[4] = -1.;
            break;
        case UIInterfaceOrientationLandscapeLeft:
            mout[0] = -1.;
            mout[5] = -1.;
            break;
        default:
        case UIInterfaceOrientationUnknown:
        case UIInterfaceOrientationLandscapeRight:
            mout[0] = 1.;
            mout[5] = 1.;
            break;
    }
}

/*
 Our camera is defined with positive x to the right, positive y down, and positive z forward
 This is rotated from the usual OpenGL camera, which has positive x to the right, positive y up, and positive z back
 (Note both are in fact right-handed systems).
 
 GL normalized device coordinates are left handed, with positive x to the right, positive y up, and positive z forward
 GL typically moves from right-handed camera to left-handed NDC by negating the z axis in the perspective matrix. Instead, we have to negate the y value.
 
 Our projection model: x_pixel_rc = focal_length * kr * (x / z) + center_x
 OpenGL ndc to screen model ( https://www.khronos.org/opengles/sdk/docs/man/xhtml/glViewport.xml ): x_pixel_gl = (1 + x_ndc) * (width_vp / 2)

 We follow the OpenCV convention where a pixel's center is at the pixel coordinate, so our viewport (for 640x480) is [-.5, 639.5], [-.5, 479.5], where OpenGL defines the viewport as [0, 640], [0, 480]
 So x_pixel_gl = (x_pixel_rc + .5) * scale
 
 Solve for x_ndc given (1 + x_ndc) * (width_vp / 2) = (focal_length * kr * (x / z) + center_x + .5) * scale
 x_ndc = focal_length * kr / (width_vp / 2) * scale * (x / z) + (center_x + .5) / (width_vp / 2) * scale - 1
 y_ndc = - (focal_length * kr / (height_vp / 2) * scale * (y / z) - (center_y + .5) / (height_vp / 2) * scale + 1

 We ignore radial distortion and drop the kr term here; better to undistort the image instead.
 */

- (void) getPerspectiveMatrix:(GLfloat[16])mout withCameraParameters:(RCCameraParameters *)cameraParameters withNear:(float)near withFar:(float)far
{
    memset(mout, 0, sizeof(GLfloat) * 16);
    mout[0] = cameraParameters.focalLength / (rotatedBufferWidth / 2.) * renderBufferScale;
    mout[2] = (cameraParameters.opticalCenterX + .5) / (rotatedBufferWidth / 2.) * renderBufferScale - 1;
    mout[5] = -cameraParameters.focalLength / (rotatedBufferHeight / 2.) * renderBufferScale;
    mout[6] = -(cameraParameters.opticalCenterY + .5) / (rotatedBufferHeight / 2.) * renderBufferScale + 1;
    mout[10] = (far+near) / (far-near);
    mout[11] = 1.0f;
    mout[14] = -2 * far * near /  (far-near);
}

- (void)dealloc
{
    [self destroyBuffers];
    
    [self cleanUpTextures];
    
    if (videoTextureCache) {
        CFRelease(videoTextureCache);
        videoTextureCache = 0;
    }
    
    [OPENGL_MANAGER deleteProgram:yuvTextureProgram];
    
    //[super dealloc]; - not needed with ARC
}

@end
