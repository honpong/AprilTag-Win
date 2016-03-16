//
//  TMTipsView.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/9/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "TMTipsView.h"
#import <RCCore/RCCore.h>

@interface TMTipsView ()

@property (nonatomic) UIWebView* webView;

@end

@implementation TMTipsView

- (id)initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame])
    {
        NSURL *htmlUrl = [[NSBundle mainBundle] URLForResource:@"tips" withExtension:@"html"]; // url of the html file bundled with the app
        
        // setup web view
        self.webView = [[UIWebView alloc] init];
        self.webView.backgroundColor = [UIColor colorWithWhite:.85 alpha:1.];
        self.webView.scalesPageToFit = NO;
        [self addSubview:self.webView];
        [self.webView addMatchSuperviewConstraints];
        
        [self.webView loadRequest:[NSURLRequest requestWithURL:htmlUrl]];
    }
    return self;
}

@end
