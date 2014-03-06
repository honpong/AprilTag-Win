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
#import "WorldState.h"

#define INITIAL_LIMITS 3.
#define POINT_SIZE 3.0

@implementation RCOpenGLView
{
    float xMin, xMax, yMin, yMax;
    float currentTime;
    RCViewpoint currentViewpoint;
    RCFeatureFilter featuresFilter;
    NSTimer * renderTimer;
    int renderStep;
    float currentScale;
    WorldState * state;
}

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
    if(renderTimer)
        [self removeRenderTimer];

    currentViewpoint = viewpoint;
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
    glPointSize( POINT_SIZE );
    featuresFilter = RCFeatureFilterShowGood;
    [self setViewpoint:RCViewpointTopDown];
}

- (void) reset
{
    NSLog(@"SampleVis reset");
    [state reset];
    xMin = -INITIAL_LIMITS;
    xMax = INITIAL_LIMITS;
    yMin = -INITIAL_LIMITS;
    yMax = INITIAL_LIMITS;
    currentScale = 1.;
    currentTime = 0;
    if(currentViewpoint == RCViewpointAnimating)
        [self setViewpoint:RCViewpointTopDown];
}

- (void) awakeFromNib
{
    state = [[WorldState alloc] init];
    [ConnectionManager sharedInstance].visDataDelegate = state;
    [ConnectionManager sharedInstance].connectionManagerDelegate = self;
    [self reset];
}

-(void) drawPath
{
    NSArray * path = [state getPath];
    for(id location in path)
    {
        glPushMatrix();
        Translation t;
        NSValue * value = location;
        [value getValue:&t];
        glTranslatef(t.x, t.y, t.z);
        glBegin(GL_POINTS);
        {
            if (t.time == currentTime)
                glColor4f(0,1,0,1);
            else
                glColor4f(0, .698, .807, 1);
            glVertex3f(0,0,0);
        }
        glEnd();
        glPopMatrix();
    }
}

- (void)drawFeatures {
    glBegin(GL_POINTS);
    {
        NSDictionary * features = [state getFeatures];
        for(id key in features)
        {
            Feature f;
            NSValue * value = [features objectForKey:key];
            [value getValue:&f];
            if (f.lastSeen == currentTime)
                glColor4f(.968, .345, .384, 1);
            else
            {
                if (featuresFilter == RCFeatureFilterShowGood && !f.good)
                    continue;
                glColor4f(1,1,1,1);
            }
            glVertex3f(f.x, f.y, f.z);
        }
    }
    glEnd();
}

- (void)drawGrid {
    float scale = 1; /* meter */
    glBegin(GL_LINES);
    glColor3f(.478, .494, .572); // grid color
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
    glColor3f(.968, .345, .384);
    glVertex3f(0, 0, 0);
    glVertex3f(.5, 0, 0);
    glColor3f(0, .788, .349);
    glVertex3f(0, 0, 0);
    glVertex3f(0, .5, 0);
    glColor3f(.982, .552, .317);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0, .5);
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
        glScalef(currentScale, currentScale, currentScale);
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
    glClearColor(.274, .286, .349, 1.); // background color
    glClear(GL_COLOR_BUFFER_BIT);
    currentTime = [state getTime];
    [self transformWorld];
    [self drawGrid];
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

- (BOOL)acceptsFirstResponder // necessary to intercept key events
{
    return YES;
}

- (void) keyUp:(NSEvent *)theEvent
{
    switch (theEvent.keyCode)
    {
        case 49: // space key
            [self reset];
            break;
    
        case 24: // = key
            [self scaleUp];
            break;
            
        case 27: // - key
            [self scaleDown];
            break;
            
        default:
            break;
    }
}

- (void) scaleUp
{
    currentScale = currentScale + .5;
}

- (void) scaleDown
{
    if (currentScale > .5) currentScale = currentScale - .5;
    else if (currentScale > .2) currentScale = currentScale - .1;
}

- (void) connectionManagerDidConnect
{
    NSLog(@"connectionManagerDidConnect");
    [self setViewpoint:RCViewpointTopDown];
}

- (void) connectionManagerDidDisconnect
{
    NSLog(@"connectionManagerDidDisconnect");
    [self setViewpoint:RCViewpointAnimating];
}

@end