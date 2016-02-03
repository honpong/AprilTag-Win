//
//  MPYouTubeVideo.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/13/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPBaseViewController.h"

@interface MPYouTubeVideo : MPBaseViewController

@property (weak, nonatomic) IBOutlet UIBarButtonItem *doneButton;
@property (weak, nonatomic) IBOutlet UIWebView *webView;
@property (nonatomic) NSString* videoUrl;
@property (weak, nonatomic) IBOutlet UINavigationBar *navBar;

- (IBAction)handleDoneButton:(id)sender;

@end
