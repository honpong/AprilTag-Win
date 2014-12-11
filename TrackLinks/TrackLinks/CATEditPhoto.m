
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

#import "CATEditPhoto.h"
#import "CATHttpInterceptor.h"
#import "CATCapturePhoto.h"
#import "CATConstants.h"
#import "CATNativeAction.h"
#import "NSString+RCString.h"

@interface CATEditPhoto ()

@property (nonatomic, readwrite) UIDeviceOrientation currentUIOrientation;

@end

@implementation CATEditPhoto
{
    BOOL isWebViewLoaded;
}

- (BOOL) prefersStatusBarHidden { return YES; }

- (void)viewDidLoad
{
    LOGME
    [super viewDidLoad];
      
    [CATHttpInterceptor setDelegate:self];
    
    NSURL *htmlUrl = [[NSBundle mainBundle] URLForResource:@"measured_photo_svg" withExtension:@"html"]; // url of the html file bundled with the app
    
    isWebViewLoaded = NO;
    
    // setup web view
    self.webView.scalesPageToFit = NO;
    self.webView.delegate = self;
    [self.webView loadRequest:[NSURLRequest requestWithURL:htmlUrl]];
}

- (void) viewWillAppear:(BOOL)animated
{
    
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
        NSString* javascript = [NSString stringWithFormat:@"loadMPhoto('%@', '%@', '%@', '%@', %i);", self.measuredPhoto.imageFileName, self.measuredPhoto.depthFileName, self.measuredPhoto.annotationsFileName, self.measuredPhoto.id_guid, units == UnitsMetric];
        [self.webView stringByEvaluatingJavaScriptFromString: javascript];
//        DLog(@"%@", javascript);
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
    else if ([request.URL.scheme isEqualToString:@"native"]) // do something on native://something links
    {
//        if ([request.URL.host isEqualToString:@"finish"]) [self finish];
        
        return NO; // indicates web view should not load the content of the link
    }
    else return NO; // disallow loading of http and all other types of links
}

#pragma mark - CATHttpInterceptorDelegate

- (NSDictionary *)handleAction:(CATNativeAction *)nativeAction error:(NSError **)error
{
    if ([nativeAction.request.URL.description endsWithString:@"/annotations/"] && [nativeAction.method isEqualToString:@"PUT"])
    {
        BOOL result = [self.measuredPhoto writeAnnotationsToFile:nativeAction.body];
        
        if (result == YES)
        {
            return @{ @"message": @"Annotations saved" };
        }
        else
        {
            NSDictionary* userInfo = @{ NSLocalizedDescriptionKey: @"Failed to write depth file" };
            *error = [NSError errorWithDomain:ERROR_DOMAIN code:500 userInfo:userInfo];
        }
    }
    else if ([nativeAction.request.URL.description endsWithString:@"/log/"] && [nativeAction.method isEqualToString:@"POST"])
    {
        [self webViewLog:[nativeAction.params objectForKey:@"message"]];
        return @{ @"message": @"Write to log successful" };
    }
    else
    {
       return @{ @"message": @"Invalid URL" };
    }
    
    return nil;
}

- (void) webViewLog:(NSString*)message
{
    if (message && message.length > 0) DLog(@"%@", message);
}

@end
