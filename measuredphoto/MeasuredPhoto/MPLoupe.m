//
//  MPLoupe.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 1/24/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPLoupe.h"
#import "MPCrosshairsLayerDelegate.h"

static CGFloat const kACLoupeDefaultRadius = 64;

@implementation MPLoupe
{
    MPCrosshairsLayerDelegate* crosshairsDelegate;
    CALayer* crosshairsLayer;
}

- (id) init
{
    self = [self initWithFrame:CGRectMake(0, 0, kACLoupeDefaultRadius*2, kACLoupeDefaultRadius*2)];
    return self;
}

- (id)initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame])
    {
        self.layer.borderWidth = 0;
		
		UIImageView *loupeImageView = [[UIImageView alloc] initWithFrame:CGRectOffset(CGRectInset(self.bounds, -3.0, -3.0), 0, 2.5)];
		loupeImageView.image = [UIImage imageNamed:@"kb-loupe-hi"];
		loupeImageView.backgroundColor = [UIColor clearColor];
		[self addSubview:loupeImageView];
        
        crosshairsDelegate = [MPCrosshairsLayerDelegate new];
        crosshairsLayer = [CALayer new];
        [crosshairsLayer setDelegate:crosshairsDelegate];
        crosshairsLayer.frame = CGRectMake(0, 0, self.frame.size.width, self.frame.size.height);
        [crosshairsLayer setNeedsDisplay];
        [self.layer addSublayer:crosshairsLayer];
    }
    return self;
}

@end
