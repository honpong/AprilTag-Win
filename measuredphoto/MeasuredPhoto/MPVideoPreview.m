//
//  TMAugmentedRealityView.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPVideoPreview.h"
#import <QuartzCore/CAEAGLLayer.h>
#import "MPCapturePhoto.h"
#import "RCCore/RCOpenGLManagerFactory.h"
#include "../../AugmentedReality/Shaders/ShaderUtilities.c"

@interface RCVideoPreviewCRT ()

- (void)displayPixelBuffer:(CVImageBufferRef)pixelBuffer;
- (void)initialize;

@end

@implementation MPVideoPreview
{
    GLuint myProgram;
    RCTransformation *cameraTrans;
    RCCameraParameters *cameraParams;
}
- (void) initialize
{
    [super initialize];
    const char *myVertShader = [OPENGL_MANAGER readFile:@"flat.vsh"];
    const char *myFragShader = [OPENGL_MANAGER readFile:@"flat.fsh"];
    // attributes
    GLint attribLocation[1] = {
        ATTRIB_VERTEX
    };
    GLchar *attribName[1] = {
        "position"
    };
    
    glueCreateProgram(myVertShader, myFragShader,
                      1, (const GLchar **)&attribName[0], attribLocation,
                      0, 0, 0, // we don't need to get uniform locations in this example
                      &myProgram);
    assert(myProgram);
    cameraTrans = nil;
    cameraParams = nil;

}

-(id)initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame])
    {
		[[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handleOrientationChange:)
                                                     name:MPUIOrientationDidChangeNotification
                                                   object:nil];
    }
    return self;
}

- (void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void) handleOrientationChange:(NSNotification*)notification
{
    if (notification.object)
    {
        MPOrientationChangeData* data = (MPOrientationChangeData*)notification.object;
        
        if (CGRectEqualToRect(crtClosedFrame, CGRectZero))
        {
            [self setCrtClosedFrame:data.orientation];
            self.frame = crtClosedFrame;
        }
        else
        {
            [self setCrtClosedFrame:data.orientation];
        }
    }
}

- (CGRect) getCrtClosedFrame:(UIDeviceOrientation)orientation
{
    if (UIDeviceOrientationIsLandscape(orientation))
    {
        return CGRectMake(self.superview.frame.size.width / 2, 0, 2., self.superview.frame.size.height);
    }
    else
    {
        return CGRectMake(0, self.superview.frame.size.height / 2, self.superview.frame.size.width, 2.);
    }
}

- (void) setCrtClosedFrame:(UIDeviceOrientation)orientation
{
    crtClosedFrame = [self getCrtClosedFrame:orientation];
}

- (void)displayPixelBuffer:(CVImageBufferRef)pixelBuffer
{
    [super displayPixelBuffer:pixelBuffer];
    [self displayArrow];
}

- (void)displayArrow
{
    if(!cameraParams || !cameraTrans) return;
    glUseProgram(myProgram);
    
    float projection[16];
    float near = .01, far = 1000.;
    
    [self getPerspectiveMatrix:projection withFocalLength:cameraParams.focalLength withNear:near withFar:far];
    
    float camera[16];
    [cameraTrans getOpenGLMatrix:camera];
    
    UIInterfaceOrientation orientation = [[UIApplication sharedApplication] statusBarOrientation];
    float camera_screen[16];
    [self getCameraScreenTransform:camera_screen forOrientation:orientation];

    glUniformMatrix4fv(glGetUniformLocation(myProgram, "projection_matrix"), 1, false, projection);
    glUniformMatrix4fv(glGetUniformLocation(myProgram, "camera_matrix"), 1, false, camera);
    glUniformMatrix4fv(glGetUniformLocation(myProgram, "camera_screen_transform"), 1, false, camera_screen);

    GLfloat vertices[] = {
        -.5, 0, -.5,
        .5, 0, -.5,
        -.5, 0, .5,
        .5, 0, .5
    };
    glVertexAttribPointer(ATTRIB_VERTEX, 3, GL_FLOAT, 0, 0, vertices);
    glEnableVertexAttribArray(ATTRIB_VERTEX);

    // Validate program before drawing. This is a good check, but only really necessary in a debug build.
    // DEBUG macro must be defined in your debug configurations if that's not already the case.
//#if defined(DEBUG)
    if (!glueValidateProgram(myProgram)) {
        DLog(@"Failed to validate program: %d", myProgram);
        return;
    }
//#endif
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

}

- (void) setViewTransform:(RCTransformation *)viewTransform withCameraParameters:(RCCameraParameters *)cameraParameters
{
    cameraParams = cameraParameters;
    cameraTrans = viewTransform;
}

@end

