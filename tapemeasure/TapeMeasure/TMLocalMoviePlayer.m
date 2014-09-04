//
//  TMLocalMoviePlayer.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 8/21/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "TMLocalMoviePlayer.h"

@interface TMLocalMoviePlayer ()

@end

@implementation TMLocalMoviePlayer

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    NSURL *movieURL = [[NSBundle mainBundle] URLForResource:@"EndlessTapeMeasure" withExtension:@"mp4"];
    
    _moviePlayer =  [[MPMoviePlayerController alloc] initWithContentURL:movieURL];
    self.moviePlayer.controlStyle = MPMovieControlStyleNone;
    self.moviePlayer.shouldAutoplay = NO;
    [self.view addSubview:self.moviePlayer.view];
    [self.view bringSubviewToFront:self.moviePlayer.view];
    
    // eliminates flash when we start playing the video
    [self.moviePlayer setFullscreen:YES animated:NO];
    [self.moviePlayer setFullscreen:NO animated:NO];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handlePause)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
}

- (BOOL) prefersStatusBarHidden { return YES; }

//- (UIInterfaceOrientation) preferredInterfaceOrientationForPresentation
//{
//    return UIInterfaceOrientationLandscapeRight;
//}

- (void) handlePause
{
    [self stopMovie];
}

- (void) playMovie
{
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(moviePlayBackDidFinish:)
                                                 name:MPMoviePlayerPlaybackDidFinishNotification
                                               object:_moviePlayer];
    
    
    [self.moviePlayer setFullscreen:YES animated:NO];
    [self.moviePlayer setCurrentPlaybackTime:0];
    [self.moviePlayer play];
}

- (void) stopMovie
{
    [self.moviePlayer setFullscreen:NO animated:NO];
    
    self.playButton.hidden = YES;
    self.skipButton.hidden = YES;
    self.messageLabel.hidden = YES;
    
    self.playAgainButton.hidden = NO;
    self.continueButton.hidden = NO;
}

- (void) moviePlayBackDidFinish:(NSNotification*)notification
{
    MPMoviePlayerController *player = [notification object];
    
    [[NSNotificationCenter defaultCenter]
         removeObserver:self
         name:MPMoviePlayerPlaybackDidFinishNotification
         object:player];
    
    if ([player respondsToSelector:@selector(setFullscreen:animated:)])
    {
        [self stopMovie];
    }
}

- (IBAction)handlePlayButton:(id)sender
{
    [self playMovie];
}

- (IBAction)handleSkipButton:(id)sender
{
    [self dismissSelf];
}

- (IBAction)handlePlayAgainButton:(id)sender
{
    [self playMovie];
}

- (IBAction)handleContinueButton:(id)sender
{
    [self dismissSelf];
}

- (void) dismissSelf
{
    if ([self.delegate respondsToSelector:@selector(moviePlayerDismissed)])
    {
        [self.delegate moviePlayerDismissed];
    }
    else if (self.presentingViewController)
    {
        [self.presentingViewController dismissViewControllerAnimated:YES completion:nil];
    }
}

@end
