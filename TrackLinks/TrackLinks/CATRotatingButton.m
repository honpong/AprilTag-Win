//  CATRotatingBarButton.m
//  TrackLinks
//
//  Created by Ben Hirashima on 7/23/14.
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

#import "CATRotatingButton.h"
#import "CATOrientationChangeData.h"
#import "UIView+RCOrientationRotation.h"
#import "CATConstants.h"

@implementation CATRotatingButton

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handleOrientationChange:)
                                                     name:CATUIOrientationDidChangeNotification
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
        CATOrientationChangeData* data = (CATOrientationChangeData*)notification.object;
        [self applyRotationTransformation:data.orientation animated:data.animated];
    }
}

@end
