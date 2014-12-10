//
//  RCLocalMoviePlayer.m
//  RCCore
//
//  Created by Ben Hirashima on 9/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCLocalMoviePlayer.h"

@interface RCLocalMoviePlayer ()

@end

@implementation RCLocalMoviePlayer

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    _moviePlayer =  [[MPMoviePlayerController alloc] initWithContentURL:self.movieURL];
    self.moviePlayer.controlStyle = MPMovieControlStyleEmbedded;
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

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (BOOL) prefersStatusBarHidden { return YES; }

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
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(moviePlayerDidExitFullscreen)
                                                 name:MPMoviePlayerDidExitFullscreenNotification
                                               object:_moviePlayer];
    
    
    [self.moviePlayer setFullscreen:YES animated:NO];
    [self.moviePlayer setCurrentPlaybackTime:0];
    [self.moviePlayer play];
}

- (void) stopMovie
{
    [self.moviePlayer stop];
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
    
    [self stopMovie];
    
    if ([self.delegate respondsToSelector:@selector(moviePlayBackDidFinish)])
    {
        [self.delegate moviePlayBackDidFinish];
    }
}

- (void) moviePlayerDidExitFullscreen
{
    [[NSNotificationCenter defaultCenter]
     removeObserver:self
     name:MPMoviePlayerDidExitFullscreenNotification
     object:self.moviePlayer];
    
    [self stopMovie];
    
    if ([self.delegate respondsToSelector:@selector(moviePlayBackDidFinish)])
    {
        [self.delegate moviePlayBackDidFinish];
    }
}

- (IBAction)handlePlayButton:(id)sender
{
    [self playMovie];
    
    if ([self.delegate respondsToSelector:@selector(moviePlayerPlayButtonTapped)])
    {
        [self.delegate moviePlayerPlayButtonTapped];
    }
}

- (IBAction)handleSkipButton:(id)sender
{
    [self dismissSelf];
    
    if ([self.delegate respondsToSelector:@selector(moviePlayerSkipButtonTapped)])
    {
        [self.delegate moviePlayerSkipButtonTapped];
    }
}

- (IBAction)handlePlayAgainButton:(id)sender
{
    [self playMovie];
    
    if ([self.delegate respondsToSelector:@selector(moviePlayerAgainButtonTapped)])
    {
        [self.delegate moviePlayerAgainButtonTapped];
    }
}

- (IBAction)handleContinueButton:(id)sender
{
    [self dismissSelf];
    
    if ([self.delegate respondsToSelector:@selector(moviePlayerContinueButtonTapped)])
    {
        [self.delegate moviePlayerContinueButtonTapped];
    }
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

- (void) setMovieURL:(NSURL *)movieURL
{
    if (self.moviePlayer) self.moviePlayer.contentURL = movieURL;
    _movieURL = movieURL;
}

@end
