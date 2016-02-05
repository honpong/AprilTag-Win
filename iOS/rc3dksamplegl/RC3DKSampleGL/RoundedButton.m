//
//  RoundedButton.m
//  RC3DKSampleGL
//
//  Created by Ben Hirashima on 3/4/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RoundedButton.h"

@implementation RoundedButton

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        self.layer.cornerRadius = 10.;
    }
    return self;
}

@end
