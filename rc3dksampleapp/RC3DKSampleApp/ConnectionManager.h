//
//  ConnectionManager.h
//  RC3DKSampleApp
//
//  Created by Brian on 9/9/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface ConnectionManager : NSObject <NSNetServiceBrowserDelegate, NSNetServiceDelegate>

- (void) startConnection;
- (void) endConnection;

- (void) connect;
- (void) disconnect;
- (BOOL) isConnected;
- (void)sendPacket:(NSDictionary *)packet;

+ (ConnectionManager *) sharedInstance;

@end
