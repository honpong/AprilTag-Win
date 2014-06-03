//
//  TMAboutView.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/9/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "TMAboutView.h"

@interface TMAboutView ()

@property (nonatomic) UIWebView* webView;

@end

@implementation TMAboutView
{
    NSURL *htmlUrl;
}

- (id)initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame])
    {
        htmlUrl = [[NSBundle mainBundle] URLForResource:@"about" withExtension:@"html"]; // url of the html file bundled with the app
        
        // setup web view
        self.webView = [[UIWebView alloc] init];
        self.webView.backgroundColor = [UIColor colorWithWhite:.85 alpha:1.];
        self.webView.scalesPageToFit = NO;
        self.webView.delegate = self;
        [self addSubview:self.webView];
        [self.webView addMatchSuperviewConstraints];
        
        [self.webView loadRequest:[NSURLRequest requestWithURL:htmlUrl]];
    }
    return self;
}

#pragma mark - UIWebViewDelegate

- (void)webViewDidStartLoad:(UIWebView *)webView
{
    // starting the load, show the activity indicator in the status bar
    [UIApplication sharedApplication].networkActivityIndicatorVisible = YES;
}

- (void)webViewDidFinishLoad:(UIWebView *)webView
{
    // finished loading, hide the activity indicator in the status bar
    [UIApplication sharedApplication].networkActivityIndicatorVisible = NO;
}

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error
{
    // load error, hide the activity indicator in the status bar
    [UIApplication sharedApplication].networkActivityIndicatorVisible = NO;
    
    // report the error inside the webview
    NSString* errorString = [NSString stringWithFormat:
                             @"<html><center><font size=+5 color='red'>An error occurred:<br>%@</font></center></html>",
                             error.localizedDescription];
    [self.webView loadHTMLString:errorString baseURL:nil];
}

// called when user taps a link on the page
- (BOOL) webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
    DLog(@"%@", request.URL.description);
    
    if ([request.URL isEqual:htmlUrl])
    {
        return YES;
    }
    else
    {
        [[UIApplication sharedApplication] openURL:request.URL];
    }
    
    return NO;
}

@end
