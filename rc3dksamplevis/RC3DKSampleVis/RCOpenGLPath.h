//
//  RCOpenGLPath.h
//  RC3DKSampleVis
//
//  Created by Brian on 8/30/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface RCOpenGLPath : NSObject

@property NSMutableArray * path;

-(void) observeWithTranslationX:(float)x y:(float)y z:(float)z time:(float)time;
-(void) drawPath:(float)time maxAge:(float)maxAge;

-(void) reset;

@end
