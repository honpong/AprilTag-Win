//
//  TMMeasurementTypeCell.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 4/2/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface TMMeasurementTypeCell : UICollectionViewCell

@property (strong, nonatomic) IBOutlet UIButton *button;
@property (strong, nonatomic) IBOutlet UILabel *label;
@property MeasurementType type;

- (void) setImage: (UIImage*)image;
- (void) setText: (NSString*)text;

@end
