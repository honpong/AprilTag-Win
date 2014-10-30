//
//  ARDelegate.m
//  RC3DKSampleAR
//
//  Created by Eagle Jones on 10/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "ARDelegate.h"
#import <QuartzCore/CAEAGLLayer.h>
#import "RCOpenGLManagerFactory.h"
#include "ShaderUtilities.h"
#include "RCDebugLog.h"

@implementation ARDelegate
{
    GLuint myProgram;
}

@synthesize initialCamera;

-(id)init
{
    if (self = [super init])
    {
        const char *myVertShader = [[RCOpenGLManagerFactory getInstance] readFile:@"flat.vsh"];
        const char *myFragShader = [[RCOpenGLManagerFactory getInstance] readFile:@"flat.fsh"];
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

- (void)renderWithSensorFusionData:(RCSensorFusionData *)data withPerspectiveMatrix:(GLKMatrix4)projection
{
    if(!data.cameraParameters || !data.cameraTransformation) return;
    glUseProgram(myProgram);
    
    float near = .01, far = 1000.;
    
    float camera[16];
    [[data.cameraTransformation getInverse] getOpenGLMatrix:camera];
    
    float model[16];
    [initialCamera getOpenGLMatrix:model];
    
    UIInterfaceOrientation orientation = [[UIApplication sharedApplication] statusBarOrientation];

    glUniformMatrix4fv(glGetUniformLocation(myProgram, "projection_matrix"), 1, false, projection.m);
    glUniformMatrix4fv(glGetUniformLocation(myProgram, "camera_matrix"), 1, false, camera);
    glUniformMatrix4fv(glGetUniformLocation(myProgram, "model_matrix"), 1, false, model);

    GLfloat vertices[] = {
        -.05, -.05, 1,
        .05, -.05, 1,
        -.05, .05, 1,
        .05, .05, 1
    };
    glVertexAttribPointer(ATTRIB_VERTEX, 3, GL_FLOAT, 0, 0, vertices);
    glEnableVertexAttribArray(ATTRIB_VERTEX);
    glUniform4f(glGetUniformLocation(myProgram, "color"), 0, 0, 1, 1);

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

