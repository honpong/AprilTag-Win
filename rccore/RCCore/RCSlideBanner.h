//
//  RCSlideBanner.h
//  RCCore
//
//  Created by Ben Hirashima on 8/21/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

typedef NS_ENUM(int, RCSlideBannerState) {
    RCSlideBannerStateShowing,
    RCSlideBannerStateAnimating,
    RCSlideBannerStateHidden
};

@interface RCSlideBanner : UIView

@property (nonatomic, readonly) RCSlideBannerState state;

- (void) showInstantly;
- (void) showAnimated;
- (void) hideAnimated;
- (void) hideWithDelay:(float)secs onCompletion:(void (^)(BOOL finished))completionBlock;
- (void) hideInstantly;
- (void) setHeightConstraint:(CGFloat)height;
- (void) setBottomSpaceToSuperviewConstraint:(CGFloat)distance;

@end