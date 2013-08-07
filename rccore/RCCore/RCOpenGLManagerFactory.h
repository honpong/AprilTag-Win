//
//  RCOpenGLManagerFactory.h
//  RCCore
//
//  Created by Ben Hirashima on 7/24/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

// the factory returns an instance of an object that implements this protocol
@protocol RCOpenGLManager <NSObject>

@property GLuint yuvTextureProgram;
@property GLuint tapeProgram;
@property EAGLContext* oglContext;

@end

// the factory methods to get/set the instance
@interface RCOpenGLManagerFactory : NSObject

+ (id<RCOpenGLManager>)getInstance;
+ (void)setInstance:(id<RCOpenGLManager>)mockObject; // makes testing possible

@end

enum {
    ATTRIB_VERTEX,
    ATTRIB_TEXTUREPOSITON,
    ATTRIB_PERPINDICULAR,
    NUM_ATTRIBUTES
};