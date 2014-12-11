//
//    Copyright (c) 2014, RealityCap, Inc.
//    All rights reserved.
//
//    BSD License
//
//    Redistribution and use in source and binary forms, with or without
//    modification, are permitted provided that the following conditions are met:
//    * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//    * Neither the name of RealityCap, Inc., nor the
//    names of its contributors may be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
//    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//    DISCLAIMED. IN NO EVENT SHALL RealityCap, Inc. BE LIABLE FOR ANY
//    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#import "UIImage+RCImageFile.h"

@implementation UIImage (RCImageFile)

+ (NSData*) jpegDataFromSampleBuffer:(CMSampleBufferRef)sampleBuffer withOrientation:(UIImageOrientation)orientation
{
    if (sampleBuffer)
    {
        sampleBuffer = (CMSampleBufferRef)CFRetain(sampleBuffer);
    }
    else
    {
        return nil;
    }
    
    //inside RCSensorFusionData we have a CMSampleBufferRef from which we can get CVImageBufferRef
    CVImageBufferRef pixBuf = CMSampleBufferGetImageBuffer(sampleBuffer);
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
    
    CFRelease(videoImage);
    CFRelease(sampleBuffer);
    CFRelease(rotatedImage);
    
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
