//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//


#import "RCSensorFusionData.h"
#import "RCSensorFusionStatus.h"


@implementation RCSensorFusionData
{

}

- (id) initWithStatus:(RCSensorFusionStatus*)status withTransformation:(RCTransformation*)transformation withFeatures:(NSArray*)featurePoints
{
    if(self = [super init])
    {
        _status = status;
        _transformation = transformation;
        _featurePoints = featurePoints;
    }
    return self;
}

@end