//
//  ReplayViewController.m
//  RCCapture
//
//  Created by Brian on 2/18/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

#import "ReplayViewController.h"
#import <RCCore/RCCore.h>

@interface ReplayViewController ()
{
    RCReplayManager * controller;
    BOOL isStarted;
    NSString * replayFilename;
    NSString * calibrationFilename;
}
@end

@implementation ReplayViewController

@synthesize startButton, progressBar, progressText;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void) startReplay
{
    NSLog(@"Start replay");
    BOOL isRealtime = [_realtimeSwitch isOn];
    [controller setupWithPath:replayFilename withCalibration:calibrationFilename withRealtime:isRealtime];
    [controller startReplay];
    [startButton setTitle:@"Stop" forState:UIControlStateNormal];
    isStarted = TRUE;
}

- (void) stopReplay
{
    NSLog(@"Stop replay");
    [controller stopReplay];
    [startButton setTitle:@"Start" forState:UIControlStateNormal];
    isStarted = FALSE;
}

- (IBAction) startStopClicked:(id)sender
{
    if(isStarted)
        [self stopReplay];
    else
        [self startReplay];
}

- (void)setProgressPercentage:(float)progress
{
    float percent = progress*100.;
    [progressBar setProgress:progress];
    [progressText setText:[NSString stringWithFormat:@"%.0f%%", percent]];
}

- (void) replayProgress:(float)progress
{
    [self setProgressPercentage:progress];
}

- (void) replayFinished
{
    NSLog(@"Replay finished");
    [self stopReplay];
}

- (NSString *)getFirstReplayFilename
{
    NSArray * paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString * documentsDirectory = [paths objectAtIndex:0];
    NSArray * documents = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:documentsDirectory error:NULL];
    NSMutableArray * files = [[NSMutableArray alloc] initWithCapacity:5];
    for(NSString * doc in documents) {
        if([doc rangeOfString:@"capture"].location != NSNotFound) {
            [files addObject:doc];
        }
    }
    if(files.count > 0)
        return [documentsDirectory stringByAppendingPathComponent:files[0]];
    return @"";
}

- (NSString *)getFirstCalibrationFilename
{
    NSArray * paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString * documentsDirectory = [paths objectAtIndex:0];
    NSArray * documents = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:documentsDirectory error:NULL];
    NSMutableArray * files = [[NSMutableArray alloc] initWithCapacity:5];
    for(NSString * doc in documents) {
        if([doc rangeOfString:@"calibration"].location != NSNotFound) {
            [files addObject:doc];
        }
    }
    if(files.count > 0)
        return [documentsDirectory stringByAppendingPathComponent:files[0]];
    return @"";
}


- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view.
    [self setProgressPercentage:0];
    controller = [[RCReplayManager alloc] init];
    controller.delegate = self;
    replayFilename = [self getFirstReplayFilename];
    calibrationFilename = [self getFirstCalibrationFilename];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
