//
//  RCGLFeature.h
//  RC3DKSampleVis
//
//  Created by Brian on 8/28/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface RCOpenGLFeature : NSObject

@property float x;
@property float y;
@property float z;
@property float lastSeen;
@property bool good;

-(void) observeWithX:(float)x y:(float)y z:(float)z time:(float)lastSeen good:(bool)good;

@end
