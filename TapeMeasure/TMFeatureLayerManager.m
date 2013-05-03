//
//  TMFeatureLayerManager.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/3/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMFeatureLayerManager.h"

@implementation TMFeatureLayerManager
@synthesize featureLayer;

TMFeatureLayerDelegate* delegate;

- (id) initWithLayerCount:(int)count
{
    self = [super init];
    if(self)
    {
        featureLayer = [CALayer new];
        delegate = [[TMFeatureLayerDelegate alloc] init];
        
        for (int i = 0; i < count; i++)
        {
            CALayer* newLayer = [CALayer new];
            newLayer.delegate = delegate;
            newLayer.hidden = YES;
            [featureLayer addSublayer:newLayer];
        }
    }
    return self;
}


@end
