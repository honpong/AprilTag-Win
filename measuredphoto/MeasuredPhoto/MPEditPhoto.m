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
#import "MPHttpInterceptor.h"
#import "MPGalleryController.h"
#import "MPCapturePhoto.h"
#import "CoreData+MagicalRecord.h"
#import "MPEditTitleController.h"

@interface MPEditPhoto ()

@property (nonatomic, readwrite) UIView* transitionFromView;

@end

@implementation MPEditPhoto
{
    BOOL isWebViewLoaded;
    MPZoomTransitionDelegate* transitionDelegate;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleOrientationChange)
                                                 name:UIDeviceOrientationDidChangeNotification
                                               object:nil];
    
    [MPHttpInterceptor setDelegate:self];
    
    NSURL *htmlUrl = [[NSBundle mainBundle] URLForResource:@"measured_photo_svg" withExtension:@"html"]; // url of the html file bundled with the app
    
    isWebViewLoaded = NO;
    
    self.titleText.delegate = self;
    self.titleText.widthConstraint = self.titleTextWidthConstraint;
    self.transitionFromView = self.titleText;
    
    // setup web view
    self.webView.scalesPageToFit = NO;
    self.webView.delegate = self;
    [self.webView loadRequest:[NSURLRequest requestWithURL:htmlUrl]];
}

- (void)viewWillAppear:(BOOL)animated
{
    self.titleText.measuredPhoto = self.measuredPhoto;
    if (isWebViewLoaded) [self loadMeasuredPhoto];
}

- (void)viewWillDisappear:(BOOL)animated
{
    
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
        [self setOrientation:newOrientation animated:YES];
        
        NSString* jsFunction = [NSString stringWithFormat:@"forceOrientationChange(%li)", (long)newOrientation];
        [self.webView stringByEvaluatingJavaScriptFromString: jsFunction];
    }
    
    if (newOrientation == UIDeviceOrientationPortrait || newOrientation == UIDeviceOrientationPortraitUpsideDown)
    {
        [UIView animateWithDuration: .5
                              delay: 0
                            options: UIViewAnimationOptionCurveEaseIn
                         animations:^{
                             self.titleButton.alpha = 0;
                         }
                         completion:^(BOOL finished){
                             self.titleButton.hidden = YES;
                         }];
    }
    else if (newOrientation == UIDeviceOrientationLandscapeLeft || newOrientation == UIDeviceOrientationLandscapeRight)
    {
        if (newOrientation == UIDeviceOrientationLandscapeLeft)
            self.titleButton.transform = CGAffineTransformMakeRotation(M_PI_2);
        else
            self.titleButton.transform = CGAffineTransformMakeRotation(-M_PI_2);
        
        [self.titleText resignFirstResponder];
        self.titleButton.hidden = NO;
        [UIView animateWithDuration: .5
                              delay: 0
                            options: UIViewAnimationOptionCurveEaseIn
                         animations:^{
                             self.titleButton.alpha = 1.;
                         }
                         completion:^(BOOL finished){
                             
                         }];
    }
}

- (void) setOrientation:(UIDeviceOrientation)orientation animated:(BOOL)animated
{
    NSValue *value = [NSValue value: &orientation withObjCType: @encode(enum UIDeviceOrientation)];
    [[NSNotificationCenter defaultCenter] postNotificationName:MPUIOrientationDidChangeNotification object:value];
}

- (IBAction)handlePhotosButton:(id)sender
{
    [self gotoGallery];
}

- (IBAction)handleCameraButton:(id)sender
{
    if ([self.presentingViewController isKindOfClass:[MPGalleryController class]])
    {
        MPCapturePhoto* cameraController = [self.storyboard instantiateViewControllerWithIdentifier:@"Camera"];
        [cameraController setOrientation:[[UIDevice currentDevice] orientation] animated:NO];
        [self presentViewController:cameraController animated:YES completion:nil];
    }
    else if ([self.presentingViewController isKindOfClass:[MPCapturePhoto class]])
    {
        [self.presentingViewController dismissViewControllerAnimated:YES completion:nil];
    }
}

- (IBAction)handleShareButton:(id)sender
{

}

- (IBAction)handleDelete:(id)sender
{
    [self.measuredPhoto MR_deleteEntity];
    [self gotoGallery];
}

- (IBAction)handleTitleButton:(id)sender
{
    [self gotoEditTitle];
}

- (void) gotoEditTitle
{
    transitionDelegate = [MPZoomTransitionDelegate new];
    MPEditTitleController* titleController = [self.storyboard instantiateViewControllerWithIdentifier:@"EditTitle"];
    titleController.measuredPhoto = self.measuredPhoto;
//    titleController.transitioningDelegate = transitionDelegate;
    [self presentViewController:titleController animated:NO completion:nil];
}

-(void) gotoGallery
{
    if ([self.presentingViewController isKindOfClass:[MPGalleryController class]])
    {
        [self.presentingViewController dismissViewControllerAnimated:YES completion:nil];
    }
    else if ([self.presentingViewController isKindOfClass:[MPCapturePhoto class]])
    {
        [self.presentingViewController.presentingViewController dismissViewControllerAnimated:YES completion:nil];
    }
}

- (void) loadMeasuredPhoto
{
    if (self.measuredPhoto)
    {
        NSString* javascript = [NSString stringWithFormat:@"main('%@', '%@');", self.measuredPhoto.imageFileName, self.measuredPhoto.depthFileName];
        NSString* result = [self.webView stringByEvaluatingJavaScriptFromString: javascript];
        DLogs(result);
    }
    else
    {
        DLog(@"ERROR: Failed to load web view because measuredPhoto is nil");
    }
}

#pragma mark - UITextFieldDelegate

- (BOOL) textFieldShouldBeginEditing:(UITextField *)textField
{
    [self gotoEditTitle];
    return NO;
}

- (BOOL) textFieldShouldReturn:(UITextField *)textField
{
    [textField resignFirstResponder];
    return YES;
}

#pragma mark - UIWebViewDelegate

- (void)webViewDidFinishLoad:(UIWebView *)webView
{
    isWebViewLoaded = YES;
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

#pragma mark - MPHttpInterceptorDelegate

- (NSDictionary *)handleAction:(MPNativeAction *)nativeAction error:(NSError **)error
{
    if ([nativeAction.action isEqualToString:@"annotations"] && [nativeAction.method isEqualToString:@"POST"])
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
    else
    {
        // for testing
        NSString* message = [nativeAction.params objectForKey:@"message"];
        message = message ? message : @"<null>";
        return @{ @"message": message };
    }
    
    return nil;
}

@end
