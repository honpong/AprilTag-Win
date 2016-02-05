//
//  TMShareSheet.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/7/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "OvershareKit.h"

@protocol TMShareSheetDelegate <NSObject>

@optional
- (OSKActivityCompletionHandler) activityCompletionHandler;
- (OSKPresentationEndingHandler) dismissalHandler;

@end

@interface TMShareSheet : NSObject <OSKActivityCustomizations>

@property (weak) UIViewController<TMShareSheetDelegate>* delegate;

+ (TMShareSheet*) shareSheetWithDelegate:(UIViewController<TMShareSheetDelegate>*) delegate;
- (void) showShareSheet_Pad_FromBarButtonItem:(UIBarButtonItem *)barButtonItem content:(OSKShareableContent *)content;
- (void) showShareSheet_Phone:(OSKShareableContent *)content;

@end
