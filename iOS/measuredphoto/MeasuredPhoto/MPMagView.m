//
//  MPMagView.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 1/20/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPMagView.h"

#define IMAGE_SCALE 4.

@implementation MPMagView
@synthesize photo;

- (id)initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame])
    {
        self.backgroundColor = [UIColor redColor];
        self.clipsToBounds = YES;
        
        photo = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, 480*IMAGE_SCALE, 640*IMAGE_SCALE)];
        photo.backgroundColor = [UIColor yellowColor];
        [self addSubview:photo];
    }
    return self;
}

- (void) layoutSubviews
{
    [super layoutSubviews];
}

- (void) setPhotoWithSampleBuffer:(CMSampleBufferRef)sampleBuffer
{
    if (sampleBuffer == nil) return;
    
    CVImageBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    if(!pixelBuffer) return;
    
    CIImage *ciImage = [CIImage imageWithCVPixelBuffer:pixelBuffer];
    
    CIContext *context = [CIContext contextWithOptions:nil];
    CGImageRef cgImageRef = [context createCGImage:ciImage fromRect:CGRectMake(0, 0, CVPixelBufferGetWidth(pixelBuffer), CVPixelBufferGetHeight(pixelBuffer))];
    UIImage *uiImage = [UIImage imageWithCGImage:cgImageRef scale:IMAGE_SCALE orientation:UIImageOrientationRight];
    CFRelease(cgImageRef);
    
    [photo setImage:uiImage];
}

@end
