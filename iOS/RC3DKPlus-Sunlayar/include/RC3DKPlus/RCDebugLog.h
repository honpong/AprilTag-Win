//
//  RCDebugLog.h
//
//  Created by Ben Hirashima on 2/5/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#include <pthread.h>

#ifndef RCDebugLog_h
    #define RCDebugLog_h

    #ifdef DEBUG
        #define LOGME fprintf(stderr,"%s %-4x %s\n", __TIME__, pthread_mach_thread_np(pthread_self()), __PRETTY_FUNCTION__);
        #define DLog(format, ...) fprintf(stderr,"%s %-4x %s %s\n", __TIME__, pthread_mach_thread_np(pthread_self()), __PRETTY_FUNCTION__, [[NSString stringWithFormat:format, ##__VA_ARGS__] UTF8String]);
    #else
        #ifdef RELEASE
            #define LOGME NSLog(@"%s", __PRETTY_FUNCTION__);
            #define DLog(fmt, ...) NSLog((@"%s " fmt), __PRETTY_FUNCTION__, ##__VA_ARGS__);
        #else
            #define LOGME // do nothing
            #define DLog(fmt, ...) // do nothing
            #define NSLog(...) // do nothing
        #endif
    #endif
#endif