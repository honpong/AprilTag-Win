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
#import "RCCore/RCDistanceFormatter.h"
#import "TMLocation.h"
#import "TMMapVC.h"
#import "TMMeasurement+TMMeasurementExt.h"
#import "TMMeasurement+TMMeasurementSync.h"
#import "TMSyncable+TMSyncableSync.h"
#import "TMNewMeasurementVC.h"

@interface TMResultsVC : UITableViewController <NSFetchedResultsControllerDelegate, UITextFieldDelegate, UIActionSheetDelegate, OptionsDelegate>
{
    UIActionSheet *sheet;
    NSURLConnection *theConnection;
}

- (IBAction)handleDeleteButton:(id)sender;
- (IBAction)handleDoneButton:(id)sender;
- (IBAction)handleKeyboardDone:(id)sender;
- (IBAction)handleActionButton:(id)sender;
- (IBAction)handlePageCurl:(id)sender;

@property (weak, nonatomic) IBOutlet UIBarButtonItem *btnDone;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *btnAction;

@property (nonatomic, strong) TMMeasurement *theMeasurement;
@property (weak) UIViewController *prevView;

@end
