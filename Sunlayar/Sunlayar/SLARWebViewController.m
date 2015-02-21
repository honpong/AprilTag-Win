//
//  SLARWebViewViewController.m
//  Sunlayar
//
//  Created by Ben Hirashima on 2/20/15.
//  Copyright (c) 2015 Sunlayar. All rights reserved.
//

#import "SLARWebViewController.h"

@implementation SLARWebViewController

- (instancetype)init
{
    self = [super init];
    if (!self) return nil;
    
    pageURL = [[NSBundle mainBundle] URLForResource:@"webgl" withExtension:@"html"];
    
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
}

@end
