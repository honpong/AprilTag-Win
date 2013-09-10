//
//  RCOpenGLView.m
//  RC3DKSampleVis
//
//  Created by Brian on 8/26/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

#import "RCOpenGLView.h"

@implementation RCOpenGLView
{
    NSMutableDictionary * features;
    NSMutableArray * path;
    float xMin, xMax, yMin, yMax;
    float maxAge;
    float currentTime;
    RCViewpoint currentViewpoint;
    RCFeatureFilter featuresFilter;
    NSTimer * renderTimer;
    int renderStep;
}

typedef struct _feature {
    float x, y, z;
    float lastSeen;
    bool good;
} Feature;

typedef struct _translation {
    float x, y, z;
    float time;
} Translation;

- (void)removeRenderTimer
{
    [renderTimer invalidate];
    renderTimer = nil;
}

- (void) renderTimerFired:(id)sender
{
    renderStep += 1;
    // Loop
    if (renderStep > 1000) {
        renderStep = 0;
    }
    [self drawRect:[self bounds]];
}

- (void)addRenderTimer
{
    renderStep = 0;
    renderTimer = [NSTimer timerWithTimeInterval:0.001   //a 1ms time interval
                                          target:self
                                        selector:@selector(renderTimerFired:)
                                        userInfo:nil
                                         repeats:YES];

    [[NSRunLoop currentRunLoop] addTimer:renderTimer
                                 forMode:NSDefaultRunLoopMode];
    [[NSRunLoop currentRunLoop] addTimer:renderTimer
                                 forMode:NSEventTrackingRunLoopMode]; //Ensure timer fires during resize
}

- (void)setViewpoint:(RCViewpoint)viewpoint
{
    if(currentViewpoint == RCViewpointAnimating)
        [self removeRenderTimer];

    currentViewpoint = viewpoint;
    if(viewpoint == RCViewpointAnimating)
        [self addRenderTimer];

    [self drawForTime:currentTime];
}

- (void)setFeatureFilter:(RCFeatureFilter)featureType
{
    featuresFilter = featureType;
    [self drawForTime:currentTime];
}

- (void)prepareOpenGL
{
    glEnable( GL_POINT_SMOOTH );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glPointSize( 6.0 );
    features = [[NSMutableDictionary alloc] initWithCapacity:100];
    path = [[NSMutableArray alloc] initWithCapacity:10];
    currentViewpoint = RCViewpointTopDown;
    featuresFilter = RCFeatureFilterShowGood;
}

- (void) reset
{
    NSLog(@"SampleVis reset");
    [features removeAllObjects];
    [path removeAllObjects];
    xMin = -5;
    xMax = 5;
    yMin = -5;
    yMax = 5;
    currentTime = 0;
    if(currentViewpoint == RCViewpointAnimating)
        [self setViewpoint:RCViewpointTopDown];
}

- (void) awakeFromNib
{
    [self reset];
    maxAge = 30;
}

- (void) observeFeatureWithId:(uint64_t)id x:(float)x y:(float)y z:(float)z lastSeen:(float)lastSeen good:(bool)good
{
    NSNumber * key = [NSNumber numberWithUnsignedLongLong:id];

    Feature f;
    f.x = x;
    f.y = y;
    f.z = z;
    f.lastSeen = lastSeen;
    f.good = good;
    NSValue * value = [NSValue value:&f withObjCType:@encode(Feature)];
    [features setObject:value forKey:key];
}

- (void) observePathWithTranslationX:(float)x y:(float)y z:(float)z time:(float)time
{
    Translation t;
    t.x = x;
    t.y = y;
    t.z = z;
    t.time = time;

    NSValue * value = [NSValue value:&t withObjCType:@encode(Translation)];
    [path addObject:value];
}

-(void) drawPath
{
    for(id location in path)
    {
        glPushMatrix();
        Translation t;
        NSValue * value = location;
        [value getValue:&t];
        if(currentTime - t.time > maxAge)
            continue;
        glTranslatef(t.x, t.y, t.z);
        glBegin(GL_POINTS);
        {
            if (t.time == currentTime)
                glColor4f(0,0,1,1);
            else
            {
                float alpha = 1 - (currentTime - t.time)/maxAge;
                glColor4f(0,1,0,alpha);
            }
            glVertex3f(0,0,0);
        }
        glEnd();
        glPopMatrix();
    }
}

