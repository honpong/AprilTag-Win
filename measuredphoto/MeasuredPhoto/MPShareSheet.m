//
//  MPShareSheet.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPShareSheet.h"

@interface MPShareSheet ()

@end

@implementation MPShareSheet

+ (MPShareSheet*) shareSheetWithDelegate:(UIViewController<MPShareSheetDelegate>*) delegate
{
    MPShareSheet* shareSheet = [MPShareSheet new];
    shareSheet.delegate = delegate;
    return shareSheet;
}

- (void) showShareSheet_Pad_FromBarButtonItem:(UIBarButtonItem *)barButtonItem content:(OSKShareableContent *)content
{
    
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
    {
        NSDictionary *options = [self createOptionsDict];
        [[OSKPresentationManager sharedInstance] presentActivitySheetForContent:content
                                                       presentingViewController:self.delegate
                                                       popoverFromBarButtonItem:barButtonItem
                                                       permittedArrowDirections:UIPopoverArrowDirectionAny
                                                                       animated:YES
                                                                        options:options];
    }
    else
    {
        [self showShareSheet_Phone:content];
    }
}

- (void) showShareSheet_Pad_FromRect:(CGRect)rect withViewController:(UIViewController*)viewController inView:(UIView*)inView content:(OSKShareableContent *)content
{
    
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
    {
        NSDictionary *options = [self createOptionsDict];
        [[OSKPresentationManager sharedInstance] presentActivitySheetForContent:content
                                                       presentingViewController:viewController
                                                                popoverFromRect:rect
                                                                         inView:inView
                                                       permittedArrowDirections:UIPopoverArrowDirectionUp
                                                                       animated:YES
                                                                        options:options];
    }
    else
    {
        [self showShareSheet_Phone:content];
    }
}

- (void) showShareSheet_Phone:(OSKShareableContent *)content
{
    NSDictionary *options = [self createOptionsDict];
    
    // 4) Present the activity sheet via the presentation manager.
    [[OSKPresentationManager sharedInstance] presentActivitySheetForContent:content
                                                   presentingViewController:self.delegate
                                                                    options:options];
}

- (NSDictionary*) createOptionsDict
{
    // 2) Setup optional completion and dismissal handlers
    OSKActivityCompletionHandler completionHandler = nil;
    OSKPresentationEndingHandler dismissalHandler = nil;
    
    if ([self.delegate respondsToSelector:@selector(activityCompletionHandler)]) completionHandler = [self.delegate activityCompletionHandler];
    if ([self.delegate respondsToSelector:@selector(dismissalHandler)]) dismissalHandler = [self.delegate dismissalHandler];
    
    // 3) Create the options dictionary. See OSKActivity.h for more options.
    NSDictionary *options = nil;
    if (completionHandler && dismissalHandler)
    {
        options = @{ OSKPresentationOption_ActivityCompletionHandler : completionHandler, OSKPresentationOption_PresentationEndingHandler : dismissalHandler };
    }
    else if (completionHandler)
    {
        options = @{ OSKPresentationOption_ActivityCompletionHandler : completionHandler };
    }
    else if (dismissalHandler)
    {
        options = @{ OSKPresentationOption_PresentationEndingHandler : dismissalHandler };
    }
    
    return options;
}

- (OSKApplicationCredential *)applicationCredentialForActivityType:(NSString *)activityType
{
    OSKApplicationCredential *appCredential = nil;
    
    if ([activityType isEqualToString:OSKActivityType_iOS_Facebook]) {
        appCredential = [[OSKApplicationCredential alloc]
                         initWithOvershareApplicationKey:RCApplicationCredential_Facebook_Key
                         applicationSecret:RCApplicationCredential_Facebook_Secret
                         appName:@"3Dimension"];
    }
    
    return appCredential;
}

@end
