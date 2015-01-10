//
//  Created by Eagle Jones on 1/10/15.
//  Copyright (c) 2015 Sunlayar. All rights reserved.
//

#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES2/glext.h>
#import <CoreVideo/CVOpenGLESTextureCache.h>
#import <AVFoundation/AVFoundation.h>
#import "RCVideoPreview.h"

@interface SLARDelegate : NSObject <RCVideoPreviewDelegate>

@property RCTransformation *initialCamera;
- (void)setRoof3DCoordinates:(float[12])roof3D with2DCoordinates:(float[8])roof2D;

@end

