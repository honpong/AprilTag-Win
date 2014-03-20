//
//  TMAugmentedRealityView.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMVideoPreview.h"
#import <QuartzCore/CAEAGLLayer.h>

@implementation TMVideoPreview

//- (void)displayTapeWithMeasurement:(RCTranslation *)measurement withStart:(RCPoint *)start withViewTransform:(RCTransformation *)viewTransform withCameraParameters:(RCCameraParameters *)cameraParameters
//{
//    glUseProgram([OPENGL_MANAGER tapeProgram]);
//    float projection[16];
//    float near = .01, far = 1000.;
//
//    [self getPerspectiveMatrix:projection withFocalLength:cameraParameters.focalLength withNear:near withFar:far];
//    
//    float camera[16];
//    [viewTransform getOpenGLMatrix:camera];
//    
//    glUniformMatrix4fv(glGetUniformLocation([OPENGL_MANAGER tapeProgram], "projection_matrix"), 1, false, projection);
//    glUniformMatrix4fv(glGetUniformLocation([OPENGL_MANAGER tapeProgram], "camera_matrix"), 1, false, camera);
//    glUniform4f(glGetUniformLocation([OPENGL_MANAGER tapeProgram], "measurement"), measurement.x, measurement.y, measurement.z, 1.);
//
//    RCPoint *end = [measurement transformPoint:start];
//    GLfloat tapeVertices[] = {
//        start.x, start.y, start.z,
//        start.x, start.y, start.z,
//        end.x, end.y, end.z,
//        end.x, end.y, end.z
//    };
//    RCScalar *length = [measurement getDistance];
//    GLfloat tapeTexCoord[] = {
//        0., 0., length.scalar, length.scalar
//    };
//    GLfloat tapePerpindicular[] = {
//        .02, -.02, .02, -.02
//    };
//	glVertexAttribPointer(ATTRIB_VERTEX, 3, GL_FLOAT, 0, 0, tapeVertices);
//    glEnableVertexAttribArray(ATTRIB_VERTEX);
//	glVertexAttribPointer(ATTRIB_TEXTUREPOSITON, 1, GL_FLOAT, 0, 0, tapeTexCoord);
//    glEnableVertexAttribArray(ATTRIB_TEXTUREPOSITON);
//	glVertexAttribPointer(ATTRIB_PERPINDICULAR, 1, GL_FLOAT, 0, 0, tapePerpindicular);
//    glEnableVertexAttribArray(ATTRIB_PERPINDICULAR);
//
//    // Validate program before drawing. This is a good check, but only really necessary in a debug build.
//    // DEBUG macro must be defined in your debug configurations if that's not already the case.
//#if defined(DEBUG)
//    if (!glueValidateProgram([OPENGL_MANAGER tapeProgram])) {
//        DLog(@"Failed to validate program: %d", [OPENGL_MANAGER tapeProgram]);
//        return;
//    }
//#endif
//    
//	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
//}

- (void) getPerspectiveMatrix:(float[16])mout withFocalLength:(float)focalLength withNear:(float)near withFar:(float)far
{
	mout[0] = 2. * focalLength / textureWidth / normalizedSamplingRect.size.width;
	mout[1] = 0.0f;
	mout[2] = 0.0f;
	mout[3] = 0.0f;
	
	mout[4] = 0.0f;
	mout[5] = -2. * focalLength / textureHeight / normalizedSamplingRect.size.height;
	mout[6] = 0.0f;
	mout[7] = 0.0f;
	
	mout[8] = 0.0f;
	mout[9] = 0.0f;
	mout[10] = (far+near) / (far-near);
	mout[11] = 1.0f;
	
	mout[12] = 0.0f;
	mout[13] = 0.0f;
	mout[14] = -2 * far * near /  (far-near);
	mout[15] = 0.0f;
}

@end

