//
//  MPAboutView.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface MPAboutView : UIView <UIWebViewDelegate>

@property (nonatomic, readonly) UIWebView* webView;

@end
