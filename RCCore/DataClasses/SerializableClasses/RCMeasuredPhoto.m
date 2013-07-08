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

}

- (void) initPhotoMeasurement:(RCSensorFusionData*)sensorFusionInput
{
    //convert CMSampleBufferRef in sensorFusionInput into PNG
    _imageData = [self sampleBufferToNSData : sensorFusionInput.sampleBuffer];
    _pngFileName = [DOCS_DIRECTORY stringByAppendingPathComponent:@"measuredphoto.png"];
    [_imageData writeToFile:_pngFileName atomically:YES];
    
    //We also need to set the feature array
    _featurePoints = sensorFusionInput.featurePoints;
    
    
    //set the identifiers
    [self setIdentifiers];
    
    //now we serialize the contents to json, and we store the PNG as well
    // json serialization isn't out of the box on a featuere point object. so we'll have to do a converstion on it. 
    
    //we need to call the upload feature here
    
    
}

- (void) setIdentifiers
{
    _bundleID = [[NSBundle mainBundle] bundleIdentifier];
    _vendorUniqueId = [[[UIDevice currentDevice] identifierForVendor] UUIDString];

}

- (NSArray*) dictionaryArrayFromFeaturePointArray:(NSArray*)rcFeaturePointArray
{
    //iterate over each point in feature array, push json representation into new array
    //make new array immutable
    //return new immutible array
    NSMutableArray *resultArray = [NSMutableArray arrayWithCapacity:rcFeaturePointArray.count];
    for (RCFeaturePoint *feature in rcFeaturePointArray) {
        [resultArray addObject:[feature dictionaryRepresenation]];
    }
    return [NSArray arrayWithArray:resultArray];
}

- (NSDictionary*) dictionaryRepresenation
{
    //instead of making this flat, we're going to call a function which recursively calls to_dictionary on other classes.
    NSMutableDictionary *tmpDic = [NSMutableDictionary dictionaryWithCapacity:6];
    [tmpDic setObject:(_pngFileName ? _pngFileName : @"null" ) forKey:@"pngFileName"];
    [tmpDic setObject:(_fileName ? _fileName : @"null" ) forKey:@"fileName"];
    [tmpDic setObject:(_bundleID ? _bundleID : @"null" ) forKey:@"bundleID"];
    [tmpDic setObject:(_vendorUniqueId ? _vendorUniqueId : @"null" ) forKey:@"vendorUniqueId"];
    [tmpDic setObject:[self dictionaryArrayFromFeaturePointArray : _featurePoints] forKey:@"featurePoints"];
    
    //we return an immutable version
    return [NSDictionary dictionaryWithDictionary:tmpDic];
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


@end
