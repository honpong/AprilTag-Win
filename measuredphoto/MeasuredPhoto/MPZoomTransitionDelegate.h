//
//  MPZoomTransitionDelegate.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/24/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol MPZoomTransitionFromView <NSObject>

@property (nonatomic, readonly) UIView* transitionFromView;

@end

@interface MPZoomTransitionDelegate : NSObject <UIViewControllerTransitioningDelegate>

@end
