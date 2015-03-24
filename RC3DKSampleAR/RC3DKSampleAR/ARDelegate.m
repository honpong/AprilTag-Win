//
//  ARDelegate.m
//  RC3DKSampleAR
//
//  Created by Eagle Jones on 10/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "ARDelegate.h"
#import <QuartzCore/CAEAGLLayer.h>
#import <QuickstartKit/QuickstartKit.h>
#include "chair_chesterfield.h"

//Uncomment this line to show the origin / axes of the coordinate system
//#define SHOW_AXES

#pragma mark - Shader source

static const char *vertSrc = SHADER_STRINGIFY(
                                              attribute vec4 position;
                                              
                                              uniform mat4 projection_matrix;
                                              uniform mat4 camera_matrix;
                                              uniform mat4 model_matrix;
                                              
void main()
{
    gl_Position = projection_matrix * camera_matrix * model_matrix * position;
});

static const GLchar *fragSrc = SHADER_STRINGIFY(
void main()
{
    gl_FragColor = vec4(0., 0., 0., .2);
});

#pragma mark - ARDelegate

@implementation ARDelegate
{
    RCGLShaderProgram *program;
    //RCGLShaderProgram *shadowprogram;
    GLKTextureInfo *texture;
}

@synthesize initialCamera;

-(id)init
{
    if (self = [super init])
    {
        program = [[RCGLShaderProgram alloc] init];
        [program buildWithVertexFileName:@"shader.vsh" withFragmentFileName:@"shader.fsh"];
        
        texture = [GLKTextureLoader textureWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"chair_chesterfield_d.png" ofType: nil] options:NULL error:NULL];

        //shadowprogram = [[RCGLShaderProgram alloc] init];
        //[shadowprogram buildWithVertexSource:vertSrc withFragmentSource:fragSrc];

    }
    return self;
}

const static GLfloat squarevertex[] = {
    -.5, -.5, 0.,
    .5, -.5, 0.,
    -.5, .5, 0.,
    .5, .5, 0.,
    -.5, .5, 0.,
    .5, -.5, 0.
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
    
//    glUniform4f([program getUniformLocation:@"material_ambient"], 1., 0., 0., 1);
//    glUniform4f([program getUniformLocation:@"material_diffuse"], 0., 0., 0., 1);
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
    
//    glUniform4f([program getUniformLocation:@"material_ambient"], 0.25, 0.125, .05, 1);
//    glUniform4f([program getUniformLocation:@"material_diffuse"], 0.25, 0.125, .05, 1);
    glUniform4f([program getUniformLocation:@"material_specular"], .1, .1, .1, 1);
    glUniform1f([program getUniformLocation:@"material_shininess"], 20.);
    
    //Concatenating GLKit matrices goes left to right, and our shaders multiply with matrices on the left and vectors on the right.
    //So the last transformation listed is applied to our vertices first
    
    if(data.originQRCode == nil)
    {
        //Place it 1/2 meter in front of the initial camera position
        /*[initialCamera getOpenGLMatrix:model.m];
        model = GLKMatrix4Translate(model, 0, 0, .5);
        GLKMatrix4 inverseInitialCameraRotation;
        [[initialCamera.rotation getInverse] getOpenGLMatrix:inverseInitialCameraRotation.m];
        model = GLKMatrix4Multiply(model, inverseInitialCameraRotation);*/
        model = GLKMatrix4Translate(model, 0., 1.5, -1.5);
    }
    
    //Position it at the origin
    //model = GLKMatrix4Translate(model, 0., 0., 0.);
    //Scale our model so it's 10 cm on a side
    model = GLKMatrix4Scale(model, 1.1, 1.1, 1.1);
    model = GLKMatrix4Translate(model, 0., 0., .5);
    model = GLKMatrix4RotateZ(model, M_PI);
    model = GLKMatrix4RotateX(model, M_PI_2);
    
    glUniformMatrix4fv([program getUniformLocation:@"model_matrix"], 1, false, model.m);
    
    glEnableVertexAttribArray([program getAttribLocation:@"position"]);
    glEnableVertexAttribArray([program getAttribLocation:@"normal"]);
    glEnableVertexAttribArray([program getAttribLocation:@"texture_coordinate"]);

    glVertexAttribPointer([program getAttribLocation:@"position"], 3, GL_FLOAT, 0, 0, chair_chesterfieldVerts);
    glVertexAttribPointer([program getAttribLocation:@"normal"], 3, GL_FLOAT, 0, 0, chair_chesterfieldNormals);
    glVertexAttribPointer([program getAttribLocation:@"texture_coordinate"], 2, GL_FLOAT, 0, 0, chair_chesterfieldTexCoords);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(texture.target, texture.name);
    glUniform1i([program getUniformLocation:@"texture_value"], 0);
    
    glDrawArrays(GL_TRIANGLES, 0, chair_chesterfieldNumVerts);

    glDisableVertexAttribArray([program getAttribLocation:@"position"]);
    glDisableVertexAttribArray([program getAttribLocation:@"normal"]);
    glDisableVertexAttribArray([program getAttribLocation:@"texture_coordinate"]);
    
    
    /*glUseProgram(shadowprogram.program);
    
    model = GLKMatrix4Identity;
    model = GLKMatrix4Translate(model, 0., 1.5, -1.5);
    model = GLKMatrix4Scale(model, .66, .66, .66);
    
    glEnableVertexAttribArray([shadowprogram getAttribLocation:@"position"]);
    glUniformMatrix4fv([shadowprogram getUniformLocation:@"camera_matrix"], 1, false, camera.m);
    glUniformMatrix4fv([shadowprogram getUniformLocation:@"model_matrix"], 1, false, model.m);
    glUniformMatrix4fv([shadowprogram getUniformLocation:@"projection_matrix"], 1, false, cameraToScreen.m);

    glVertexAttribPointer([shadowprogram getAttribLocation:@"position"], 3, GL_FLOAT, 0, 0, squarevertex);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glEnableVertexAttribArray([shadowprogram getAttribLocation:@"position"]);*/
    


}


@end

