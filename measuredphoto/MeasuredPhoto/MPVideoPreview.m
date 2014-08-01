//
//  TMAugmentedRealityView.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPVideoPreview.h"
#import <QuartzCore/CAEAGLLayer.h>
#import "MPCapturePhoto.h"

@implementation MPVideoPreview

-(id)initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame])
    {
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
        
        if (CGRectEqualToRect(crtClosedFrame, CGRectZero))
        {
            [self setCrtClosedFrame:data.orientation];
            self.frame = crtClosedFrame;
        }
        else
        {
            [self setCrtClosedFrame:data.orientation];
        }
    }
}

- (CGRect) getCrtClosedFrame:(UIDeviceOrientation)orientation
{
    if (UIDeviceOrientationIsLandscape(orientation))
    {
        return CGRectMake(self.superview.frame.size.width / 2, 0, 2., self.superview.frame.size.height);
    }
    else
    {
        return CGRectMake(0, self.superview.frame.size.height / 2, self.superview.frame.size.width, 2.);
    }
}

- (void) setCrtClosedFrame:(UIDeviceOrientation)orientation
{
    crtClosedFrame = [self getCrtClosedFrame:orientation];
}

@end

