//
//  TMResultsVC.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@class TMMeasurement;

@interface TMResultsVC : UITableViewController <NSFetchedResultsControllerDelegate>
- (IBAction)handleDoneButton:(id)sender;
- (IBAction)handleDeleteButton:(id)sender;
- (IBAction)handleUpgradeButton:(id)sender;

@property (weak, nonatomic) IBOutlet UIButton *upgradeBtn;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *btnDone;

@property (strong, nonatomic) NSFetchedResultsController *fetchedResultsController;
@property (strong, nonatomic) NSManagedObjectContext *managedObjectContext;

@property (nonatomic, strong) TMMeasurement *theMeasurement;

@property (weak, nonatomic) IBOutlet UILabel *theName;
@property (weak, nonatomic) IBOutlet UILabel *pointToPoint;
@property (weak, nonatomic) IBOutlet UILabel *totalPath;
@property (weak, nonatomic) IBOutlet UILabel *horzDist;
@property (weak, nonatomic) IBOutlet UILabel *vertDist;
@property (weak, nonatomic) IBOutlet UILabel *theDate;

@end
