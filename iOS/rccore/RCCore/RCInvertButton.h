//
//  RCInvertButton.h
//  RCCore
//
//  Created by Ben Hirashima on 8/26/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface RCInvertButton : UIButton
{
    UIColor* originalBgColor;
    UIColor* originalTextColor;
}

@property (nonatomic) UIColor* invertedColor;

@end
