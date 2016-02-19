//
//  UIView+RCAutoLayoutDebugging.m
//  DistanceLabelTest
//
//  Created by Ben Hirashima on 10/2/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "UIView+RCAutoLayoutDebugging.h"
@import ObjectiveC;

@implementation UIView (RCAutoLayoutDebugging)

- (void)setAutoLayoutNameTag:(NSString *)nameTag
{
    objc_setAssociatedObject(self, "rc_nameTag", nameTag, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (NSString *)autoLayoutNameTag
{
    return objc_getAssociatedObject(self, "rc_nameTag");
}

@end
