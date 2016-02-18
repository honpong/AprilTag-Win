//
//  MPWebViewController.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/24/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface MPWebViewController : UIViewController <UIWebViewDelegate>

@property (weak, nonatomic) IBOutlet UILabel *titleLabel;
@property (weak, nonatomic) IBOutlet UIButton *backButton;
@property (weak, nonatomic) IBOutlet UIWebView *webView;
@property (nonatomic) NSURL *htmlUrl;

- (IBAction)handleBackButton:(id)sender;

@end
