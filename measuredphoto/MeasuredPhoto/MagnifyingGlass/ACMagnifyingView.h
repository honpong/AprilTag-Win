//
//  ACMagnifyingView.h
//  MagnifyingGlass
//
//  Created by Arnaud Coomans on 30/12/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MPView.h"

@class ACMagnifyingGlass;

@interface ACMagnifyingView : MPView

@property (nonatomic, retain) ACMagnifyingGlass *magnifyingGlass;
@property (nonatomic, assign) CGFloat magnifyingGlassShowDelay;

@end
