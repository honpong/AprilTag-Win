//
//  MPDepthFileWriter.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 5/27/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface MPDepthFileWriter : NSObject

+ (void) writeDepthDataToFile:(NSURL*)url withPoints:(NSArray*)pointArray;

@end
