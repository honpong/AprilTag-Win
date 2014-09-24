//
//  MPTipsView.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPTipsView.h"

@implementation MPTipsView

- (id)initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame])
    {
        NSURL *htmlUrl = [[NSBundle mainBundle] URLForResource:@"tips" withExtension:@"html"]; // url of the html file bundled with the app
        
        // setup web view
        _webView = [[UIWebView alloc] init];
        self.webView.backgroundColor = [UIColor colorWithWhite:.85 alpha:1.];
        self.webView.scalesPageToFit = NO;
        [self addSubview:self.webView];
        [self.webView addMatchSuperviewConstraints];
        
        [self.webView loadRequest:[NSURLRequest requestWithURL:htmlUrl]];
    }
    return self;
}

@end
