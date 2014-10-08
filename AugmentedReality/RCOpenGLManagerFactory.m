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

/*
 The OpenGL context is owned by this singleton because GL's management of contexts and iOS's management of views don't play nicely under ARC
 Specifically, an apple engineer at WWDC confirmed that the storyboard behaves as follows:
 1. creates the view the first time you navigate to it
 2. when you navigate away, does NOT destroy the view
 3. when you navigate back to it, instantiates a NEW copy of the view
 (at this point, the old view still exists, and a new copy has been created, and both have a reference to the same opengl context)
 4. assigns it to wherever that view is stored internally
 (only then is the old view destroyed, when the assignment eliminates the only reference to the old view, and this invalidates the opengl context)
*/
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
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [RCOpenGLManagerImpl new];
    });
    return instance;
}

+ (void) setInstance:(id<RCOpenGLManager>)mockObject
{
    instance = mockObject;
}

@end
