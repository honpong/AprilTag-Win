
//  Copyright (c) 2015 Sunlayar. All rights reserved.
//

#import "SLRoofController.h"
#import "SLConstants.h"
#import "NSString+RCString.h"
#import "RCDebugLog.h"
#import "SLAugRealityController.h"

@implementation SLRoofController
{
    BOOL isWebViewLoaded;
}

- (instancetype)init
{
    self = [super init];
    if (!self) return nil;
    
    _webView = [UIWebView new];
        
    return self;
}

- (BOOL) prefersStatusBarHidden { return YES; }

- (void)viewDidLoad
{
    LOGME
    [super viewDidLoad];
    
    [RCHttpInterceptor setDelegate:self];
          
    NSURL *htmlUrl = [[NSBundle mainBundle] URLForResource:@"roof_definition" withExtension:@"html"]; // url of the html file bundled with the app
    
    isWebViewLoaded = NO;
    
    // setup web view
    self.webView.scalesPageToFit = NO;
    self.webView.delegate = self;
    [self.view addSubview:self.webView];
    [self.webView loadRequest:[NSURLRequest requestWithURL:htmlUrl]];
}


- (void)viewDidLayoutSubviews
{
    self.webView.frame = self.view.frame;
}

- (void) viewDidDisappear:(BOOL)animated
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.webView stringByEvaluatingJavaScriptFromString:@"clear_all();"];
    });
}

- (void) setMeasuredPhoto:(SLMeasuredPhoto *)measuredPhoto
{
    _measuredPhoto = measuredPhoto;
}

#pragma mark - Orientation

- (NSUInteger)supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskLandscapeRight;
}

#pragma mark - Misc

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

- (void) finishAndGotoNext
{
    // prevents crash
    self.webView.delegate = nil;
    [self.webView stopLoading];
    
    SLAugRealityController* arController = [SLAugRealityController new];
    arController.measuredPhoto = self.measuredPhoto;
    self.view.window.rootViewController = arController;
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

#pragma mark - RCHttpInterceptorDelegate

- (NSDictionary *)handleAction:(ARNativeAction *)nativeAction error:(NSError **)error
{
    if ([nativeAction.request.URL.relativePath endsWithString:@"/annotations"] && [nativeAction.method isEqualToString:@"PUT"])
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
    else if ([nativeAction.request.URL.relativePath endsWithString:@"/next"])
    {
        dispatch_async(dispatch_get_main_queue(), ^{
            [self finishAndGotoNext];
        });
        
        return @{ @"message": @"Proceeding to next" };
    }
    else if ([nativeAction.request.URL.relativePath endsWithString:@"/log"] && [nativeAction.method isEqualToString:@"POST"])
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
