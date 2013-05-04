//
//  TMFeatureLayerManager.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/3/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMFeaturesLayer.h"

@implementation TMFeaturesLayer

int const featureCount = 100;
TMFeatureLayerDelegate* delegate;

- (id) init
{
    self = [super init];
    if(self)
    {
        delegate = [[TMFeatureLayerDelegate alloc] init];
        
        for (int i = 0; i < featureCount; i++)
        {
            CALayer* newLayer = [CALayer new];
            newLayer.delegate = delegate;
            newLayer.hidden = YES;
            [self addSublayer:newLayer];
        }
    }
    return self;
}

- (void) setFeaturePositions:(Feature[])features
{
    
}


@end
