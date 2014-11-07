//
//  MPARDelegate.m
//  MeasuredPhoto
//
//  Created by Eagle Jones on 10/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPARDelegate.h"
#import <QuartzCore/CAEAGLLayer.h>
#import "RCCore/RCGLManagerFactory.h"
#import "RCCore/RCGLShaderProgram.h"

@implementation MPARDelegate
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

const static int vertex_count = 7*4;
const static int vertex_size = 2;

const static GLfloat arrow_vertices[vertex_count * vertex_size] = {
    //top
    -.5, .5,
    -.5, 2.5,
    -1.5, 2.5,
    0., 4.,
    1.5, 2.5,
    .5, 2.5,
    .5, .5,
    
    //right
    .5, .5,
    2.5, .5,
    2.5, 1.5,
    4., 0.,
    2.5, -1.5,
    2.5, -.5,
    .5, -.5,
    
    //bottom
    .5, -.5,
    .5, -2.5,
    1.5, -2.5,
    0., -4.,
    -1.5, -2.5,
    -.5, -2.5,
    -.5, -.5,
    
    //left
    -.5, -.5,
    -2.5, -.5,
    -2.5, -1.5,
    -4., 0.,
    -2.5, 1.5,
    -2.5, .5,
    -.5, .5,

};
/*
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
};*/


- (void) setProgress:(float)progress
{
    
}

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
    GLKMatrix4 model;
    
    //Place it 1/2 meter in front of the initial camera position
    [initialCamera getOpenGLMatrix:model.m];
    model = GLKMatrix4Translate(model, 0, 0, .5);
    
    //Scale our cube so it's 10 cm on a side
    model = GLKMatrix4Scale(model, .05, .05, .05);

    glUniformMatrix4fv([program getUniformLocation:@"model_matrix"], 1, false, model.m);
    
    glEnableVertexAttribArray([program getAttribLocation:@"position"]);
//    glEnableVertexAttribArray([program getAttribLocation:@"normal"]);

    glVertexAttribPointer([program getAttribLocation:@"position"], vertex_size, GL_FLOAT, 0, 0, arrow_vertices);
//    glVertexAttribPointer([program getAttribLocation:@"normal"], 3, GL_FLOAT, 0, 0, cube_normals);
    
    glDrawArrays(GL_LINE_LOOP, 0, vertex_count);

    glDisableVertexAttribArray([program getAttribLocation:@"position"]);
//    glDisableVertexAttribArray([program getAttribLocation:@"normal"]);

}


@end

