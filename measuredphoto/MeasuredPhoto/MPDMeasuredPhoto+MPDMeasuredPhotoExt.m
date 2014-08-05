//
//  MPDMeasuredPhoto+MPDMeasuredPhotoExt.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/8/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPDMeasuredPhoto+MPDMeasuredPhotoExt.h"
#import "UIImage+MPImageFile.h"

@implementation MPDMeasuredPhoto (MPDMeasuredPhotoExt)

- (void) awakeFromInsert
{
    [super awakeFromInsert];
    NSString* guid = [[NSUUID UUID] UUIDString];
    self.id_guid = guid;
    self.created_at = [[NSDate date] timeIntervalSince1970];
}

- (BOOL) writeImagetoJpeg:(CMSampleBufferRef)sampleBuffer withOrientation:(UIDeviceOrientation)orientation
{
    if (sampleBuffer)
    {
        sampleBuffer = (CMSampleBufferRef)CFRetain(sampleBuffer);
    }
    else
    {
        return NO;
    }
    
    NSData* jpgData = [UIImage jpegDataFromSampleBuffer:sampleBuffer
                                        withOrientation:[UIView imageOrientationFromDeviceOrientation:orientation]];
    
    NSError* error;
    BOOL success = [jpgData writeToFile:[self imageFileName] options:NSDataWritingFileProtectionNone error:&error];
    
    CFRelease(sampleBuffer);
    
    if (error || !success)
    {
        DLog(@"FAILED TO WRITE JPEG: %@", error); // TODO: better error handling
        return NO;
    }
    else
    {
        return YES;
    }
}

- (NSString*) imageFileName
{
    NSString* fileName = [NSString stringWithFormat:@"%@.jpg", self.id_guid];
    NSURL* fileUrl = [DOCUMENTS_DIRECTORY_URL URLByAppendingPathComponent:fileName];
    return fileUrl.path;
}

- (NSString*) depthFileName
{
    NSString* fileName = [NSString stringWithFormat:@"%@-stereo-remesh.json", self.id_guid];
    NSURL* fileUrl = [DOCUMENTS_DIRECTORY_URL URLByAppendingPathComponent:fileName];
    return fileUrl.path;
}

- (NSString*) annotationsFileName
{
    NSString* fileName = [NSString stringWithFormat:@"%@-annotations.json", self.id_guid];
    NSURL* fileUrl = [DOCUMENTS_DIRECTORY_URL URLByAppendingPathComponent:fileName];
    return fileUrl.path;
}

- (BOOL) writeAnnotationsToFile:(NSString*)jsonString
{
    NSError* error = nil;
    BOOL result = [jsonString writeToFile:[self annotationsFileName] atomically:NO encoding:NSUTF8StringEncoding error:&error];
    if (error) DLog(@"%@", error);
    return result;
}

- (BOOL) deleteAssociatedFiles
{
    NSError* error;
    BOOL isSuccess = YES;
    
    for (NSURL* url in [[NSFileManager defaultManager] contentsOfDirectoryAtURL:DOCUMENTS_DIRECTORY_URL includingPropertiesForKeys:nil options:0 error:&error])
    {
        NSString* fileName = url.pathComponents.lastObject;
        if (url.pathExtension.length > 0 && [fileName beginsWithString:self.id_guid])
        {
            [[NSFileManager defaultManager] removeItemAtURL:url error:&error];
            if (error)
            {
                DLog("%@", error);
                isSuccess = NO;
            }
        }
    }
    
    if (error)
    {
        DLog("%@", error);
        isSuccess = NO;
    }
    
    return isSuccess;
}

@end
