//
//  MPEditPhoto.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 3/18/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPViewController.h"
#import <RC3DK/RC3DK.h>

@protocol MPEditPhotoDelegate <NSObject>

- (void) didFinishEditingPhoto;

@end

@interface MPEditPhoto : MPViewController <UIWebViewDelegate>

@property (nonatomic) id<MPEditPhotoDelegate> delegate;
@property (nonatomic, readonly) UIWebView *webView;
@property (nonatomic) RCSensorFusionData* sfData;

@end
