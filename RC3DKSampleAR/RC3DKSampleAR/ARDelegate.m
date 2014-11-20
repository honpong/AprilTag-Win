//
//  ARDelegate.m
//  RC3DKSampleAR
//
//  Created by Eagle Jones on 10/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "ARDelegate.h"
#import <QuartzCore/CAEAGLLayer.h>
#import "RCGLManagerFactory.h"
#import "RCGLShaderProgram.h"
#include "RCDebugLog.h"

@implementation ARDelegate
{
    RCGLShaderProgram *program;
}

@synthesize initialCamera;

-(id)init
{
    if (self = [super init])
    {
        program = [[RCGLShaderProgram alloc] init];
        [program buildWithVertexFileName:@"shader.vsh" withFragmentFileName:@"shader.fsh"];
    }
    return self;
}

const static int vertex_count = 6*6;

const static GLfloat cube_vertices[vertex_count * 3] = {
    //top
    1,1,1, -1,1,1, -1,-1,1,
    -1,-1,1, 1,-1,1, 1,1,1,
    //bottom
    1,1,-1, 1,-1,-1, -1,-1,-1,
    -1,-1,-1, -1,1,-1, 1,1,-1,
    
    //left
    -1,1,1, -1,1,-1, -1,-1,-1,
    -1,-1,-1, -1,-1,1, -1,1,1,
    //right
    1,1,1, 1,-1,1, 1,-1,-1,
    1,-1,-1, 1,1,-1, 1,1,1,
    
    //back
    1,-1,1, -1,-1,1, -1,-1,-1,
    -1,-1,-1, 1,-1,-1, 1,-1,1,
    //front
    1,1,1, 1,1,-1, -1,1,-1,
    -1,1,-1, -1,1,1, 1,1,1
};

const static GLfloat cube_normals[vertex_count * 3] = {
    //top
    0,0,1, 0,0,1, 0,0,1,
    0,0,1, 0,0,1, 0,0,1,
    //bottom
    0,0,-1, 0,0,-1, 0,0,-1,
    0,0,-1, 0,0,-1, 0,0,-1,
    
    //left
    -1,0,0, -1,0,0, -1,0,0,
    -1,0,0, -1,0,0, -1,0,0,
    //right
    1,0,0, 1,0,0, 1,0,0,
    1,0,0, 1,0,0, 1,0,0,
    
    //back
    0,-1,0, 0,-1,0, 0,-1,0,
    0,-1,0, 0,-1,0, 0,-1,0,
    //front
    0,1,0, 0,1,0, 0,1,0,
    0,1,0, 0,1,0, 0,1,0,
};

- (void)renderWithSensorFusionData:(RCSensorFusionData *)data withCameraToScreenMatrix:(GLKMatrix4)cameraToScreen
{
    if(!data.cameraParameters || !data.cameraTransformation) return;
    glUseProgram(program.program);

    glUniformMatrix4fv([program getUniformLocation:@"projection_matrix"], 1, false, cameraToScreen.m);

    GLKMatrix4 camera;
    [[data.cameraTransformation getInverse] getOpenGLMatrix:camera.m];
    glUniformMatrix4fv([program getUniformLocation:@"camera_matrix"], 1, false, camera.m);
    
    glUniform3f([program getUniformLocation:@"light_direction"], 0, 0, 1);
    glUniform4f([program getUniformLocation:@"light_ambient"], .8, .8, .8, 1);
    glUniform4f([program getUniformLocation:@"light_diffuse"], .8, .8, .8, 1);
    glUniform4f([program getUniformLocation:@"light_specular"], .8, .8, .8, 1);
    
    glUniform4f([program getUniformLocation:@"material_ambient"], 0., 0., 1., 1);
    glUniform4f([program getUniformLocation:@"material_diffuse"], 0., 0.5, 1., 1);
    glUniform4f([program getUniformLocation:@"material_specular"], 1., 1., 1., 1);
    glUniform1f([program getUniformLocation:@"material_shininess"], 200.);
    
    //Concatenating GLKit matrices goes left to right, and our shaders multiply with matrices on the left and vectors on the right.
    //So the last transformation listed is applied to our vertices first
    GLKMatrix4 model = GLKMatrix4Identity;
    
    /*if(data.originQRCode == nil)
    {
        //Place it 1/2 meter in front of the initial camera position
        [initialCamera getOpenGLMatrix:model.m];
        model = GLKMatrix4Translate(model, 0, 0, .5);
        GLKMatrix4 inverseInitialCameraRotation;
        [[initialCamera.rotation getInverse] getOpenGLMatrix:inverseInitialCameraRotation.m];
        model = GLKMatrix4Multiply(model, inverseInitialCameraRotation);
    }*/
    
    //Position it with the bottom face on the ground
    model = GLKMatrix4Translate(model, 0., 0., .05);
    //Scale our cube so it's 10 cm on a side
    model = GLKMatrix4Scale(model, .05, .05, .05);

    glUniformMatrix4fv([program getUniformLocation:@"model_matrix"], 1, false, model.m);
    
    glEnableVertexAttribArray([program getAttribLocation:@"position"]);
    glEnableVertexAttribArray([program getAttribLocation:@"normal"]);

    glVertexAttribPointer([program getAttribLocation:@"position"], 3, GL_FLOAT, 0, 0, cube_vertices);
    glVertexAttribPointer([program getAttribLocation:@"normal"], 3, GL_FLOAT, 0, 0, cube_normals);
    
    glDrawArrays(GL_TRIANGLES, 0, vertex_count);

    glDisableVertexAttribArray([program getAttribLocation:@"position"]);
    glDisableVertexAttribArray([program getAttribLocation:@"normal"]);

}


@end

