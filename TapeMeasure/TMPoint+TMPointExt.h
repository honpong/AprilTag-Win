//
//  TMPoint+TMPointExt.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/7/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMPoint.h"

typedef enum { FeatureQualityHigh, FeatureQualityMedium, FeatureQualityLow } FeatureQuality;

@interface TMPoint (TMPointExt)

- (FeatureQuality)getFeatureQualityAsEnum;
- (void)setFeatureQualityWithEnum:(FeatureQuality)quality;

@end
