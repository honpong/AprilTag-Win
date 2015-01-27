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
#include "Balloon.h"

//Uncomment this line to show the origin / axes of the coordinate system
//#define SHOW_AXES

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

const static GLfloat x_vertex[6] = {
    0., 0., 0.,
    1., 0., 0.
};

const static GLfloat y_vertex[6] = {
    0., 0., 0.,
    0., 1., 0.
};

const static GLfloat z_vertex[6] = {
    0., 0., 0.,
    0., 0., 1.
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
    
    glUniform4f([program getUniformLocation:@"material_ambient"], 1., 0., 0., 1);
    glUniform4f([program getUniformLocation:@"material_diffuse"], 0., 0., 0., 1);
    glUniform4f([program getUniformLocation:@"material_specular"], 1., 1., 1., 1);
    glUniform1f([program getUniformLocation:@"material_shininess"], 200.);

    GLKMatrix4 model = GLKMatrix4Identity;
    glUniformMatrix4fv([program getUniformLocation:@"model_matrix"], 1, false, model.m);

#ifdef SHOW_AXES
    glEnableVertexAttribArray([program getAttribLocation:@"position"]);
    glVertexAttribPointer([program getAttribLocation:@"position"], 3, GL_FLOAT, 0, 0, x_vertex);
    glDrawArrays(GL_LINES, 0, 2);
    

    glUniform4f([program getUniformLocation:@"material_ambient"], 0., 1., 0., 1);
    glVertexAttribPointer([program getAttribLocation:@"position"], 3, GL_FLOAT, 0, 0, y_vertex);
    glDrawArrays(GL_LINES, 0, 2);

    glUniform4f([program getUniformLocation:@"material_ambient"], 0., 0., 1., 1);
    glVertexAttribPointer([program getAttribLocation:@"position"], 3, GL_FLOAT, 0, 0, z_vertex);
    glDrawArrays(GL_LINES, 0, 2);
#endif
    
    glUniform4f([program getUniformLocation:@"material_ambient"], 0., 0., 1., 1);
    glUniform4f([program getUniformLocation:@"material_diffuse"], 0., 0.5, 1., 1);
    glUniform4f([program getUniformLocation:@"material_specular"], 1., 1., 1., 1);
    glUniform1f([program getUniformLocation:@"material_shininess"], 200.);
    
    //Concatenating GLKit matrices goes left to right, and our shaders multiply with matrices on the left and vectors on the right.
    //So the last transformation listed is applied to our vertices first
    
    if(data.originQRCode == nil)
    {
        //Place it 1/2 meter in front of the initial camera position
        [initialCamera getOpenGLMatrix:model.m];
        model = GLKMatrix4Translate(model, 0, 0, .5);
        GLKMatrix4 inverseInitialCameraRotation;
        [[initialCamera.rotation getInverse] getOpenGLMatrix:inverseInitialCameraRotation.m];
        model = GLKMatrix4Multiply(model, inverseInitialCameraRotation);
    }
    
    //Position it at the origin
    //model = GLKMatrix4Translate(model, 0., 0., 0.);
    //Scale our model so it's 10 cm on a side
    model = GLKMatrix4Scale(model, .25, .25, .25);
    model = GLKMatrix4RotateX(model, M_PI_2);
    
    glUniformMatrix4fv([program getUniformLocation:@"model_matrix"], 1, false, model.m);
    
    glEnableVertexAttribArray([program getAttribLocation:@"position"]);
    glEnableVertexAttribArray([program getAttribLocation:@"normal"]);

    glVertexAttribPointer([program getAttribLocation:@"position"], 3, GL_FLOAT, 0, 0, BalloonVerts);
    glVertexAttribPointer([program getAttribLocation:@"normal"], 3, GL_FLOAT, 0, 0, BalloonNormals);
    
    glDrawArrays(GL_TRIANGLES, 0, BalloonNumVerts);

    glDisableVertexAttribArray([program getAttribLocation:@"position"]);
    glDisableVertexAttribArray([program getAttribLocation:@"normal"]);

}


@end

