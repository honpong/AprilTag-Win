//
//  TMFeatureLayerManager.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/3/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMFeatureLayerDelegate.h"

typedef enum { FeatureQualityHigh, FeatureQualityMedium, FeatureQualityLow } FeatureQuality;
typedef struct
{
    int x, y;
    FeatureQuality quality;
    
} Feature;

@interface TMFeaturesLayer : CALayer

@end
