//
//  MPTitleTextBox.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MPDMeasuredPhoto+MPDMeasuredPhotoExt.h"

@interface MPTitleTextBox : UITextField

@property (nonatomic) MPDMeasuredPhoto* measuredPhoto;
@property (nonatomic) NSLayoutConstraint* widthConstraint;

@end
