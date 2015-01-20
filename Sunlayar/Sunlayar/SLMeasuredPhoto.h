//
//  Created by Ben Hirashima on 12/29/14.
//  Copyright (c) 2014 Sunlayar. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>

@interface SLMeasuredPhoto : NSObject

@property (nonatomic) NSString * id_guid;
@property (nonatomic) NSTimeInterval created_at;

- (BOOL) writeImagetoJpeg:(CMSampleBufferRef)sampleBuffer withOrientation:(UIDeviceOrientation)orientation;
- (BOOL) writeAnnotationsToFile:(NSString*)jsonString;
- (BOOL) deleteAssociatedFiles;
- (NSDictionary*) getRoofJsonDict;

- (NSString*) imageFileName;
- (NSString*) depthFileName;
- (NSString*) annotationsFileName;

@end
