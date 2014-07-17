//
//  MPGalleryCell.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/16/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface MPGalleryCell : UICollectionViewCell

- (UIImageView*) imageView;
- (UILabel*) titleLabel;

- (void) setImage:(UIImage*)image;
- (void) setTitle:(NSString*)title;

@end
