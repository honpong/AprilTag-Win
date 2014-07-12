//
//  MPEditPhoto.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 3/18/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPViewController.h"
#import <RC3DK/RC3DK.h>
#import "JavascriptBridgeURLCache.h"

@protocol MPEditPhotoDelegate <NSObject>

- (void) didFinishEditingPhoto;

@end

@class MPDMeasuredPhoto;

@interface MPEditPhoto : MPViewController <UIWebViewDelegate, JavascriptBridgeDelegate>

@property (nonatomic) id<MPEditPhotoDelegate> delegate;
@property (nonatomic, readonly) UIWebView *webView;
@property (nonatomic) RCSensorFusionData* sfData;
@property (nonatomic) MPDMeasuredPhoto* measuredPhoto;

@end
