//
//  RCGLManagerFactory.h
//  RCCore
//
//  Created by Ben Hirashima on 7/24/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>
#include <OpenGLES/ES2/gl.h>

// the factory returns an instance of an object that implements this protocol
@protocol RCGLManager <NSObject>

@property EAGLContext* oglContext;

@end

// the factory methods to get/set the instance
@interface RCGLManagerFactory : NSObject

+ (id<RCGLManager>)getInstance;
+ (void)setInstance:(id<RCGLManager>)mockObject; // makes testing possible

@end

