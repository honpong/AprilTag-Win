//
//  TMPoint+TMPointExt.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/7/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMPoint+TMPointExt.h"

@implementation TMPoint (TMPointExt)

- (FeatureQuality)getFeatureQualityAsEnum
{
    return (FeatureQuality)self.quality;
}

- (void)setFeatureQualityWithEnum:(FeatureQuality)quality
{
    self.quality = (int)quality;
}

- (void)setFeatureQualityWithFloat:(float)quality
{
    if (quality < 0.5) { self.quality = FeatureQualityLow; return; }
    if (quality < 0.75) { self.quality = FeatureQualityMedium; return; }
    self.quality = FeatureQualityHigh;
}

@end
