//
//  TMOpenGLManager.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 6/14/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMOpenGLManagerFactory.h"

// private inner class implements TMOpenGLManager protocol
@interface TMOpenGLManagerImpl : NSObject <TMOpenGLManager>
{
    int examplePrivateVar;
}

@property int exampleProtocolProperty; // implementation of a property defined in the protocol

@end

@implementation TMOpenGLManagerImpl

- (id)init
{
    self = [super init];
    
    if (self)
    {
        // do custom init stuff
    }
    
    return self;
}

- (void) exampleProtocolMethod
{
    // implementation of a method defined in the protocol
}

@end
// end private inner class

// public factory class
@implementation TMOpenGLManagerFactory

static id<TMOpenGLManager> instance;

+ (id<TMOpenGLManager>) getInstance
{
    if (instance == nil)
    {
        instance = [TMOpenGLManagerImpl new];
    }
    
    return instance;
}

+ (void) setInstance:(id<TMOpenGLManager>)mockObject
{
    instance = mockObject;
}

@end

