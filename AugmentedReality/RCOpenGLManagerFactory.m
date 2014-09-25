//
//  RCOpenGLManagerFactory.m
//  RCCore
//
//  Created by Ben Hirashima on 7/24/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCOpenGLManagerFactory.h"
#include "ShaderUtilities.h"

@interface RCOpenGLManagerImpl : NSObject <RCOpenGLManager>
{
}

@end

@implementation RCOpenGLManagerImpl

@synthesize oglContext;

- (const GLchar *)readFile:(NSString *)name
{
    NSString *path;
    const GLchar *source;
    
    path = [[NSBundle mainBundle] pathForResource:name ofType: nil];
    source = (GLchar *)[[NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil] UTF8String];
    
    return source;
}

- (bool)createProgram:(GLuint *)program withVertexShader:(const GLchar *)vertSrc withFragmentShader:(const GLchar *)fragSrc
{
    // Load vertex and fragment shaders
    //const GLchar *vertSrc = [self readFile:@"yuvtorgb.vsh"];
    //const GLchar *fragSrc = [self readFile:@"yuvtorgb.fsh"];
    
    // attributes
    GLint attribLocation[NUM_ATTRIBUTES] = {
        ATTRIB_VERTEX, ATTRIB_TEXTUREPOSITON
    };
    GLchar *attribName[NUM_ATTRIBUTES] = {
        "position", "textureCoordinate"
    };
    
    glueCreateProgram(vertSrc, fragSrc,
                      2, (const GLchar **)&attribName[0], attribLocation,
                      0, 0, 0, // we don't need to get uniform locations in this example
                      program);
    
    if (!*program)
        return false;

    return true;
}

- (void)deleteProgram:(GLuint)program
{
    if(program) glDeleteProgram(program);
}

- (bool)setupGL
{
    oglContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    if(!oglContext) {
        DLog(@"Failed to create OpenGL ES context");
        return false;
    }
    [EAGLContext setCurrentContext:oglContext];
    
    return true;
}

- (id)init
{
    self = [super init];
    
    if (self)
    {
        if(![self setupGL]) return nil;
    }
    
    return self;
}

- (void) dealloc
{
    if(oglContext) {
        oglContext = nil;
    }
}

@end
// end private inner class

// public factory class
@implementation RCOpenGLManagerFactory

static id<RCOpenGLManager> instance;

+ (id<RCOpenGLManager>) getInstance
{
    if (instance == nil)
    {
        instance = [RCOpenGLManagerImpl new];
    }
    
    return instance;
}

+ (void) setInstance:(id<RCOpenGLManager>)mockObject
{
    instance = mockObject;
}

@end
