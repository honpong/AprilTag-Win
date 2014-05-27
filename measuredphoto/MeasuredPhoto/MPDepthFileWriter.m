//
//  MPDepthFileWriter.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 5/27/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPDepthFileWriter.h"

@implementation MPDepthFileWriter

+ (void) writeDepthDataToFile:(NSURL*)url withPoints:(NSArray*)pointArray
{
    if ([[NSFileManager new] createFileAtPath:[url path] contents:nil attributes:nil] != YES) // overwrites existing file
    {
        NSLog(@"Failed to create file: %@", [url path]);
    }
    
    NSError* error;
    NSFileHandle* fileHandle = [NSFileHandle fileHandleForWritingToURL:url error:&error];
    
    [fileHandle writeData:[@"{" dataUsingEncoding:NSUTF8StringEncoding]];
    
    BOOL isFirstItem = YES;
    for (RCFeaturePoint* point in pointArray)
    {
        NSString* string = [NSString stringWithFormat:@"\"%.0f,%.0f\":[%.6f,%.6f,%.6f]", point.x, point.y, point.worldPoint.v0, point.worldPoint.v1, point.worldPoint.v2];
        
        if (isFirstItem)
            isFirstItem = NO;
        else
            string = [@"," stringByAppendingString:string];
        
        [fileHandle writeData:[string dataUsingEncoding:NSUTF8StringEncoding]];
    }
    
    [fileHandle writeData:[@"}" dataUsingEncoding:NSUTF8StringEncoding]];
    [fileHandle closeFile];
}

//+ (NSArray*) buildTriangulatedPointsArray
//{
//    const int pixelsToSkip = 5;
//    const int margin = 20; // ignore a certain number of pixels from the edge of image
//    const int imageWidth = 640 - margin;
//    const int imageHeight = 480 - margin;
//    
//    NSMutableArray* result = [NSMutableArray new];
//    
//    for (int y = margin; y <= imageHeight; y = y + pixelsToSkip)
//    {
//        for (int x = margin; x <= imageWidth; x = x + pixelsToSkip)
//        {
//            RCFeaturePoint* point = [STEREO triangulatePoint:CGPointMake(x, y)];
//            if (point) [result addObject:point];
//        }
//    }
//    
//    return result;
//}

@end
