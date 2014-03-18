//
//  MPEditPhoto.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 3/18/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPViewController.h"

@protocol MPEditPhotoDelegate <NSObject>

- (void) didFinishEditingPhoto;

@end

@interface MPEditPhoto : MPViewController <UIWebViewDelegate>

@property (nonatomic) id<MPEditPhotoDelegate> delegate;
@property (nonatomic) UIWebView *webView;

@end
