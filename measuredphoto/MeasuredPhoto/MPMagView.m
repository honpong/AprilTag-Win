//
//  MPMagView.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 1/20/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPMagView.h"

@implementation MPMagView
@synthesize arView;

- (id)initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame])
    {
        self.backgroundColor = [UIColor redColor];
        self.clipsToBounds = YES;
        
        arView = [[MPAugmentedRealityView alloc] initWithFrame:CGRectMake(0, 0, 480 * 4, 640 * 4)];
        arView.backgroundColor = [UIColor yellowColor];
        [self addSubview:arView];
    }
    return self;
}

/*
// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
- (void)drawRect:(CGRect)rect
{
    // Drawing code
}
*/

@end
