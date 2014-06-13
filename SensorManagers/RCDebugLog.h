//
//  RCDebugLog.h
//
//  Created by Ben Hirashima on 2/5/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#ifndef RCDebugLog_h
#define RCDebugLog_h

#ifdef DEBUG
#define LOGME NSLog(@"%s", __PRETTY_FUNCTION__);
#define DLog(fmt, ...) NSLog((@"%s " fmt), __PRETTY_FUNCTION__, ##__VA_ARGS__);
#else
#define LOGME // do nothing
#define DLog(fmt, ...) // do nothing
#endif

#endif
