//
//  MPEditPhoto.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 3/18/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPEditPhoto.h"
#import <RCCore/RCCore.h>
#import "MPDMeasuredPhoto+MPDMeasuredPhotoExt.h"
#import "NativeAction.h"

@interface MPEditPhoto ()
@property (nonatomic, readwrite) UIWebView* webView;
@end

@implementation MPEditPhoto
{
}
@synthesize sfData;

- (void)dealloc
{
    [NSURLCache setJavascriptBridgeDelegate:nil];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleOrientationChange)
                                                 name:UIDeviceOrientationDidChangeNotification
                                               object:nil];
    
    JavascriptBridgeURLCache *cache = [[JavascriptBridgeURLCache alloc] initWithHost:API_HOST];
    [NSURLCache setSharedURLCache:cache];
    [NSURLCache setJavascriptBridgeDelegate:self];
    
    NSURL *htmlUrl = [[NSBundle mainBundle] URLForResource:@"test" withExtension:@"html"]; // url of the html file bundled with the app
    
    // setup web view
    self.webView = [[UIWebView alloc] init];
    self.webView.backgroundColor = [UIColor whiteColor];
    self.webView.scalesPageToFit = NO;
    self.webView.delegate = self;
    [self.view addSubview:self.webView];
    [self.webView addMatchSuperviewConstraints];
    
    [self.webView loadRequest:[NSURLRequest requestWithURL:htmlUrl]];
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
    return UIInterfaceOrientationMaskPortrait;
}

- (void) handleOrientationChange
{
    UIDeviceOrientation newOrientation = [[UIDevice currentDevice] orientation];
    
    if (newOrientation == UIDeviceOrientationPortrait || newOrientation == UIDeviceOrientationPortraitUpsideDown || newOrientation == UIDeviceOrientationLandscapeLeft || newOrientation == UIDeviceOrientationLandscapeRight)
    {
        NSString* jsFunction = [NSString stringWithFormat:@"forceOrientationChange(%i)", newOrientation];
        [self.webView stringByEvaluatingJavaScriptFromString: jsFunction];
    }
}

// this helps dismiss the keyboard when the "Done" button is clicked
- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    [textField resignFirstResponder];
    [self.webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:[textField text]]]];
    return YES;
}

#pragma mark - UIWebViewDelegate

- (void)webViewDidStartLoad:(UIWebView *)webView
{
//    [UIApplication sharedApplication].networkActivityIndicatorVisible = YES;
}

- (void)webViewDidFinishLoad:(UIWebView *)webView
{
//    [UIApplication sharedApplication].networkActivityIndicatorVisible = NO;
    
    if (self.measuredPhoto)
    {
        NSString* fileBaseName = [[RCStereo sharedInstance] fileBaseName];
        NSString* depthFilename = [fileBaseName stringByAppendingString:@".json"];
        NSString* photoFilename = [self.measuredPhoto imageFileName];
        
        [webView stringByEvaluatingJavaScriptFromString: [NSString stringWithFormat:@"main('%@', '%@')", photoFilename, depthFilename]];
    }
    else
    {
        DLog(@"ERROR: Failed to load web view because measuredPhoto is nil");
    }
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
    NSString *body = [[NSString alloc] initWithData:[request HTTPBody] encoding:NSUTF8StringEncoding];
    DLogs(body);
    
    if ([request.URL.scheme isEqualToString:@"file"])
    {
        return YES; // allow loading local files
    }
    else if ([request.URL.scheme isEqualToString:@"native"]) // do something on native://something links
    {
        if ([request.URL.host isEqualToString:@"finish"]) [self finish];
        
        return NO; // indicates web view should not load the content of the link
    }
    else return NO; // disallow loading of http and all other types of links
}

// called when navigating away from this view controller
- (void) finish
{
    if ([self.delegate respondsToSelector:@selector(didFinishEditingPhoto)]) [self.delegate didFinishEditingPhoto];
}

#pragma mark - JavascriptBridgeDelegate

- (NSDictionary *)handleAction:(NativeAction *)nativeAction error:(NSError **)error {
    // For this demo, we'll handle two types of requests. The first will simply show a native UIAlertView with params
    // passed from Javascript, and the second will go fetch the contacts from our address book and pass names and phone
    // numbers back to Javascript.
    
    // -------- GET /alert
//    if ([nativeAction.action isEqualToString:@"test"]) {
//        //Typically, this request is sent to native code on the Web Thread, so if we want to do something that is
//        // going to draw to the screen from native code, we need to run it on the main thread.
//        dispatch_async(dispatch_get_main_queue(), ^{
//            UIAlertView *alertView = [[UIAlertView alloc] initWithTitle:nativeAction.action message:[nativeAction.params objectForKey:@"message"] delegate:nil cancelButtonTitle:@"Ok" otherButtonTitles:nil];
//            [alertView show];
//        });
//        return nil;
//    }
    
    
//    if ([nativeAction.action isEqualToString:@"test"] && [nativeAction.method isEqualToString:@"POST"])
    if ([nativeAction.action isEqualToString:@"test"])
    {
        NSString* message = [nativeAction.params objectForKey:@"message"];
        message = message ? message : @"<null>";
        return @{ @"message": message };
    }
    
    return nil;
}

@end
