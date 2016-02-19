//
//  MPWebViewController.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/24/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPWebViewController.h"

@interface MPWebViewController ()

@end

@implementation MPWebViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    self.webView.delegate = self;
    if (self.htmlUrl) [self.webView loadRequest:[NSURLRequest requestWithURL:self.htmlUrl]];
}

- (IBAction)handleBackButton:(id)sender
{
    [self.presentingViewController dismissViewControllerAnimated:YES completion:nil];
}

- (void) setHtmlUrl:(NSURL *)htmlUrl
{
    _htmlUrl = htmlUrl;
    [self.webView loadRequest:[NSURLRequest requestWithURL:htmlUrl]];
}

#pragma mark - UIWebViewDelegate

- (BOOL) webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
    if ([request.URL.scheme isEqualToString:@"file"])
    {
        return YES;
    }
    else
    {
        [[UIApplication sharedApplication] openURL:request.URL];
        return NO;
    }
}

@end
