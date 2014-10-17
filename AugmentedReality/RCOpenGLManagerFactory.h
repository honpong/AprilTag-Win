//
//  RCOpenGLManagerFactory.h
//  RCCore
//
//  Created by Ben Hirashima on 7/24/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>
#include <OpenGLES/ES2/gl.h>

// the factory returns an instance of an object that implements this protocol
@protocol RCOpenGLManager <NSObject>

@property EAGLContext* oglContext;
- (const GLchar *)readFile:(NSString *)name;
- (bool)createProgram:(GLuint *)program withVertexShader:(const GLchar *)vertSrc withFragmentShader:(const GLchar *)fragSrc;
- (void)deleteProgram:(GLuint)program;
@end

// the factory methods to get/set the instance
@interface RCOpenGLManagerFactory : NSObject

+ (id<RCOpenGLManager>)getInstance;
+ (void)setInstance:(id<RCOpenGLManager>)mockObject; // makes testing possible

@end

enum {
    ATTRIB_VERTEX,
    ATTRIB_TEXTUREPOSITON,
    NUM_ATTRIBUTES
};