//
//  MPYouTubeVideo.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/13/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPYouTubeVideo.h"
#import "MPCapturePhoto.h"

@implementation MPYouTubeVideo
{
    NSString* html;
}
@synthesize webView, videoUrl, navBar;

- (void) viewDidLoad
{
    [super viewDidLoad];
    if (SYSTEM_VERSION_LESS_THAN(@"7.0")) [navBar setBarStyle:UIBarStyleBlack];
    videoUrl = @"http://www.youtube.com/embed/mhlkZO2yybM";
}

- (void) viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
}

- (void) viewDidLayoutSubviews
{
    if (html == nil)
    {
        html =[NSString stringWithFormat:@"\
                              <html><head>\
                              <style type=\"text/css\">\
                              body {\
                              background-color: black;\
                              color: white;\
                              margin:0;\
                              }\
                              </style>\
                              </head><body>\
                              <iframe width=\"%0.0f\" height=\"%0.0f\" src=\"%@\" frameborder='0' allowfullscreen></iframe>\
                              </body></html>", webView.bounds.size.width, webView.bounds.size.height, videoUrl];
        [webView loadHTMLString:html baseURL:nil];
    }
}

- (NSUInteger)supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskLandscape;
}

- (IBAction)handleDoneButton:(id)sender
{
    MPCapturePhoto* vc = [self.storyboard instantiateViewControllerWithIdentifier:@"MeasuredPhoto"];
    self.view.window.rootViewController = vc;
}
@end
