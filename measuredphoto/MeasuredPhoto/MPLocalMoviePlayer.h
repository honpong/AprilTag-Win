//
//  MPLocalMoviePlayer.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 5/5/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <RCCore/RCCore.h>

@interface MPLocalMoviePlayer : RCLocalMoviePlayer

@property (weak, nonatomic) id<RCLocalMoviePlayerDelegate> delegate;
@property (nonatomic) MPMoviePlayerController* moviePlayer;
@property (weak, nonatomic) IBOutlet UIButton *playButton;
@property (strong, nonatomic) IBOutlet UIView *skipButton;
@property (weak, nonatomic) IBOutlet UIButton *playAgainButton;
@property (weak, nonatomic) IBOutlet UIButton *continueButton;
@property (weak, nonatomic) IBOutlet UILabel *messageLabel;

- (IBAction)handlePlayButton:(id)sender;
- (IBAction)handleSkipButton:(id)sender;
- (IBAction)handlePlayAgainButton:(id)sender;
- (IBAction)handleContinueButton:(id)sender;

@end
