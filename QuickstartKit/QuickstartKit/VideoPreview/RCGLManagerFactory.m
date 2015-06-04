//
//  RCGLManagerFactory.m
//  RCCore
//
//  Created by Ben Hirashima on 7/24/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCGLManagerFactory.h"

@interface RCGLManagerImpl : NSObject <RCGLManager>
{
}

@end

@implementation RCGLManagerImpl

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

- (bool)setupGL
{
    oglContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    if(!oglContext) {
        NSLog(@"Failed to create OpenGL ES context");
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
@implementation RCGLManagerFactory

static id<RCGLManager> instance;

+ (id<RCGLManager>) getInstance
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [RCGLManagerImpl new];
    });
    return instance;
}

+ (void) setInstance:(id<RCGLManager>)mockObject
{
    instance = mockObject;
}

@end
