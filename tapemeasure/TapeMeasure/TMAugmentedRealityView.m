//
//  TMAugmentedRealityView.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 6/12/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMAugmentedRealityView.h"
#import "TMLineLayerDelegate.h"

@implementation TMAugmentedRealityView
{
    TMCrosshairsLayerDelegate *crosshairsDelegate;
    CALayer *crosshairsLayer;
    NSMutableArray* pointsPool;
}
@synthesize videoView, featuresLayer;

- (id) initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame])
    {
        [self initialize];
    }
    return self;
}

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        [self initialize];
    }
    return self;
}

- (void) initialize
{
    LOGME
        
    videoView = [[TMVideoPreview alloc] initWithFrame:self.frame];
    videoView.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:videoView];
    [self sendSubviewToBack:videoView];
    [videoView addMatchSuperviewConstraints];
    
    featuresLayer = [[RCFeaturesLayer alloc] initWithFeatureCount:FEATURE_COUNT andColor:nil];
    featuresLayer.hidden = YES;
    [self.layer addSublayer:featuresLayer];
    
    crosshairsDelegate = [TMCrosshairsLayerDelegate new];
    crosshairsLayer = [CALayer new];
    [crosshairsLayer setDelegate:crosshairsDelegate];
    crosshairsLayer.hidden = YES;
    [crosshairsLayer setNeedsDisplay];
    [self.layer addSublayer:crosshairsLayer];
}

- (void) layoutSubviews
{
    featuresLayer.frame = CGRectMake(0, 0, self.bounds.size.width, self.bounds.size.height);
    crosshairsLayer.frame = CGRectMake(0, 0, self.bounds.size.width, self.bounds.size.height);
    
    [super layoutSubviews];
}

- (void) showCrosshairs
{
    crosshairsLayer.hidden = NO;
    [crosshairsLayer needsLayout];
}

- (void) hideCrosshairs
{
    crosshairsLayer.hidden = YES;
    [crosshairsLayer needsLayout];
}

- (void) showFeatures
{
    featuresLayer.hidden = NO;
}

- (void) hideFeatures
{
    featuresLayer.hidden = YES;
}

@end
