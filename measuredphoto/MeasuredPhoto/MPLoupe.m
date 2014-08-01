//
//  MPLoupe.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 1/24/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPLoupe.h"
#import "MPCrosshairsLayerDelegate.h"
#import "MPCapturePhoto.h"

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
        
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handleOrientationChange:)
                                                     name:MPUIOrientationDidChangeNotification
                                                   object:nil];
    }
    return self;
}

- (void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void) handleOrientationChange:(NSNotification*)notification
{
    if (notification.object)
    {
        MPOrientationChangeData* data = (MPOrientationChangeData*)notification.object;
       
        switch (data.orientation)
        {
            case UIDeviceOrientationPortrait:
            {
                self.touchPointOffset = CGPointMake(0, self.defaultOffset);
                break;
            }
            case UIDeviceOrientationPortraitUpsideDown:
            {
                self.touchPointOffset = CGPointMake(0, -self.defaultOffset);
                break;
            }
            case UIDeviceOrientationLandscapeLeft:
            {
                self.touchPointOffset = CGPointMake(-self.defaultOffset, 0);
                break;
            }
            case UIDeviceOrientationLandscapeRight:
            {
                self.touchPointOffset = CGPointMake(self.defaultOffset, 0);
                break;
            }
            default:
                break;
        }
    }
}

@end
