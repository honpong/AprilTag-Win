//
//  TMMeasurementsList.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/1/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreData/CoreData.h>

@interface TMMeasurementsList : UITableViewController <NSFetchedResultsControllerDelegate>

- (IBAction)handleDeleteButton:(id)sender;

@property (strong, nonatomic) NSFetchedResultsController *fetchedResultsController;
@property (strong, nonatomic) NSManagedObjectContext *managedObjectContext;

@property (weak, nonatomic) IBOutlet UILabel *measurementName;
@property (weak, nonatomic) IBOutlet UILabel *measurementValue;

@property (nonatomic, strong) NSArray *measurementsData;
@end
