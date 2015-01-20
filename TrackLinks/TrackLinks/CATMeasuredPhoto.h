
//  Copyright (c) 2014 Caterpillar. All rights reserved.


#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>

@interface CATMeasuredPhoto : NSObject

@property (nonatomic) NSString * id_guid;
@property (nonatomic) NSTimeInterval created_at;

- (BOOL) writeImagetoJpeg:(CMSampleBufferRef)sampleBuffer withOrientation:(UIDeviceOrientation)orientation;
- (BOOL) deleteAssociatedFiles;
- (NSString*) imageFileName;
- (NSString*) depthFileName;

@end
