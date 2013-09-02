//
//  RCOpenGLPath.m
//  RC3DKSampleVis
//
//  Created by Brian on 8/30/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import "RCOpenGLPath.h"

#include <OpenGL/gl.h>

@implementation RCOpenGLPath

/* TODO: draw rotation, need to make rodrigues accessible here */

typedef struct _translation {
    float x, y, z;
    float time;
} Translation;

-(void) observeWithTranslationX:(float)x y:(float)y z:(float)z time:(float)time
{
    if(!self.path)
        self.path = [[NSMutableArray alloc] initWithCapacity:10];

    Translation t;
    t.x = x;
    t.y = y;
    t.z = z;
    t.time = time;

    NSValue * value = [NSValue value:&t withObjCType:@encode(Translation)];
    [self.path addObject:value];
}

-(void) drawPath:(float)time maxAge:(float)maxAge
{
    // Sort locations by time
    for(id location in self.path)
    {
        glPushMatrix();
        Translation t;
        NSValue * value = location;
        [value getValue:&t];
        if(time - t.time > maxAge)
            continue;
        glTranslatef(t.x, t.y, t.z);
        glBegin(GL_POINTS);
        {
            if (t.time == time)
                glColor4f(0,0,1,1);
            else
            {
                float alpha = 1 - (time - t.time)/maxAge;
                glColor4f(0,1,0,alpha);
            }
            glVertex3f(0,0,0);
        }
        glEnd();
        glPopMatrix();
    }
}

-(void) reset
{
    NSLog(@"RCOpenGL reset");
    [self.path removeAllObjects];
}

@end
