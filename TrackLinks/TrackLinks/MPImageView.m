//
//  MPImageView.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 1/24/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPImageView.h"

const float kImageScale = 4.;

@implementation MPImageView

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code
    }
    return self;
}

- (void) setImageWithSampleBuffer:(CMSampleBufferRef)sampleBuffer
{
    if (sampleBuffer == nil) return;
    sampleBuffer = (CMSampleBufferRef)CFRetain(sampleBuffer);
    
    CVImageBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    
    pixelBuffer = (CVImageBufferRef)CFRetain(pixelBuffer);
    CFRelease(sampleBuffer);
    
    CIImage *ciImage = [CIImage imageWithCVPixelBuffer:pixelBuffer];
    
    CFRelease(pixelBuffer);
    
    CIContext *context = [CIContext contextWithOptions:nil];
    CGImageRef cgImageRef = [context createCGImage:ciImage fromRect:CGRectMake(0, 0, CVPixelBufferGetWidth(pixelBuffer), CVPixelBufferGetHeight(pixelBuffer))];
    UIImage *uiImage = [UIImage imageWithCGImage:cgImageRef scale:kImageScale orientation:UIImageOrientationRight];
    CFRelease(cgImageRef);
    
    [self setImage:uiImage];
}

@end
