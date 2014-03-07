//
//  ConnectionManager.h
//  RC3DKSampleVis
//
//  Created by Brian on 2/27/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

#import <Foundation/Foundation.h>

@class GCDAsyncSocket;

@protocol RCVisDataDelegate <NSObject>

- (void) observeTime:(float)time;
- (void) observeFeatureWithId:(uint64_t)id x:(float)x y:(float)y z:(float)z lastSeen:(float)lastSeen good:(bool)good;
- (void) observePathWithTranslationX:(float)x y:(float)y z:(float)z time:(float)time;
- (void) reset;

@end

@protocol RCConnectionManagerDelegate <NSObject>

- (void) connectionManagerDidConnect;
- (void) connectionManagerDidDisconnect;

@end

@interface ConnectionManager : NSObject<NSNetServiceDelegate>
{
    GCDAsyncSocket *listenSocket;
    GCDAsyncSocket *connectedSocket;
    NSNetService *netService;
}

@property (weak, nonatomic) id<RCVisDataDelegate> visDataDelegate;
@property (weak, nonatomic) id<RCConnectionManagerDelegate> connectionManagerDelegate;

- (void)startListening;

+ (ConnectionManager *) sharedInstance;

@end
