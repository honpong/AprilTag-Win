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

@synthesize yuvTextureProgram;
@synthesize tapeProgram;
@synthesize oglContext;

- (const GLchar *)readFile:(NSString *)name
{
    NSString *path;
    const GLchar *source;
    
    path = [[NSBundle mainBundle] pathForResource:name ofType: nil];
    source = (GLchar *)[[NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil] UTF8String];
    
    return source;
}

const GLchar *vertSrc = "\n\
attribute vec4 position;\n\
attribute mediump vec4 textureCoordinate;\n\
varying mediump vec2 coordinate;\n\
\n\
void main()\n\
{\n\
	gl_Position = position;\n\
	coordinate = textureCoordinate.xy;\n\
}\n\
";

const GLchar *fragSrc = "\n\
uniform sampler2D videoFrameY;\n\
uniform sampler2D videoFrameUV;\n\
\n\
varying highp vec2 coordinate;\n\
\n\
void main()\n\
{\n\
    mediump vec3 yuv;\n\
    lowp vec3 rgb;\n\
    \n\
    yuv.x = texture2D(videoFrameY, coordinate).r;\n\
    yuv.yz = texture2D(videoFrameUV, coordinate).rg - vec2(0.5, 0.5);\n\
    \n\
    // Using BT.709 which is the standard for HDTV\n\
    rgb = mat3(      1,       1,      1,\n\
               0, -.18732, 1.8556,\n\
               1.57481, -.46813,      0) * yuv;\n\
    \n\
    gl_FragColor = vec4(rgb, 1);\n\
}\n\
";

- (bool)loadShaders
{
    // Load vertex and fragment shaders
    //const GLchar *vertSrc = [self readFile:@"yuvtorgb.vsh"];
    //const GLchar *fragSrc = [self readFile:@"yuvtorgb.fsh"];
    
    // attributes
    GLint attribLocation[NUM_ATTRIBUTES] = {
        ATTRIB_VERTEX, ATTRIB_TEXTUREPOSITON, ATTRIB_PERPINDICULAR
    };
    GLchar *attribName[NUM_ATTRIBUTES] = {
        "position", "textureCoordinate", "perpindicular"
    };
    
    glueCreateProgram(vertSrc, fragSrc,
                      2, (const GLchar **)&attribName[0], attribLocation,
                      0, 0, 0, // we don't need to get uniform locations in this example
                      &yuvTextureProgram);
    
    if (!yuvTextureProgram)
        return false;
    
    glUseProgram(yuvTextureProgram);
    // Get uniform locations.
    glUniform1i(glGetUniformLocation(yuvTextureProgram, "videoFrameY"), 0);
    glUniform1i(glGetUniformLocation(yuvTextureProgram, "videoFrameUV"), 1);
    
    return true;
}

- (bool)setupGL
{
    oglContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    if(!oglContext) {
        DLog(@"Failed to create OpenGL ES context");
        return false;
    }
    [EAGLContext setCurrentContext:oglContext];
    
    if(![self loadShaders]) {
        DLog(@"Failed to load OpenGL ES shaders");
        return false;
    }
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
    if (yuvTextureProgram) {
        glDeleteProgram(yuvTextureProgram);
        yuvTextureProgram = 0;
    }
    if(tapeProgram) {
        glDeleteProgram(tapeProgram);
        tapeProgram = 0;
    }
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
