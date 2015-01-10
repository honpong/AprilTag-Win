//
//  Created by Eagle Jones on 1/10/15.
//  Copyright (c) 2015 Sunlayar. All rights reserved.
//

#import "SLARDelegate.h"
#import <QuartzCore/CAEAGLLayer.h>
#import "RCGLManagerFactory.h"
#import "RCGLShaderProgram.h"
#include "RCDebugLog.h"

@implementation SLARDelegate
{
    RCGLShaderProgram *program;
    float roof_vertex_3D[12];
    float roof_vertex_2D[8];
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

- (void)setRoof3DCoordinates:(float[12])roof3D with2DCoordinates:(float[8])roof2D
{
    for(int i = 0; i < 12; ++i)
    {
        roof_vertex_3D[i] = roof3D[i];
    }
    for(int i = 0; i < 8; ++i)
    {
        roof_vertex_2D[i] = roof2D[i];
    }
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
    
    glUniform4f([program getUniformLocation:@"material_ambient"], 0., 0., 1., .5);
    glUniform4f([program getUniformLocation:@"material_diffuse"], 0., 0.5, 1., .5);
    glUniform4f([program getUniformLocation:@"material_specular"], 1., 1., 1., .5);
    glUniform1f([program getUniformLocation:@"material_shininess"], 200.);
    
    //Concatenating GLKit matrices goes left to right, and our shaders multiply with matrices on the left and vectors on the right.
    //So the last transformation listed is applied to our vertices first
    GLKMatrix4 model = GLKMatrix4Identity;
    
    //Place it 1/2 meter in front of the initial camera position
    [initialCamera getOpenGLMatrix:model.m];
    /*model = GLKMatrix4Translate(model, 0, 0, .5);
    GLKMatrix4 inverseInitialCameraRotation;
    [[initialCamera.rotation getInverse] getOpenGLMatrix:inverseInitialCameraRotation.m];
    model = GLKMatrix4Multiply(model, inverseInitialCameraRotation);
    
    //Scale our cube so it's 10 cm on a side
    model = GLKMatrix4Scale(model, .05, .05, .05);*/

    glUniformMatrix4fv([program getUniformLocation:@"model_matrix"], 1, false, model.m);
    
    glEnableVertexAttribArray([program getAttribLocation:@"position"]);
//    glEnableVertexAttribArray([program getAttribLocation:@"normal"]);

    glVertexAttribPointer([program getAttribLocation:@"position"], 3, GL_FLOAT, 0, 0, roof_vertex_3D);
//    glVertexAttribPointer([program getAttribLocation:@"normal"], 3, GL_FLOAT, 0, 0, cube_normals);
    
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableVertexAttribArray([program getAttribLocation:@"position"]);
 //   glDisableVertexAttribArray([program getAttribLocation:@"normal"]);

}


@end

