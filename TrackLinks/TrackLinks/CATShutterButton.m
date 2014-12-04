//  CATShutterButtonView.m
//  TrackLinks
//
//  Created by Ben Hirashima on 2/6/14.
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

#import "CATShutterButton.h"
#import "CATCapturePhoto.h"
#import "UIView+RCOrientationRotation.h"

@implementation CATShutterButton

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)
        {
            [[NSNotificationCenter defaultCenter] addObserver:self
                                                     selector:@selector(handleOrientationChange:)
                                                         name:CATUIOrientationDidChangeNotification
                                                       object:nil];
        }
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
