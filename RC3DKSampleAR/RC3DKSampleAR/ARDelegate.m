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

enum {
    ATTRIB_VERTEX,
    NUM_ATTRIBUTES
};

const static GLint attribLocation[NUM_ATTRIBUTES] = {
    ATTRIB_VERTEX
};

const static GLchar * attribName[NUM_ATTRIBUTES] = {
    "position"
};

enum {
    UNIFORM_COLOR,
    UNIFORM_PROJECTION,
    UNIFORM_CAMERA,
    UNIFORM_MODEL,
    NUM_UNIFORMS
};

static int uniformLocation[NUM_UNIFORMS];

const static GLchar *uniformName[NUM_UNIFORMS] =
{
    "color",
    "projection_matrix",
    "camera_matrix",
    "model_matrix"
};


-(id)init
{
    if (self = [super init])
    {
        const char *myVertShader = [[RCOpenGLManagerFactory getInstance] readFile:@"flat.vsh"];
        const char *myFragShader = [[RCOpenGLManagerFactory getInstance] readFile:@"flat.fsh"];
        
        glueCreateProgram(myVertShader, myFragShader,
                          NUM_ATTRIBUTES, attribName, attribLocation,
                          NUM_UNIFORMS, uniformName, uniformLocation,
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
    
    GLKMatrix4 camera;
    [[data.cameraTransformation getInverse] getOpenGLMatrix:camera.m];
    
    GLKMatrix4 model;
    [initialCamera getOpenGLMatrix:model.m];
    
    UIInterfaceOrientation orientation = [[UIApplication sharedApplication] statusBarOrientation];

    glUniformMatrix4fv(uniformLocation[UNIFORM_PROJECTION], 1, false, projection.m);
    glUniformMatrix4fv(uniformLocation[UNIFORM_CAMERA], 1, false, camera.m);
    glUniformMatrix4fv(uniformLocation[UNIFORM_MODEL], 1, false, model.m);

    GLfloat vertices[] = {
        -.05, -.05, 1,
        .05, -.05, 1,
        -.05, .05, 1,
        .05, .05, 1
    };
    glVertexAttribPointer(ATTRIB_VERTEX, 3, GL_FLOAT, 0, 0, vertices);
    glEnableVertexAttribArray(ATTRIB_VERTEX);
    glUniform4f(uniformLocation[UNIFORM_COLOR], 0, 0, 1, 1);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    static const GLfloat cubeVerticesStrip[] = {
        // Front face
        -1,-1,1, 1,-1,1, -1,1,1, 1,1,1,
        // Right face
        1,1,1, 1,-1,1, 1,1,-1, 1,-1,-1,
        // Back face
        1,-1,-1, -1,-1,-1, 1,1,-1, -1,1,-1,
        // Left face
        -1,1,-1, -1,-1,-1, -1,1,1, -1,-1,1,
        // Bottom face
        -1,-1,1, -1,-1,-1, 1,-1,1, 1,-1,-1,
        
        // move to top
        1,-1,-1, -1,1,1,
        
        // Top Face
        -1,1,1, 1,1,1, -1,1,-1, 1,1,-1
    };
    glVertexAttribPointer(ATTRIB_VERTEX, 3, GL_FLOAT, 0, 0, cubeVerticesStrip);
    glEnableVertexAttribArray(ATTRIB_VERTEX);
    glUniform4f(uniformLocation[UNIFORM_COLOR], 0, 1, 0, 1);

    model = GLKMatrix4Translate(model, 0, 0, 2);
    model = GLKMatrix4Scale(model, .1, .1, .1);
    glUniformMatrix4fv(uniformLocation[UNIFORM_MODEL], 1, false, model.m);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 26);

    // Validate program before drawing. This is a good check, but only really necessary in a debug build.
    // DEBUG macro must be defined in your debug configurations if that's not already the case.
#if defined(DEBUG)
    if (!glueValidateProgram(myProgram)) {
        DLog(@"Failed to validate program: %d", myProgram);
        return;
    }
#endif

}


@end

