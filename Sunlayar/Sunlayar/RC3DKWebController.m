//
//  RC3DKWebController.m
//  Sunlayar
//
//  Created by Ben Hirashima on 2/20/15.
//  Copyright (c) 2015 Sunlayar. All rights reserved.
//

#import "RC3DKWebController.h"
#import "RCSensorDelegate.h"
#import "RCDebugLog.h"
#import "ARNativeAction.h"
#import "NSString+RCString.h"
#import "RCLocationManager.h"
#import "SLConstants.h"
#import "NSObject+SBJson.h"

@implementation RC3DKWebController
{
    id<RCSensorDelegate> sensorDelegate;
    BOOL useLocation;
}

- (void) viewDidLoad
{
    [super viewDidLoad];
    
//    [NSURLProtocol registerClass:[RCHttpInterceptor class]];
    [RCHttpInterceptor setDelegate:self];
    
    useLocation = [LOCATION_MANAGER isLocationExplicitlyAllowed] && [NSUserDefaults.standardUserDefaults boolForKey:PREF_USE_LOCATION];
    
    sensorDelegate = [SensorDelegate sharedInstance];
    
    _videoView = [[RCVideoPreview alloc] initWithFrame:self.view.frame];
    self.videoView.orientation = UIInterfaceOrientationLandscapeRight;
    [self.view addSubview:self.videoView];
    
    [[sensorDelegate getVideoProvider] setDelegate:self.videoView];
    
    // setup web view
    _webView = [UIWebView new];
    self.webView.scalesPageToFit = NO;
    self.webView.delegate = self;
    self.webView.opaque = NO;
    self.webView.backgroundColor = [UIColor clearColor];
    [self.view addSubview:self.webView];
    [self.view bringSubviewToFront:self.webView];
    [self.webView loadRequest:[NSURLRequest requestWithURL:pageURL]];
}

-(void)viewDidLayoutSubviews
{
    self.videoView.frame = self.view.frame;
    self.webView.frame = self.view.frame;
}

#pragma mark - 3DK Stuff

- (void) startSensors
{
    LOGME
    [sensorDelegate startAllSensors];
}

- (void)stopSensors
{
    LOGME
    [sensorDelegate stopAllSensors];
}

- (void)startSensorFusion
{
    [[UIApplication sharedApplication] setIdleTimerDisabled:YES];
    SENSOR_FUSION.delegate = self;
    [[sensorDelegate getVideoProvider] setDelegate:nil];
    if (useLocation) [SENSOR_FUSION setLocation:[LOCATION_MANAGER getStoredLocation]];
    [SENSOR_FUSION startSensorFusionWithDevice:[sensorDelegate getVideoDevice]];
}

- (void)stopSensorFusion
{
    [SENSOR_FUSION stopSensorFusion];
    [[sensorDelegate getVideoProvider] setDelegate:self.videoView];
    [[UIApplication sharedApplication] setIdleTimerDisabled:NO];
}

#pragma mark - RCSensorFusionDelegate

- (void) sensorFusionDidChangeStatus:(RCSensorFusionStatus *)status
{
    NSString* statusJson = [[status dictionaryRepresentation] JavascriptObjRepresentation]; // expensive
    NSString* javascript = [NSString stringWithFormat:@"sensorFusionDidChangeStatus(%@);", statusJson];
    DLog(@"%@", javascript);
    [self.webView stringByEvaluatingJavaScriptFromString: javascript];
}

- (void) sensorFusionDidUpdateData:(RCSensorFusionData*)data
{
    NSString* dataJson = [[data dictionaryRepresentation] JavascriptObjRepresentation]; // expensive
    NSString* javascript = [NSString stringWithFormat:@"sensorFusionDidUpdateData(%@);", dataJson];
//    DLog(@"%@", javascript);
    [self.webView stringByEvaluatingJavaScriptFromString: javascript];
    
    if(data.sampleBuffer) [self.videoView displaySensorFusionData:data];
}

#pragma mark - UIWebViewDelegate

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
    DLog(@"%@", nativeAction.request.URL.description);
    
    if ([nativeAction.request.URL.description endsWithString:@"/startSensors/"])
    {
        [self startSensors];
        return @{ @"message": @"Starting all sensors" };
    }
    else if ([nativeAction.request.URL.description endsWithString:@"/stopSensors/"])
    {
        [self stopSensors];
        return @{ @"message": @"Stopping all sensors" };
    }
    else if ([nativeAction.request.URL.description endsWithString:@"/startSensorFusion/"])
    {
        [self startSensorFusion];
        return @{ @"message": @"Starting sensor fusion" };
    }
    else if ([nativeAction.request.URL.description endsWithString:@"/stopSensorFusion/"])
    {
        [self stopSensorFusion];
        return @{ @"message": @"Stopping sensor fusion" };
    }
    
    return nil;
}

- (void) webViewLog:(NSString*)message
{
    if (message && message.length > 0) DLog(@"%@", message);
}


@end
