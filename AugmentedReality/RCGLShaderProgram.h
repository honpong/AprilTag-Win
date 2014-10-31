//
//  RCGLShaderProgram.h
//  RC3DKSampleAR
//
//  Created by Eagle Jones on 10/31/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

#if !defined(SHADER_STRINGIFY)
#define __STRINGIFY( _x )   # _x
#define SHADER_STRINGIFY( _x )   __STRINGIFY( _x )
#endif

@interface RCGLShaderProgram : NSObject

@property (readonly, nonatomic) GLuint program;
@property (readonly, nonatomic) NSDictionary *uniforms;
@property (readonly, nonatomic) NSDictionary *attributes;

- (BOOL) buildWithVertexSource:(const GLchar *)vertexSource withFragmentSource:(const GLchar *)fragmentSource;
- (BOOL) buildWithVertexFileName:(NSString *)vertexFileName withFragmentFileName:(NSString *)fragmentFileName;
- (GLint) getUniformLocation:(NSString *)name;
- (GLint) getAttribLocation:(NSString *)name;

@end
