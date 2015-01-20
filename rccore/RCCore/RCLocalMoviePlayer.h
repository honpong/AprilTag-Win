//
//  RCLocalMoviePlayer.h
//  RCCore
//
//  Created by Ben Hirashima on 9/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MediaPlayer/MediaPlayer.h>

@protocol RCLocalMoviePlayerDelegate <NSObject>

@optional
- (void) moviePlayerDismissed;
- (void) moviePlayBackDidFinish;
- (void) moviePlayerPlayButtonTapped;
- (void) moviePlayerSkipButtonTapped;
- (void) moviePlayerContinueButtonTapped;
- (void) moviePlayerAgainButtonTapped;

@end

@interface RCLocalMoviePlayer : UIViewController

@property (weak, nonatomic) id<RCLocalMoviePlayerDelegate> delegate;
@property (nonatomic) MPMoviePlayerController* moviePlayer;
@property (weak, nonatomic) IBOutlet UIButton *playButton;
@property (strong, nonatomic) IBOutlet UIView *skipButton;
@property (weak, nonatomic) IBOutlet UIButton *playAgainButton;
@property (weak, nonatomic) IBOutlet UIButton *continueButton;
@property (weak, nonatomic) IBOutlet UILabel *messageLabel;
@property (nonatomic) NSURL *movieURL;

- (IBAction)handlePlayButton:(id)sender;
- (IBAction)handleSkipButton:(id)sender;
- (IBAction)handlePlayAgainButton:(id)sender;
- (IBAction)handleContinueButton:(id)sender;

@end
