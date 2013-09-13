//
//  MPYouTubeVideo.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/13/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPYouTubeVideo.h"

@implementation MPYouTubeVideo
@synthesize webView, videoUrl;

- (void) viewDidLoad
{
    [super viewDidLoad];
    videoUrl = @"http://www.youtube.com/embed/mhlkZO2yybM";
}

- (void) viewDidLayoutSubviews
{
    NSString* html =[NSString stringWithFormat:@"\
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
                          </body></html>", self.view.bounds.size.width, self.view.bounds.size.height, videoUrl];
    [webView loadHTMLString:html baseURL:nil];
}

- (NSUInteger)supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskLandscape;
}

@end
