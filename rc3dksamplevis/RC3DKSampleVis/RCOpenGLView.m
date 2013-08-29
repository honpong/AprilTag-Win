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
#import "RCOpenGLFeature.h"

@implementation RCOpenGLView
{
    NSMutableDictionary * features;
    float xMin, xMax, yMin, yMax;
}

- (void) reset
{
    features = [[NSMutableDictionary alloc] initWithCapacity:100];
    xMin = -10;
    xMax = 10;
    yMin = -10;
    yMax = 10;
}

- (void) awakeFromNib
{
    [self reset];
}

- (void) observeFeatureWithId:(uint64_t)id x:(float)x y:(float)y z:(float)z lastSeen:(float)lastSeen
{
    NSNumber * key = [NSNumber numberWithUnsignedLongLong:id];
    
    RCOpenGLFeature * f = [features objectForKey:key];
    if(!f)
    {
        f = [[RCOpenGLFeature alloc] init];
        [features setObject:f forKey:key];
    }
    [f observeWithX:x y:y z:z time:lastSeen];
}


- (void)drawFeatures {
    glColor3f(1.0,1.0,1.0);
    glBegin(GL_POINTS);
    {
        for(id key in features)
        {
            RCOpenGLFeature * f = [features objectForKey:key];
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
    [self drawRect:[self bounds]];
}

- (void)drawRect:(NSRect)bounds {
    NSLog(@"DrawRect");
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    [self drawGrid];
    if(features)
        [self drawFeatures];
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

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}
@end