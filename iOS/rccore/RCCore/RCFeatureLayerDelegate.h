//
//  RCFeatureLayerDelegate.h
//  RCCore
//
//  Created by Ben Hirashima on 7/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>

extern CGFloat const kMPFeatureRadius;
extern CGFloat const kMPFeatureFrameSize;

@interface RCFeatureLayerDelegate : NSObject

@property (nonatomic) UIColor* color;

@end