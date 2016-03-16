//
//  MPDMeasuredPhoto+MPDMeasuredPhotoExt.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/8/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPDMeasuredPhoto.h"
#import <UIKit/UIKit.h>
#import <CoreMedia/CoreMedia.h>

@interface MPDMeasuredPhoto (MPDMeasuredPhotoExt)

- (BOOL) writeImagetoJpeg:(CMSampleBufferRef)sampleBuffer withOrientation:(UIDeviceOrientation)orientation;
- (BOOL) writeAnnotationsToFile:(NSString*)jsonString;
- (BOOL) deleteAssociatedFiles;
- (NSString*) imageFileName;
- (NSString*) depthFileName;
- (NSString*) annotationsFileName;

@end