- (void)drawFeatures {
    glBegin(GL_POINTS);
    {
        for(id key in features)
        {
            Feature f;
            NSValue * value = [features objectForKey:key];
            [value getValue:&f];
            if (f.lastSeen > currentTime || currentTime - f.lastSeen > maxAge)
                continue;
            if (f.lastSeen == currentTime)
                glColor4f(1.0,0,0,1.0);
            else
            {
                if (featuresFilter == RCFeatureFilterShowGood && !f.good)
                    continue;
                float alpha = 1 - (currentTime - f.lastSeen)/maxAge;
                glColor4f(1.0,1.0,1.0,alpha);
            }
            glVertex3f(f.x, f.y, f.z);
        }
    }
    glEnd();
}

- (void)drawGrid {
    float scale = 1; /* meter */
    glBegin(GL_LINES);
    glColor3f(.2, .2, .2);
    /* Grid */
    for(float x = -10*scale; x < 11*scale; x += scale)
    {
        glVertex3f(x, -10*scale, 0);
        glVertex3f(x, 10*scale, 0);
        glVertex3f(-10*scale, x, 0);
        glVertex3f(10*scale, x, 0);
        glVertex3f(-0, -10*scale, 0);
        glVertex3f(-0, 10*scale, 0);
        glVertex3f(-10*scale, -0, 0);
        glVertex3f(10*scale, -0, 0);
        
        glVertex3f(0, -.1*scale, x);
        glVertex3f(0, .1*scale, x);
        glVertex3f(-.1*scale, 0, x);
        glVertex3f(.1*scale, 0, x);
        glVertex3f(0, -.1*scale, -x);
        glVertex3f(0, .1*scale, -x);
        glVertex3f(-.1*scale, 0, -x);
        glVertex3f(.1*scale, 0, -x);
    }
    /* Axes */
    glColor3f(1., 0., 0.);
    glVertex3f(0, 0, 0);
    glVertex3f(1, 0, 0);
    glColor3f(0., 1., 0.);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 1, 0);
    glColor3f(0., 0., 1.);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0, 1);
    glEnd();
}

- (void)drawForTime:(float)time
{
    currentTime = time;
    [self drawRect:[self bounds]];
}

- (void)transformAnimated
{
    // Rotate 90 degrees, wait 1 second, rotate 90 degrees, wait 1 second, rotate back, wait 3 seconds

    float time = renderStep / 100.;
    if(time < 2)
    {
        glRotatef(-90. * time / 2, 1, 0, 0);
    }
    else if(time >= 2 && time < 3)
    {
        // do nothing
        glRotatef(-90, 1, 0, 0);
    }
    else if(time >= 3 && time < 5)
    {
        glRotatef(-90, 1, 0, 0);
        glRotatef(-90. * (time-3)/2, 0, 0, 1);
    }
    else if(time >= 5 && time < 6)
    {
        glRotatef(-90, 1, 0, 0);
        glRotatef(-90, 0, 0, 1);
    }
    else if(time >= 6 && time < 8)
    {
        glRotatef(-90. * (8 - time)/2, 1, 0, 0);
        glRotatef(-90. * (8 - time)/2, 0, 0, 1);
    }
    else if(time >= 8)
    {
        // do nothing
    }
    else
        NSLog(@"Animated rendering didn't know what to do for time %f", time);
}

- (void)transformWorld
{
    // No need to transform for top-down
    if (currentViewpoint == RCViewpointTopDown) {
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
    }

    if (currentViewpoint == RCViewpointSide) {
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glRotatef(-90, 0, 1, 0);
        glRotatef(-90, 1, 0, 0);
    }

    if (currentViewpoint == RCViewpointAnimating) {
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        [self transformAnimated];
    }
}

- (void)drawRect:(NSRect)bounds {
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    [self transformWorld];
    [self drawGrid];
    if(features)
        [self drawFeatures];
    [self drawPath];
    glPopMatrix(); // For the view
    glFlush();
}

- (void)reshape
{
    NSSize size = [self frame].size;
    NSOpenGLContext * context = [NSOpenGLContext currentContext];

    [context update];

    glViewport( 0, 0, (GLint) size.width, (GLint) size.height );

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport (0, 0, size.width, size.height);
    float xoffset = 0, yoffset = 0;
    float dx = xMax - xMin;
    float dy = yMax - yMin;
    float dy_scaled = (xMax - xMin) * size.height / size.width;
    float dx_scaled = (yMax - yMin) * size.width / size.height;
    if(dx_scaled > dx)
        xoffset = (dx_scaled - dx)/2.;
    else if (dy_scaled > dy)
        yoffset = (dy_scaled - dy)/2.;

    glOrtho(xMin-xoffset,xMax+xoffset, yMin-yoffset,yMax+yoffset, 10000., -10000.);
}
@end