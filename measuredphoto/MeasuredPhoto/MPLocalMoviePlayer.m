//
//  MPLocalMoviePlayer.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 5/5/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPLocalMoviePlayer.h"

@interface MPLocalMoviePlayer ()

@end

@implementation MPLocalMoviePlayer

- (void) viewDidLoad
{
    self.movieURL = [[NSBundle mainBundle] URLForResource:@"Tutorial" withExtension:@"mp4"];
    [super viewDidLoad];
}

- (void) viewDidAppear:(BOOL)animated
{
//    [TMAnalytics logEvent:@"View.Tutorial"];
    [super viewDidAppear:animated];
}

- (IBAction)handlePlayButton:(id)sender
{
//    [TMAnalytics logEvent:@"View.Tutorial.Play"];
    [super handlePlayButton:sender];
}

- (IBAction)handleSkipButton:(id)sender
{
//    [TMAnalytics logEvent:@"View.Tutorial.Skip"];
    [super handleSkipButton:sender];
}

- (IBAction)handlePlayAgainButton:(id)sender
{
//    [TMAnalytics logEvent:@"View.Tutorial.PlayAgain"];
    [super handlePlayAgainButton:sender];
}

- (IBAction)handleContinueButton:(id)sender
{
    [super handleContinueButton:sender];
}

@end
