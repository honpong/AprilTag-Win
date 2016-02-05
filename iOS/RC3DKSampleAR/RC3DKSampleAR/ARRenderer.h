//
//  ARDelegate.m
//  RC3DKSampleAR
//
//  Created by Eagle Jones on 10/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES2/glext.h>
#import <CoreVideo/CVOpenGLESTextureCache.h>
#import <AVFoundation/AVFoundation.h>
#import <QuickstartKit/QuickstartKit.h>

@interface ARRenderer : NSObject <RCVideoPreviewDelegate>

@property RCTransformation *initialCamera;

@end

