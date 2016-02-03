//
//  UIImage+MPImageFile.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 3/18/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "UIImage+MPImageFile.h"

@implementation UIImage (MPImageFile)

+ (NSData*) jpegDataFromSampleBuffer:(CMSampleBufferRef)sampleBuffer withOrientation:(UIImageOrientation)orientation
{
    if (!sampleBuffer) return nil;
    
    //inside RCSensorFusionData we have a CMSampleBufferRef from which we can get CVImageBufferRef
    CVImageBufferRef pixBuf = CMSampleBufferGetImageBuffer(sampleBuffer);
    if(!pixBuf) return nil;
    //we then need to turn that CVImageBuffer into an UIImage, which can be written to NSData as a JPG
    CIImage *ciImage = [CIImage imageWithCVPixelBuffer:pixBuf];
    
    CIContext *temporaryContext = [CIContext contextWithOptions:nil];
    CGImageRef videoImage = [temporaryContext
                             createCGImage:ciImage
                             fromRect:CGRectMake(0, 0,
                                                 CVPixelBufferGetWidth(pixBuf),
                                                 CVPixelBufferGetHeight(pixBuf))];
    
    CGImageRef rotatedImage = [self CreateCGImageRotatedByAngle:videoImage angle:[self getRotationAngleFromOrientation:orientation]];
    
    UIImage *uiImage = [UIImage imageWithCGImage:rotatedImage scale:1. orientation:UIImageOrientationUp];
    
    CFRelease(rotatedImage);
    CFRelease(videoImage);
    
    return UIImageJPEGRepresentation(uiImage, 0.8);
}

+ (CGImageRef)CreateCGImageRotatedByAngle:(CGImageRef)imgRef angle:(CGFloat)angle CF_RETURNS_RETAINED
{
    CGFloat angleInRadians = angle;
    
    CGFloat width = CGImageGetWidth(imgRef);
    CGFloat height = CGImageGetHeight(imgRef);
    
    CGRect imgRect = CGRectMake(0, 0, width, height);
    CGAffineTransform transform = CGAffineTransformMakeRotation(angleInRadians);
    CGRect rotatedRect = CGRectApplyAffineTransform(imgRect, transform);
    
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef bmContext = CGBitmapContextCreate(NULL,
                                                   rotatedRect.size.width,
                                                   rotatedRect.size.height,
                                                   8,
                                                   0,
                                                   colorSpace,
                                                   kCGImageAlphaPremultipliedFirst);
    CGContextSetAllowsAntialiasing(bmContext, YES);
    CGContextSetInterpolationQuality(bmContext, kCGInterpolationHigh);
    CGColorSpaceRelease(colorSpace);
    CGContextTranslateCTM(bmContext,
                          +(rotatedRect.size.width/2),
                          +(rotatedRect.size.height/2));
    CGContextRotateCTM(bmContext, angleInRadians);
    CGContextDrawImage(bmContext, CGRectMake(-width/2, -height/2, width, height),
                       imgRef);
    
    CGImageRef rotatedImage = CGBitmapContextCreateImage(bmContext);
    CFRelease(bmContext);
    
    return rotatedImage;
}

+ (CGFloat) getRotationAngleFromOrientation:(UIImageOrientation)orientation
{
    switch (orientation)
    {
        case UIImageOrientationUp:
            return 0;
            break;
        case UIImageOrientationDown:
            return M_PI;
            break;
        case UIImageOrientationRight:
            return -M_PI_2;
            break;
        case UIImageOrientationLeft:
            return M_PI_2;
            break;
            
        default:
            return 0;
            break;
    }
}

@end
