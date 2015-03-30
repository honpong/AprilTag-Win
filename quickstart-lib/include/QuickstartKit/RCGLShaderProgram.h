//
//  RCGLShaderProgram.h
//  RC3DKSampleAR
//
//  Created by Eagle Jones on 10/31/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#include <OpenGLES/ES2/gl.h>

/**
 A convenience macro for embedding multi-line strings in source without additional decoration.
 */
#if !defined(SHADER_STRINGIFY)
#define __STRINGIFY( _x )   # _x
#define SHADER_STRINGIFY( _x )   __STRINGIFY( _x )
#endif

/**
 This class manages an OpenGL shader program. It handles compiling the vertex and fragment shaders from files or strings,
 links, and automatically enumerates uniforms and attributes. When the DEBUG macro is not defined, most errors are silently ignored, but
 when debugging (the DEBUG macro must be defined), it will log lots of debugging information to the console.
 */
@interface RCGLShaderProgram : NSObject

/**
 The handle of the program object to be used with, for example, glUseProgram
 */
@property (readonly, nonatomic) GLuint program;
@property (readonly, nonatomic) NSDictionary *uniforms;
@property (readonly, nonatomic) NSDictionary *attributes;

- (BOOL) buildWithVertexSource:(const GLchar *)vertexSource withFragmentSource:(const GLchar *)fragmentSource;
- (BOOL) buildWithVertexFileName:(NSString *)vertexFileName withFragmentFileName:(NSString *)fragmentFileName;

/**
 This requests the uniform location for a given name.
 @returns The index of the uniform bound to  name, or -1 if name is not valid. Note that the shader compiler can drop unused
          variables and -1 will be returned, even if name existed in the original shader source. If the DEBUG macro is defined,
          requesting an invalid variable name will log the entire dictionary of uniforms.
 */
- (GLint) getUniformLocation:(NSString *)name;

/**
 This requests the attribute location for a given name.
 @returns The index of the attribute bound to name, or -1 if name is not valid. Note that the shader compiler can drop unused
 variables and -1 will be returned, even if name existed in the original shader source. If the DEBUG macro is defined,
 requesting an invalid variable name will log the entire dictionary of attributes.
 */
- (GLint) getAttribLocation:(NSString *)name;

@end
