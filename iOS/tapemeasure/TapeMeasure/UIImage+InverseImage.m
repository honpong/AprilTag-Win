//
//  UIImage+InverseImage.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/20/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "UIImage+InverseImage.h"

@implementation UIImage (InverseImage)

- (UIImage*) invertedImage
{
    CIFilter* filter = [CIFilter filterWithName:@"CIColorInvert"];
    [filter setDefaults];
    [filter setValue:[[CIImage alloc] initWithCGImage:self.CGImage] forKey:kCIInputImageKey];
    UIImage* invertedImage = [[UIImage alloc] initWithCIImage:filter.outputImage scale:self.scale orientation:self.imageOrientation];
    return invertedImage;
}

@end
