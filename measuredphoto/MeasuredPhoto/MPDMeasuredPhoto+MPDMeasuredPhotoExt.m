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
    NSString* fileName = [NSString stringWithFormat:@"%@-stereo.json", self.id_guid];
    NSURL* fileUrl = [DOCUMENTS_DIRECTORY_URL URLByAppendingPathComponent:fileName];
    return fileUrl.path;
}

@end
