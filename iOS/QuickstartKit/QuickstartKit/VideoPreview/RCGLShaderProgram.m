//
//  RCGLShaderProgram.m
//  RC3DKSampleAR
//
//  Created by Eagle Jones on 10/31/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCGLShaderProgram.h"
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

@implementation RCGLShaderProgram
{
}

@synthesize program, uniforms, attributes;

- (GLuint)compileShader:(const GLchar *)source withType:(GLenum)type
{
    GLuint shader = glCreateShader(type);
    if(!shader) return shader;
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
#ifdef DEBUG
    GLint logLength;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetShaderInfoLog(shader, logLength, &logLength, log);
        NSLog(@"Shader compile log:\n%s", log);
        free(log);
    }
#endif
    return shader;
}

- (NSString *)readFile:(NSString *)name
{
    NSString *path = [[NSBundle mainBundle] pathForResource:name ofType: nil];
    return [NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil];
}

- (NSDictionary *)enumerateVariables:(GLenum)kind
{
    NSMutableDictionary *res = [[NSMutableDictionary alloc] init];
    GLint count;
    glGetProgramiv(program, kind, &count);
    
    GLsizei bufsize;
    if(kind == GL_ACTIVE_UNIFORMS)
    {
        glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &bufsize);
    } else if(kind == GL_ACTIVE_ATTRIBUTES) {
        glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &bufsize);
    } else return nil;
    GLsizei length;
    GLint size;
    GLenum type;
    GLchar name[bufsize];
    if(kind == GL_ACTIVE_UNIFORMS)
    {
        for(int index = 0; index < count; ++index)
        {
            glGetActiveUniform(program, index, bufsize, &length, &size, &type, name);
            GLint loc = glGetUniformLocation(program, name);
            [res setObject:[NSNumber numberWithInt:loc] forKey:[NSString stringWithCString:name encoding:NSUTF8StringEncoding]];
        }
    } else if(kind == GL_ACTIVE_ATTRIBUTES) {
        for(int index = 0; index < count; ++index)
        {
            glGetActiveAttrib(program, index, bufsize, &length, &size, &type, name);
            GLint loc = glGetAttribLocation(program, name);
            [res setObject:[NSNumber numberWithInt:loc] forKey:[NSString stringWithCString:name encoding:NSUTF8StringEncoding]];
        }
    }
    return res;
}

- (BOOL) buildWithVertexSource:(const GLchar *)vertexSource withFragmentSource:(const GLchar *)fragmentSource;
{
    program = glCreateProgram();
    GLint vertex = [self compileShader:vertexSource withType:GL_VERTEX_SHADER];
    GLint fragment = [self compileShader:fragmentSource withType:GL_FRAGMENT_SHADER];
    if(!program || !vertex || !fragment) return false;
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    
    glLinkProgram(program);
    
    //only show link log / validate if we are in debug mode
#ifdef DEBUG
    GLint logLength;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(program, logLength, &logLength, log);
        NSLog(@"Program link log:\n%s", log);
        free(log);
    }
    
    glValidateProgram(program);
    
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(program, logLength, &logLength, log);
        NSLog(@"Program validate log:\n%s", log);
        free(log);
    }
#endif

    GLint param;
    glGetProgramiv(program, GL_LINK_STATUS, &param);
    if(param == GL_FALSE) return false;
    
    uniforms = [self enumerateVariables:GL_ACTIVE_UNIFORMS];
    attributes = [self enumerateVariables:GL_ACTIVE_ATTRIBUTES];

    //Calling delete now marks the shaders for deletion, but they won't actually be deleted until the program is
    glDeleteShader(fragment);
    glDeleteShader(vertex);
    return true;
}

- (BOOL) buildWithVertexFileName:(NSString *)vertexFileName withFragmentFileName:(NSString *)fragmentFileName;
{
    NSString *vertSrc = [self readFile:vertexFileName];
    NSString *fragSrc = [self readFile:fragmentFileName];
    const GLchar *vertString = [vertSrc UTF8String];
    const GLchar *fragString = [fragSrc UTF8String];
    return [self buildWithVertexSource:vertString withFragmentSource:fragString];
}

- (GLint) getUniformLocation:(NSString *)name
{
    NSNumber *loc = (NSNumber *)uniforms[name];
    if(loc == nil)
    {
#ifdef DEBUG
        NSLog(@"Attempted to get location for an invalid uniform: %@. Valid uniforms are: %@", name, uniforms);
#endif
        return -1;
    }
    return [loc intValue];
}

- (GLint) getAttribLocation:(NSString *)name
{
    NSNumber *loc = (NSNumber *)attributes[name];
    if(loc == nil)
    {
#ifdef DEBUG
        NSLog(@"Attempted to get location for an invalid attribute: %@. Valid attributes are: %@", name, attributes);
#endif
        return -1;
    }
    return [loc intValue];
}

- (id) init
{
    if(self = [super init])
    {
        program = 0;
    }
    return self;
}

- (void) dealloc
{
    if(program)
    {
        glDeleteProgram(program);
    }
}

@end
