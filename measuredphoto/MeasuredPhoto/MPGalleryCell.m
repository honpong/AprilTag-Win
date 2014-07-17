//
//  MPGalleryCell.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/16/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPGalleryCell.h"

@implementation MPGalleryCell

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code
    }
    return self;
}

- (UIImageView*) imageView
{
    return (UIImageView*)[self viewWithTag:1];
}

- (UILabel*) titleLabel
{
    return (UILabel*)[self viewWithTag:2];
}

- (void) setImage:(UIImage*)image
{
    [self.imageView setImage:image];
}

- (void) setTitle:(NSString*)title
{
    [self.titleLabel setText:title];
}

@end
