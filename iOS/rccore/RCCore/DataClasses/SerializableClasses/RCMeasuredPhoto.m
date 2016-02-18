//
//  RCMeasuredPhoto.m
//  RCCore
//
//  Created by Jordan Miller on 6/28/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCMeasuredPhoto.h"

#define DATE_FORMATTER_INBOUND [RCDateFormatter getInstanceForFormat:@"yyyy-MM-dd'T'HH:mm:ss'Z'"]
#define DATE_FORMATTER_OUTBOUND [RCDateFormatter getInstanceForFormat:@"yyyy-MM-dd'T'HH:mm:ss"]


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
    _timestamp = [NSDate date];
    
    //set the identifiers
    [self setIdentifiers];
    
    //now we serialize the contents to json, and we store the PNG as well
    // json serialization isn't out of the box on a featuere point object. so we'll have to do a converstion on it. 
    _jsonRepresntation = [self jsonRepresenation];
    
    //we need to call the upload feature here
    [self upLoad:^{} onFailure:^(NSInteger statusCode){}];
    
}

- (void) upLoad:(void (^)())successBlock onFailure:(void (^)(NSInteger))failureBlock
{
    //returns the persisted URL where this was uploaded too.
    //TODO -> if we don't have an internet connection, we have to wait to do the following when we do have one.
    
    //we need to have a valid user. if we don't have one, we need to create one

    
    
    //we need the user to have been authenticated, and have the apropriate authentication cookies.
    
    //we then need to do a post, that incldudes all the data.
    [self postFileAndJson:successBlock onFailure:failureBlock];
    //[self postMeasuredPhotoJson];
    
    //parse what is returned from the post, pull out the url, save to _persistedUrl

}

- (void) postFileAndJson:(void (^)())successBlock onFailure:(void (^)(NSInteger))failureBlock
{
    RCHTTPClient *instance = [RCHTTPClient sharedInstance];
    
    NSDictionary* postParams = @{ @"json_data": _jsonRepresntation , @"measured_at": [DATE_FORMATTER_OUTBOUND stringFromDate:_timestamp] };
    DLog(@"%@", postParams);
    
    NSMutableURLRequest *request = [instance multipartFormRequestWithMethod:@"POST" path:@"api/v1/measuredphotos/" parameters:postParams constructingBodyWithBlock: ^(id <AFMultipartFormData>formData) {
        [formData appendPartWithFileData:_imageData name:@"photo" fileName:@"photo.png" mimeType:@"image/png"];
    }];
    
    AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];

    DLog(@"Operation: %@", operation);
    [operation setUploadProgressBlock:^(NSUInteger bytesWritten, long long totalBytesWritten, long long totalBytesExpectedToWrite) {
        DLog(@"Sent %lld of %lld bytes", totalBytesWritten, totalBytesExpectedToWrite);
    }];
    
    [operation setCompletionBlockWithSuccess:
     ^(AFHTTPRequestOperation *operation,
       id responseObject) {
         NSString *response = [operation responseString];
         DLog(@"\nresponse: [%@]\n",response);
         if (successBlock) successBlock();
     } failure:^(AFHTTPRequestOperation *operation, NSError *error) {
         DLog(@"error: %@", [operation error]);
         if (failureBlock) failureBlock(0);
     }];
    
    [operation start];
}

- (void) postJsonData:(NSDictionary*)params onSuccess:(void (^)())successBlock onFailure:(void (^)(NSInteger statusCode))failureBlock
{
    DLog(@"%@", params);
    RCHTTPClient *instance = [RCHTTPClient sharedInstance];
    
    [instance
     postPath:@"api/v1/datum_logged/"
     parameters:params
     success:^(AFHTTPRequestOperation *operation, id JSON)
     {
         DLog(@"%@", operation.responseString);
         self.statusCode = [operation.response statusCode];
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

- (void) postMeasuredPhotoJson
{
    LOGME
    NSDictionary* postParams = @{ @"flag":@5, @"blob": _jsonRepresntation };
    
    [self
     postJsonData:postParams
     onSuccess:^()
     {
         
         
     }
     onFailure:^(NSInteger statusCode)
     {
         self.statusCode = statusCode;
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
        [resultArray addObject:[feature dictionaryRepresentation]];
    }
    return [NSArray arrayWithArray:resultArray];
}

- (NSString*) jsonRepresenation
{
    NSDictionary *measuredPhotoDic = [self dictionaryRepresentation];
    NSError *error;
    NSData *measuredPhotoDicJsonData = [NSJSONSerialization dataWithJSONObject:measuredPhotoDic
                                                                       options:NSJSONWritingPrettyPrinted
                                                                         error:&error];
    return [[NSString alloc] initWithData:measuredPhotoDicJsonData encoding:NSUTF8StringEncoding];
}

- (NSDictionary*) dictionaryRepresentation
{
    //instead of making this flat, we're going to call a function which recursively calls to_dictionary on other classes.
    NSMutableDictionary *tmpDic = [NSMutableDictionary dictionaryWithCapacity:6];
    tmpDic[@"pngFileName"] = (_pngFileName ? _pngFileName : @"null" );
    tmpDic[@"fileName"] = (_fileName ? _fileName : @"null" );
    tmpDic[@"bundleID"] = (_bundleID ? _bundleID : @"null" );
    tmpDic[@"vendorUniqueId"] = (_vendorUniqueId ? _vendorUniqueId : @"null" );
    tmpDic[@"featurePoints"] = [self dictionaryArrayFromFeaturePointArray : _featurePoints];
    
    //we return an immutable version
    return [NSDictionary dictionaryWithDictionary:tmpDic];
}

- (NSData*) sampleBufferToNSData:(CMSampleBufferRef)sampleBuffer
{
    if (!sampleBuffer) return nil;
    
    //inside RCSensorFusionData we have a CMSampleBufferRef from which we can get CVImageBufferRef
    CVImageBufferRef pixBuf = CMSampleBufferGetImageBuffer(sampleBuffer);
    if(!pixBuf) return nil;
    
    //we then need to turn that CVImageBuffer into an UIImage, which can be written to NSData as a JPG
    CIImage *ciImage = [CIImage imageWithCVPixelBuffer:pixBuf];
    
    CIContext *temporaryContext = [CIContext contextWithOptions:nil];
    CGImageRef videoImage = [temporaryContext
                             createCGImage:ciImage
                             fromRect:CGRectMake(0, 0,
                                                 CVPixelBufferGetWidth(pixBuf),
                                                 CVPixelBufferGetHeight(pixBuf))];
    
    UIImage *uiImage = [UIImage imageWithCGImage:videoImage];
    CFRelease(videoImage);
    return UIImagePNGRepresentation(uiImage);
}


@end
