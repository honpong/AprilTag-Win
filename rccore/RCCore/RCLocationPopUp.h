//
//  RCLocationPopUp.h
//  RCCore
//
//  Created by Ben Hirashima on 9/5/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCThreeButtonSlideView.h"

@protocol RCLocationPopUpDelegate <NSObject>

- (void) handleAllowLocationButton;
- (void) handleLaterLocationButton;
- (void) handleNeverLocationButton;

@end

@interface RCLocationPopUp : RCThreeButtonSlideView

@property (weak, nonatomic) id<RCLocationPopUpDelegate> delegate;

@end
