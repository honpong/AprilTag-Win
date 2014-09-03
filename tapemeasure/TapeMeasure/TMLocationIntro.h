//
//  TMLocationIntro.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 8/28/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@protocol TMLocationIntroDelegate <NSObject>

- (void) nextButtonTapped;

@end

@interface TMLocationIntro : UIViewController

@property (weak, nonatomic) id<TMLocationIntroDelegate> delegate;
@property (weak, nonatomic) IBOutlet UIButton *nextButton;
@property (weak, nonatomic) IBOutlet UILabel *introLabel;

- (IBAction)handleNextButton:(id)sender;

@end
