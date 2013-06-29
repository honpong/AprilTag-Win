//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//


#import "RCSensorFusionData.h"

@implementation RCSensorFusionData
{

}

- (id) initWithStatus:(RCSensorFusionStatus*)status withTransformation:(RCTransformation*)transformation withCameraTransformation:cameraTransformation withCameraParameters:(RCCameraParameters *)cameraParameters withTotalPath:(RCScalar *)totalPath withFeatures:(NSArray*)featurePoints withSampleBuffer:(CMSampleBufferRef)sampleBuffer
{
    if(self = [super init])
    {
        _status = status;
        _transformation = transformation;
        _cameraTransformation = cameraTransformation;
        _cameraParameters = cameraParameters;
        _totalPath = totalPath;
        _featurePoints = featurePoints;
        _sampleBuffer = sampleBuffer;
    }
    return self;
}

@end