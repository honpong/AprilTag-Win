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
- (void) addFeatures: (NSArray *) features;
@end
