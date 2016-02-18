//
//  CaptureFilename.m
//  RCUtility
//
//  Created by Brian on 4/8/15.
//  Copyright (c) 2015 Realitycap. All rights reserved.
//

#import "CaptureFilename.h"


@implementation CaptureFilename
+ (NSNumber *) numberFromString:(NSString *)string withMatch:(NSTextCheckingResult *)match withIndex:(int)index
{
    NSNumberFormatter *f = [[NSNumberFormatter alloc] init];
    f.numberStyle = NSNumberFormatterDecimalStyle;
    return [f numberFromString:[string substringWithRange:[match rangeAtIndex:index]]];
}

+ (NSDictionary *) parseFilename:(NSString *)filename
{
    NSMutableDictionary * parameters = [NSMutableDictionary dictionaryWithDictionary:@{@"width": @640, @"height": @480, @"framerate": @30}];

    NSRegularExpression * filename_parse = [NSRegularExpression regularExpressionWithPattern:@"(.*)_(\\d+)_(\\d+)_(\\d+)Hz.capture.*" options:0 error:nil];
    NSArray * matches = [filename_parse matchesInString:filename options:0 range:NSMakeRange(0, filename.length)];
    for(NSTextCheckingResult* match in matches) {
        parameters[@"basename"] = [filename substringWithRange:[match rangeAtIndex:1]];
        parameters[@"width"] = [CaptureFilename numberFromString:filename withMatch:match withIndex:2];
        parameters[@"height"] = [CaptureFilename numberFromString:filename withMatch:match withIndex:3];
        parameters[@"framerate"] = [CaptureFilename numberFromString:filename withMatch:match withIndex:4];
    }

    return parameters;
}

+ (NSString *) filenameFromParameters:(NSDictionary *)parameters
{
    NSString * filename = [NSString stringWithFormat:@"%@_%@_%@_%@Hz.capture",
                           parameters[@"basename"],
                           parameters[@"width"],
                           parameters[@"height"],
                           parameters[@"framerate"]
                           ];
    if(parameters[@"length"])
        filename = [NSString stringWithFormat:@"%@_L%@", filename, parameters[@"length"]];
    if(parameters[@"pathlength"])
        filename = [NSString stringWithFormat:@"%@_PL%@", filename, parameters[@"pathlength"]];
    return filename;
}

@end