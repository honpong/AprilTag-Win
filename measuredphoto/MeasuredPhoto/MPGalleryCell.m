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

- (void) setImage:(UIImage*)image
{
    [self.imgButton setImage:image forState:UIControlStateNormal];
    [self.imgButton.imageView setContentMode:UIViewContentModeScaleAspectFit];
}

- (void) setTitle:(NSString*)title
{
    [self.titleLabel setText:title];
}

- (IBAction)handleImageButton:(id)sender
{
    
}

@end
