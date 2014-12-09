//
//  MPDMeasuredPhoto.h
//  TrackLinks
//
//  Created by Ben Hirashima on 12/8/14.
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>

@interface MPDMeasuredPhoto : NSObject

@property (nonatomic) NSString * id_guid;
@property (nonatomic) NSTimeInterval created_at;

- (BOOL) writeImagetoJpeg:(CMSampleBufferRef)sampleBuffer withOrientation:(UIDeviceOrientation)orientation;
- (BOOL) writeAnnotationsToFile:(NSString*)jsonString;
- (BOOL) deleteAssociatedFiles;
- (NSString*) imageFileName;
- (NSString*) depthFileName;
- (NSString*) annotationsFileName;

@end
