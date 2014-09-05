//
//  RCThreeButtonSlideView.h
//  RCCore
//
//  Created by Ben Hirashima on 9/5/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCSlideBanner.h"
#import "RCInvertButton.h"

@interface RCThreeButtonSlideView : RCSlideBanner

@property (nonatomic) UILabel* label;
@property (nonatomic) RCInvertButton* button1;
@property (nonatomic) RCInvertButton* button2;
@property (nonatomic) RCInvertButton* button3;

- (void) setPrimaryColor:(UIColor*)color;

@end
