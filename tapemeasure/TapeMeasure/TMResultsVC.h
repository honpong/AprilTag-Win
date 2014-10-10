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
#import "OvershareKit.h"
#import "TMShareSheet.h"

@interface TMResultsVC : TMTableViewController <NSFetchedResultsControllerDelegate,
                                                UITextFieldDelegate,
                                                UIActionSheetDelegate,
                                                OptionsDelegate,
                                                TMShareSheetDelegate,
                                                RCRateMeViewDelegate,
                                                RCLocationPopUpDelegate>
{
    UIActionSheet *sheet;
    NSURLConnection *theConnection;
}

- (IBAction)handleDeleteButton:(id)sender;
- (IBAction)handleNewButton:(id)sender;
- (IBAction)handleKeyboardDone:(id)sender;
- (IBAction)handleActionButton:(id)sender;
- (IBAction)handleUnitsButton:(id)sender;
- (IBAction)handleListButton:(id)sender;
- (void)saveMeasurement;

@property (weak, nonatomic) IBOutlet UIBarButtonItem *btnNew;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *btnAction;
@property (weak, nonatomic) IBOutlet RCDistanceLabel *distLabel;

@property TMMeasurement *theMeasurement;

@end
