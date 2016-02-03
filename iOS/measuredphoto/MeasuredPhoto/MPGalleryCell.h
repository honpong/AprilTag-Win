//
//  MPGalleryCell.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/16/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface MPGalleryCell : UICollectionViewCell

@property (weak, nonatomic) IBOutlet UIButton *imgButton;
@property (weak, nonatomic) IBOutlet UILabel *titleLabel;
@property (nonatomic) NSString* guid;

- (void) setImage:(UIImage*)image;
- (void) setTitle:(NSString*)title;

- (IBAction)handleImageButton:(id)sender;

@end
