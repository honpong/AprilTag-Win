//
//  MPMessageBox.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/25/14.
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

#import "CATMessageBox.h"

@implementation CATMessageBox

- (id)initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame])
    {
        [self initialize];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        [self initialize];
    }
    return self;
}

- (void) initialize
{
    self.layer.cornerRadius = 20.;
}

@end
