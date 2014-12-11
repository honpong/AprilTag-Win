//
//  UIImage+MPImageFile.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 3/18/14.
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>

@interface UIImage (RCImageFile)

+ (NSData*) jpegDataFromSampleBuffer:(CMSampleBufferRef)sampleBuffer withOrientation:(UIImageOrientation)orientation;

@end
