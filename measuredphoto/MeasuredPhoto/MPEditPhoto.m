//
//  MPEditPhoto.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 3/18/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPEditPhoto.h"
#import "UIView+MPConstraints.h"

@interface MPEditPhoto ()

@end

@implementation MPEditPhoto
@synthesize webView;

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    webView = [[UIWebView alloc] init];
    webView.backgroundColor = [UIColor whiteColor];
    webView.scalesPageToFit = NO;
    webView.delegate = self;
    [self.view addSubview:self.webView];
    [webView addMatchSuperviewConstraints];
    
    NSURL *url = [[NSBundle mainBundle] URLForResource:@"edit" withExtension:@"html"];
    [webView loadRequest:[NSURLRequest requestWithURL:url]];
}

- (void)viewWillAppear:(BOOL)animated
{
    self.webView.delegate = self; // setup the delegate as the web view is shown
}

- (void)viewWillDisappear:(BOOL)animated
{
    [webView stopLoading]; // in case the web view is still loading its content
    webView.delegate = nil; // disconnect the delegate as the webview is hidden
    [UIApplication sharedApplication].networkActivityIndicatorVisible = NO;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // we support rotation in this view controller
    return YES;
}

// this helps dismiss the keyboard when the "Done" button is clicked
- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    [textField resignFirstResponder];
    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:[textField text]]]];
    return YES;
}

#pragma mark -
#pragma mark UIWebViewDelegate

- (void)webViewDidStartLoad:(UIWebView *)webView
{
    // starting the load, show the activity indicator in the status bar
    [UIApplication sharedApplication].networkActivityIndicatorVisible = YES;
}

- (void)webViewDidFinishLoad:(UIWebView *)webView
{
    // finished loading, hide the activity indicator in the status bar
    [UIApplication sharedApplication].networkActivityIndicatorVisible = NO;
    
    [self.webView stringByEvaluatingJavaScriptFromString: @"setMessage('Hello, sucka')"];
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

- (BOOL) webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
    DLog(@"shouldStartLoadWithRequest: %@", request.URL);
    
    if ([request.URL.scheme isEqualToString:@"native"])
    {
        if ([request.URL.host isEqualToString:@"finish"]) [self finish];
            
        return NO;
    }
    else return YES;
}

- (void) finish
{
    if ([self.delegate respondsToSelector:@selector(didFinishEditingPhoto)]) [self.delegate didFinishEditingPhoto];
}

@end
