//
//  TMOpenGLManager.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 6/14/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

// the factory returns an instance of an object that implements this protocol
@protocol TMOpenGLManager <NSObject>

@property int exampleProtocolProperty;

- (void) exampleProtocolMethod;

@end

// the factory methods to get/set the instance
@interface TMOpenGLManagerFactory : NSObject

+ (id<TMOpenGLManager>)getInstance;
+ (void)setInstance:(id<TMOpenGLManager>)mockObject; // makes testing possible

@end