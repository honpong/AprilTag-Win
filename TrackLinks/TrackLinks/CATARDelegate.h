//
//  MPARDelegate.m
//  MeasuredPhoto
//
//  Created by Eagle Jones on 10/23/14.
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES2/glext.h>
#import <CoreVideo/CVOpenGLESTextureCache.h>
#import <AVFoundation/AVFoundation.h>
#import "RC3DK.h"
#import "RCVideoPreview.h"

@interface CATARDelegate : NSObject <RCVideoPreviewDelegate>

- (void) setProgressHorizontal:(float)horizontal withVertical:(float)vertical;
@property RCTransformation *initialCamera;
@property bool hidden;

@end

