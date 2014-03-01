//
//  MPThumbnailButton.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/6/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPThumbnailButton.h"
#import "UIView+MPCascadingRotation.h"

@implementation MPThumbnailButton

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)
        {
            [[NSNotificationCenter defaultCenter] addObserver:self
                                                     selector:@selector(handleOrientationChange)
                                                         name:UIDeviceOrientationDidChangeNotification
                                                       object:nil];
        }
    }
    return self;
}

- (void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void) handleOrientationChange
{
    UIDeviceOrientation orientation = [[UIDevice currentDevice] orientation];
    [self applyRotationTransformation:orientation];
}

@end
