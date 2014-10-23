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

@implementation MPVideoPreview
{
    GLuint myProgram;
}

-(id)init
{
    if (self = [super init])
    {
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
                          0, 0, 0,
                          &myProgram);
        assert(myProgram);
    }
    return self;
}

- (void)renderWithSensorFusionData:(RCSensorFusionData *)data withPerspectiveMatrix:(float[16])projection withCameraScreenMatrix:(GLfloat *)camera_screen
{
    if(!data.cameraParameters || !data.cameraTransformation) return;
    glUseProgram(myProgram);
    
    float near = .01, far = 1000.;
    
    float camera[16];
    [[data.cameraTransformation getInverse] getOpenGLMatrix:camera];
    
    UIInterfaceOrientation orientation = [[UIApplication sharedApplication] statusBarOrientation];

    glUniformMatrix4fv(glGetUniformLocation(myProgram, "projection_matrix"), 1, false, projection);
    glUniformMatrix4fv(glGetUniformLocation(myProgram, "camera_matrix"), 1, false, camera);
    glUniformMatrix4fv(glGetUniformLocation(myProgram, "camera_screen_transform"), 1, false, camera_screen);

    GLfloat vertices[] = {
        0, -.05, -.05,
        0, .05, -.05,
        0, -.05, .05,
        0, .05, .05
    };
    glVertexAttribPointer(ATTRIB_VERTEX, 3, GL_FLOAT, 0, 0, vertices);
    glEnableVertexAttribArray(ATTRIB_VERTEX);

    // Validate program before drawing. This is a good check, but only really necessary in a debug build.
    // DEBUG macro must be defined in your debug configurations if that's not already the case.
#if defined(DEBUG)
    if (!glueValidateProgram(myProgram)) {
        DLog(@"Failed to validate program: %d", myProgram);
        return;
    }
#endif
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

}


@end

