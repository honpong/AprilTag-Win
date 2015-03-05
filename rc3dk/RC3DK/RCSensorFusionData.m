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

- (id) initWithTransformation:(RCTransformation*)transformation withCameraTransformation:cameraTransformation withCameraParameters:(RCCameraParameters *)cameraParameters withTotalPath:(RCScalar *)totalPath withFeatures:(NSArray*)featurePoints withSampleBuffer:(CMSampleBufferRef)sampleBuffer withTimestamp:(uint64_t)timestamp withOriginQRCode:(NSString *)originQRCode
{
    if(self = [super init])
    {
        _transformation = transformation;
        _cameraTransformation = cameraTransformation;
        _cameraParameters = cameraParameters;
        _totalPathLength = totalPath;
        _featurePoints = featurePoints;
        if (sampleBuffer) _sampleBuffer = (CMSampleBufferRef)CFRetain(sampleBuffer);
        _timestamp = timestamp;
        _originQRCode = originQRCode;
    }
    return self;
}

- (void) dealloc
{
    if (_sampleBuffer) CFRelease(_sampleBuffer);
}

- (NSDictionary *)dictionaryRepresentation
{
    NSDictionary* dict;
    
    // although this code is repetitive, it might be faster than using a NSMutableDictionary
    if (self.originQRCode)
    {
        dict = @{
               @"transformation": [self.transformation dictionaryRepresentation],
               @"cameraTransformation" : [self.cameraTransformation dictionaryRepresentation],
               @"cameraParameters": [self.cameraParameters dictionaryRepresentation],
               @"featurePoints": self.featurePoints, // expensive
               @"totalPathLength": [self.totalPathLength dictionaryRepresentation],
               @"timestamp": @(self.timestamp),
               @"originQRCode" : self.originQRCode
               };
    }
    else
    {
        dict = @{
               @"transformation": [self.transformation dictionaryRepresentation],
               @"cameraTransformation" : [self.cameraTransformation dictionaryRepresentation],
               @"cameraParameters": [self.cameraParameters dictionaryRepresentation],
               @"featurePoints": self.featurePoints, // expensive
               @"totalPathLength": [self.totalPathLength dictionaryRepresentation],
               @"timestamp": @(self.timestamp)
               };
    }
    
    return dict;
}

- (NSDictionary *)dictionaryRepresentationForJsonSerialization
{
    NSDictionary* dict;
    
    // although this code is repetitive, it might be faster than using a NSMutableDictionary
    if (self.originQRCode)
    {
        dict = @{
                 @"transformation": [self.transformation dictionaryRepresentation],
                 @"cameraTransformation" : [self.cameraTransformation dictionaryRepresentation],
                 @"cameraParameters": [self.cameraParameters dictionaryRepresentation],
                 @"totalPathLength": [self.totalPathLength dictionaryRepresentation],
                 @"timestamp": @(self.timestamp),
                 @"originQRCode" : self.originQRCode
                 };
    }
    else
    {
        dict = @{
                 @"transformation": [self.transformation dictionaryRepresentation],
                 @"cameraTransformation" : [self.cameraTransformation dictionaryRepresentation],
                 @"cameraParameters": [self.cameraParameters dictionaryRepresentation],
                 @"totalPathLength": [self.totalPathLength dictionaryRepresentation],
                 @"timestamp": @(self.timestamp)
                 };
    }
    
    return dict;
}

@end