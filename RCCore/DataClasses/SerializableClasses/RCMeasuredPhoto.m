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
    _jsonRepresntation = [self jsonRepresenation];
    
    //we need to call the upload feature here
    [self upLoad];
    
}

- (void) upLoad
{
    //returns the persisted URL where this was uploaded too.
    //TODO -> if we don't have an internet connection, we have to wait to do the following when we do have one.
    
    //we need to have a valid user. if we don't have one, we need to create one

    
    
    //we need the user to have been authenticated, and have the apropriate authentication cookies.
    
    //we then need to do a post, that incldudes all the data.
    [self postMeasuredPhotoJson:nil onFailure:nil];
    
    
    //parse what is returned from the post, pull out the url, save to _persistedUrl

}



- (void) postJsonData:(NSDictionary*)params onSuccess:(void (^)())successBlock onFailure:(void (^)(int statusCode))failureBlock
{
    DLog(@"%@", params);
    RCHTTPClient *instance = [RCHTTPClient sharedInstance];
    
    [instance
     postPath:@"api/v1/datum_logged/"
     parameters:params
     success:^(AFHTTPRequestOperation *operation, id JSON)
     {
         DLog(@"%@", operation.responseString);
         if (successBlock) successBlock();
     }
     failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         DLog(@"%@", operation.responseString);
         
         NSString *requestBody = [[NSString alloc] initWithData:operation.request.HTTPBody encoding:NSUTF8StringEncoding];
         DLog(@"Failed request body:\n%@", requestBody);
         if (failureBlock) failureBlock(operation.response.statusCode);
     }
     ];
}

- (void) postMeasuredPhotoJson:(void (^)())successBlock onFailure:(void (^)(int statusCode))failureBlock
{
    DLog(@"postMeasuredPhotoJson");
    NSDictionary* postParams = @{ @"flag":[NSNumber numberWithInt: 5], @"blob": _jsonRepresntation };
    
    [self
     postJsonData:postParams
     onSuccess:^()
     {
         if (successBlock) successBlock();
     }
     onFailure:^(int statusCode)
     {
         if (failureBlock) failureBlock(statusCode);
     }
     ];
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

- (NSString*) jsonRepresenation
{
    NSDictionary *measuredPhotoDic = [self dictionaryRepresenation];
    NSError *error;
    NSData *measuredPhotoDicJsonData = [NSJSONSerialization dataWithJSONObject:measuredPhotoDic
                                                                       options:NSJSONWritingPrettyPrinted
                                                                         error:&error];
    return [[NSString alloc] initWithData:measuredPhotoDicJsonData encoding:NSUTF8StringEncoding];
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
