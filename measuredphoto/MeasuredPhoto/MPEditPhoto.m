//
//  MPEditPhoto.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 3/18/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPEditPhoto.h"
#import <RCCore/RCCore.h>

@interface MPEditPhoto ()
@property (nonatomic, readwrite) UIWebView* webView;
@end

@implementation MPEditPhoto
{
    NSURL* cachedHtmlUrl;
}
@synthesize sfData;

- (void)viewDidLoad
{
    [super viewDidLoad];
    
//    NSURL *htmlUrl = [[NSBundle mainBundle] URLForResource:@"edit" withExtension:@"html"]; // url of the html file bundled with the app
//    cachedHtmlUrl = [CACHE_DIRECTORY_URL URLByAppendingPathComponent:@"edit.html"]; // url where we keep a cached version of it
    cachedHtmlUrl = [NSURL URLWithString:@"https://internal.realitycap.com/test_measuredphoto/"];
    
    // copy bundled html file to cache dir
//    NSError* error;
//    [[NSFileManager defaultManager] copyItemAtURL:htmlUrl toURL:cachedHtmlUrl error:&error];
//    if (error)
//        DLog(@"FAILED TO COPY HTML FILE: %@", error); // TODO: better error handling
    
    // setup web view
    self.webView = [[UIWebView alloc] init];
    self.webView.backgroundColor = [UIColor whiteColor];
    self.webView.scalesPageToFit = NO;
    self.webView.delegate = self;
    [self.view addSubview:self.webView];
    [self.webView addMatchSuperviewConstraints];
    
    [self.webView loadRequest:[NSURLRequest requestWithURL:cachedHtmlUrl]];
}

- (void)viewWillAppear:(BOOL)animated
{
    self.webView.delegate = self; // setup the delegate as the web view is shown
}

- (void)viewWillDisappear:(BOOL)animated
{
    [self.webView stopLoading]; // in case the web view is still loading its content
    self.webView.delegate = nil; // disconnect the delegate as the webview is hidden
    [UIApplication sharedApplication].networkActivityIndicatorVisible = NO;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return NO;
}

- (NSUInteger)supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskPortrait | UIInterfaceOrientationMaskPortraitUpsideDown;
}

// this helps dismiss the keyboard when the "Done" button is clicked
- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    [textField resignFirstResponder];
    [self.webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:[textField text]]]];
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
    
    [webView stringByEvaluatingJavaScriptFromString: @"setMessage('Hello, sucka')"];
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
    DLogs(request.URL.description);
    
//    if ([request.URL.scheme isEqualToString:@"file"])
//    {
//        return YES; // allow loading local files
//    }
//    else if ([request.URL.scheme isEqualToString:@"native"]) // do something on native://something links
//    {
//        if ([request.URL.host isEqualToString:@"finish"]) [self finish];
//        
//        return NO; // indicates web view should not load the content of the link
//    }
//    else return NO; // disallow loading of http and all other types of links
    
    return YES; // temp for testing
}

// called when navigating away from this view controller
- (void) finish
{
    // delete cached html and photo
    NSError* error;
    BOOL exists = [[NSFileManager defaultManager] fileExistsAtPath:cachedHtmlUrl.path];
    if (exists) [[NSFileManager defaultManager] removeItemAtPath:cachedHtmlUrl.path error:&error];
    if (error)
        DLog(@"FAILED TO DELETE HTML FILE: %@", error); // TODO: better error handling
    
    error = nil;
    exists = [[NSFileManager defaultManager] fileExistsAtPath:CACHED_PHOTO_URL.path];
    if (exists) [[NSFileManager defaultManager] removeItemAtPath:CACHED_PHOTO_URL.path error:&error];
    if (error)
        DLog(@"FAILED TO DELETE PHOTO: %@", error); // TODO: better error handling
    
    if ([self.delegate respondsToSelector:@selector(didFinishEditingPhoto)]) [self.delegate didFinishEditingPhoto];
}

@end
