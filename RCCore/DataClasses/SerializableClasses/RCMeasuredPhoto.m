//
//  RCMeasuredPhoto.m
//  RCCore
//
//  Created by Jordan Miller on 6/28/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCMeasuredPhoto.h"

@implementation RCMeasuredPhoto
{
    NSData      *imageData;
    NSString    *filename;
    NSArray     *featurePoints;
    NSURL       *url;
    BOOL        is_persisted;
    NSString    *pngFileName;
}

- (void) initPhotoMeasurement:(RCSensorFusionData*)sensorFusionInput
{
    //convert CMSampleBufferRef in sensorFusionInput into PNG
    imageData = [self sampleBufferToNSData : sensorFusionInput.sampleBuffer];
    pngFileName = [DOCS_DIRECTORY stringByAppendingPathComponent:@"measuredphoto.png"];
    [imageData writeToFile:pngFileName atomically:YES];
    
    //We also need to set the feature array
    featurePoints = sensorFusionInput.featurePoints;
    
    
    //now we serialize the contents to json, and we store the PNG as well
    // json serialization isn't out of the box on a featuere point object. so we'll have to do a converstion on it. 
    
    //we need to call the upload feature here
    
    
}

- (NSDictionary*) rcFeaturePointToDictionary:(RCFeaturePoint*)feature
{
    //instead of making this flat, we're going to call a function which recursively calls to_dictionary on other classes. 
    NSMutableDictionary *tmpDic = [NSMutableDictionary dictionaryWithCapacity:7];
    [tmpDic setObject:[NSNumber numberWithUnsignedInt:feature.id] forKey:@"id"];
    [tmpDic setObject:[NSNumber numberWithFloat:feature.x] forKey:@"x"];
    [tmpDic setObject:[NSNumber numberWithFloat:feature.y] forKey:@"y"];
    [tmpDic setObject:[NSNumber numberWithFloat:feature.depth.scalar] forKey:@"depth_scalar"];
    [tmpDic setObject:[NSNumber numberWithFloat:feature.depth.standardDeviation] forKey:@"depth_standardDeviation"];
    //@property (nonatomic, readonly) RCPoint *worldPoint;
    //@property (nonatomic, readonly) bool initialized;
}

- (NSString*) jsonFromRCFeaturePointArray:(NSArray*)rcFeaturePointArray
{
    //iterate over each point in feature array, push json representation into new array
    //make new array immutable
    //return new immutible array
    NSMutableArray *resultArray = [NSMutableArray arrayWithCapacity:rcFeaturePointArray.count];
    for (RCFeaturePoint *feature in rcFeaturePointArray) {
        [resultArray addObject:[self rcFeaturePointToDictionary:feature]];
    }
}

- (NSData*) sampleBufferToNSData:(CMSampleBufferRef)sampleBuffer
{
    //inside RCSensorFusionData we have a CMSampleBufferRef from which we can get CVImageBufferRef
    CVImageBufferRef pixBuf = CMSampleBufferGetImageBuffer(sampleBuffer);
    //we then need to turn that CVImageBuffer into an UIImage, which can be written to NSData as a PNG
    CIImage *ciImage = [CIImage imageWithCVPixelBuffer:pixBuf];
    
    CIContext *temporaryContext = [CIContext contextWithOptions:nil];
    CGImageRef videoImage = [temporaryContext
                             createCGImage:ciImage
                             fromRect:CGRectMake(0, 0,
                                                 CVPixelBufferGetWidth(pixBuf),
                                                 CVPixelBufferGetHeight(pixBuf))];
    
    UIImage *uiImage = [UIImage imageWithCGImage:videoImage];
    return UIImagePNGRepresentation(uiImage);
}



- (BOOL) is_persisted
{
    return is_persisted;
}


- (NSURL*) url
{
    return url;
}

@end
