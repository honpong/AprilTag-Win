//
//  RCOpenGLView.h
//  RC3DKSampleVis
//
//  Created by Brian on 8/26/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface RCOpenGLView : NSOpenGLView

- (void) drawRect: (NSRect) bounds;
- (void) drawForTime: (float) time;
- (void) observeFeatureWithId:(uint64_t)id x:(float)x y:(float)y z:(float)z lastSeen:(float)lastSeen;
- (void) observePathWithTranslationX:(float)x y:(float)y z:(float)z time:(float)time;

- (void) reset;

@end
