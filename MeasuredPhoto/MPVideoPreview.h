//
//  TMAugmentedRealityView.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES2/glext.h>
#import <CoreVideo/CVOpenGLESTextureCache.h>
#import <AVFoundation/AVFoundation.h>
#import <RC3DK/RC3DK.h>
#import <RCCore/RCVideoPreview.h>
#import <RCCore/RCOpenGLManagerFactory.h>

@interface MPVideoPreview : RCVideoPreview <RCVideoFrameDelegate>

- (void)displayTapeWithMeasurement:(RCTranslation *)measurement withStart:(RCPoint *)start withViewTransform:(RCTransformation *)viewTransform withCameraParameters:(RCCameraParameters *)cameraParameters;

@end

