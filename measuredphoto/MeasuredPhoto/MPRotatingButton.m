//
//  MPRotatingBarButton.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPRotatingButton.h"

@implementation MPRotatingButton

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
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
    UIDeviceOrientation orientation;
    
    if (notification.object)
    {
        [((NSValue*)notification.object) getValue:&orientation];
        [self applyRotationTransformation:orientation animated:YES];
    }
}

@end
