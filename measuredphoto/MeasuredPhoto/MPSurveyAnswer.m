//
//  MPSurveyAnswer.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 5/6/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPSurveyAnswer.h"
#import "MPAnalytics.h"
#import <RCCore/RCCore.h>

@implementation MPSurveyAnswer

+ (void) postAnswer:(BOOL)isAccurate
{
    LOGME;
    
    [MPAnalytics logEvent:@"Choice.AccuracySurvey" withParameters:@{ @"Accurate" : isAccurate ? @"Yes" : @"No" }];
    
    NSString* vendorId = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
    NSString* answer = isAccurate ? @"true" : @"false";
    NSString* jsonString = [NSString stringWithFormat:@"{ \"id\":\"%@\", \"is_accurate\": \"%@\" }", vendorId, answer];
    NSDictionary* postParams = @{ @"secret": @"BensTheDude", JSON_KEY_FLAG:@(JsonBlobFlagAccuracyQuestion), JSON_KEY_BLOB: jsonString };
    
    [HTTP_CLIENT
     postPath:API_DATUM_LOGGED
     parameters:postParams
     success:^(AFHTTPRequestOperation *operation, id JSON)
     {
         DLog(@"POST Response\n%@", operation.responseString);
     }
     failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         if (operation.response.statusCode)
         {
             DLog(@"Failed to POST. Status: %li %@", (long)operation.response.statusCode, operation.responseString);
             NSString *requestBody = [[NSString alloc] initWithData:operation.request.HTTPBody encoding:NSUTF8StringEncoding];
             DLog(@"Failed request body:\n%@", requestBody);
         }
         else
         {
             DLog(@"Failed to POST.\n%@", error);
         }
     }
     ];
}

@end
