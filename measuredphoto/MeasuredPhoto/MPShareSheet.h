//
//  MPShareSheet.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//


#import "OvershareKit.h"

@protocol MPShareSheetDelegate <NSObject>

@optional
- (OSKActivityCompletionHandler) activityCompletionHandler;
- (OSKPresentationEndingHandler) dismissalHandler;

@end

@interface MPShareSheet : NSObject <OSKActivityCustomizations>

@property (weak) UIViewController<MPShareSheetDelegate>* delegate;

+ (MPShareSheet*) shareSheetWithDelegate:(UIViewController<MPShareSheetDelegate>*) delegate;
- (void) showShareSheet_Pad_FromBarButtonItem:(UIBarButtonItem *)barButtonItem content:(OSKShareableContent *)content;
- (void) showShareSheet_Pad_FromRect:(CGRect)rect withViewController:(UIViewController*)viewController inView:(UIView*)inView content:(OSKShareableContent *)content;
- (void) showShareSheet_Phone:(OSKShareableContent *)content;

@end
