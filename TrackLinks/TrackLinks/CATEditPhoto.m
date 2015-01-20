
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

#import "CATEditPhoto.h"
#import "CATCapturePhoto.h"
#import "CATConstants.h"
#import "NSString+RCString.h"

@implementation CATEditPhoto
{
    BOOL isWebViewLoaded;
}

- (BOOL) prefersStatusBarHidden { return YES; }

- (void)viewDidLoad
{
    LOGME
    [super viewDidLoad];
          
    NSURL *htmlUrl = [[NSBundle mainBundle] URLForResource:@"measured_photo_svg" withExtension:@"html"]; // url of the html file bundled with the app
    
    isWebViewLoaded = NO;
    
    // setup web view
    self.webView.scalesPageToFit = NO;
    self.webView.delegate = self;
    [self.webView loadRequest:[NSURLRequest requestWithURL:htmlUrl]];
}

- (void) viewDidDisappear:(BOOL)animated
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.webView stringByEvaluatingJavaScriptFromString:@"clear_all();"];
    });
}

- (void) setMeasuredPhoto:(CATMeasuredPhoto *)measuredPhoto
{
    _measuredPhoto = measuredPhoto;
}

#pragma mark - Orientation

- (NSUInteger)supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskLandscapeRight;
}

#pragma mark - Event handlers

- (IBAction)handleCameraButton:(id)sender
{
    [self.measuredPhoto deleteAssociatedFiles];
    [self.presentingViewController dismissViewControllerAnimated:NO completion:nil];
}

#pragma mark -

- (void) loadMeasuredPhoto
{
    if (self.measuredPhoto)
    {
        Units units = (Units)[NSUserDefaults.standardUserDefaults integerForKey:PREF_UNITS];
        NSString* javascript = [NSString stringWithFormat:@"loadMPhoto('%@', '%@', '%@', '%@', %i);", self.measuredPhoto.imageFileName, self.measuredPhoto.depthFileName, nil, self.measuredPhoto.id_guid, units == UnitsMetric];
        [self.webView stringByEvaluatingJavaScriptFromString: javascript];
    }
    else
    {
        DLog(@"ERROR: Failed to load web view because measuredPhoto is nil");
    }
}

#pragma mark - UIWebViewDelegate

- (void)webViewDidFinishLoad:(UIWebView *)webView
{
    isWebViewLoaded = YES;
    [self loadMeasuredPhoto];
}

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error
{
    // report the error inside the webview
    NSString* errorString = [NSString stringWithFormat:
                             @"<html><center><font size=+5 color='red'>An error occurred:<br>%@</font></center></html>",
                             error.localizedDescription];
    [self.webView loadHTMLString:errorString baseURL:nil];
}

// called when user taps a link on the page
- (BOOL) webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
    if ([request.URL.scheme isEqualToString:@"file"])
    {
        return YES; // allow loading local files
    }
    else return NO; // disallow loading of http and all other types of links
}

@end
