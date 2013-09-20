//
//  MPSurvey1.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/20/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPSurvey1.h"

@interface MPSurvey1 ()

@end

@implementation MPSurvey1

+ (UIViewController *) instantiateViewControllerWithDelegate:(id)delegate
{
    // These three lines prevent the compiler from optimizing out the view controller classes
    // completely, since they are only presented in a storyboard which is not directly referenced anywhere.
    [MPSurvey1 class];
    
    UIStoryboard * storyBoard = [UIStoryboard storyboardWithName:@"Survey" bundle:nil];
    MPSurvey1 * vc = (MPSurvey1*)[storyBoard instantiateInitialViewController];
//    vc.delegate = delegate;
    return vc;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view.
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
