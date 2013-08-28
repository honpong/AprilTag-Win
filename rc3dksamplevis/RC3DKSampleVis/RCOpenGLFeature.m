//
//  RCGLFeature.m
//  RC3DKSampleVis
//
//  Created by Brian on 8/28/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import "RCOpenGLFeature.h"

@implementation RCOpenGLFeature
{

}

-(void) observeWithX:(float)x_observed y:(float)y_observed z:(float)z_observed time:(float)time_observed
{
    self.x = x_observed;
    self.y = y_observed;
    self.z = z_observed;
    self.lastSeen = time_observed;
}

@end
