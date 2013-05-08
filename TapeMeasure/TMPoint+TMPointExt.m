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

@end
