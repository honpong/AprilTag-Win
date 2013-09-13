//
//  MPYouTubeVideo.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/13/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPViewController.h"

@interface MPYouTubeVideo : MPViewController

@property (weak, nonatomic) IBOutlet UIWebView *webView;
@property (nonatomic) NSString* videoUrl;

@end
