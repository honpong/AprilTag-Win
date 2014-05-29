//
//  TMResultsVC.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>
#import "TMOptionsVC.h"
#import "TMMeasurement.h"
#import "TMLocation.h"
#import "TMMapVC.h"
#import "TMMeasurement+TMMeasurementExt.h"
#import "TMMeasurement+TMMeasurementSync.h"
#import "TMSyncable+TMSyncableSync.h"
#import "TMNewMeasurementVC.h"
#import "TMTableViewController.h"
#import "RCCore/RCDistanceLabel.h"
#import "OvershareKit.h"
#import "TMShareSheet.h"

@interface TMResultsVC : TMTableViewController <NSFetchedResultsControllerDelegate, UITextFieldDelegate, UIActionSheetDelegate, OptionsDelegate, OSKActivityCustomizations, TMShareSheetDelegate>
{
    UIActionSheet *sheet;
    NSURLConnection *theConnection;
}

- (IBAction)handleDeleteButton:(id)sender;
- (IBAction)handleDoneButton:(id)sender;
- (IBAction)handleKeyboardDone:(id)sender;
- (IBAction)handleActionButton:(id)sender;
- (IBAction)handlePageCurl:(id)sender;
- (void)saveMeasurement;

@property (nonatomic) IBOutlet UIBarButtonItem *btnDone;
@property (nonatomic) IBOutlet UIBarButtonItem *btnAction;

@property TMMeasurement *theMeasurement;
@property (weak) UIViewController *prevView;

@end
