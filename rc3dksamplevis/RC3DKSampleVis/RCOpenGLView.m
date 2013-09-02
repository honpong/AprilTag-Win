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
#import "RCOpenGLPath.h"

@implementation RCOpenGLView
{
    NSMutableDictionary * features;
    RCOpenGLPath * path;
    float xMin, xMax, yMin, yMax;
    float maxAge;
    float currentTime;
    RCViewpoint currentViewpoint;
    RCFeatureFilter featuresFilter;
}

- (void)setViewpoint:(RCViewpoint)viewpoint
{
    if(viewpoint == RCViewpointDeviceView) return; // RCViewpointDeviceView is not yet implemented
    currentViewpoint = viewpoint;
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
    path = [[RCOpenGLPath alloc] init];
    currentViewpoint = RCViewpointTopDown;
    featuresFilter = RCFeatureFilterShowGood;
}

- (void) reset
{
    NSLog(@"SampleVis reset");
    [features removeAllObjects];
    [path reset];
    xMin = -10;
    xMax = 10;
    yMin = -10;
    yMax = 10;
    currentTime = 0;
}

- (void) awakeFromNib
{
    [self reset];
    maxAge = 30;
}

- (void) observePathWithTranslationX:(float)x y:(float)y z:(float)z time:(float)time
{
    [path observeWithTranslationX:x y:y z:z time:time];
}

- (void) observeFeatureWithId:(uint64_t)id x:(float)x y:(float)y z:(float)z lastSeen:(float)lastSeen good:(bool)good
{
    NSNumber * key = [NSNumber numberWithUnsignedLongLong:id];
    
    RCOpenGLFeature * f = [features objectForKey:key];
    if(!f)
    {
        f = [[RCOpenGLFeature alloc] init];
        [features setObject:f forKey:key];
    }
    [f observeWithX:x y:y z:z time:lastSeen good:good];
}


- (void)drawFeatures {
    glBegin(GL_POINTS);
    {
        for(id key in features)
        {
            RCOpenGLFeature * f = [features objectForKey:key];
            if (featuresFilter == RCFeatureFilterShowGood && !f.good)
                continue;
            if (f.lastSeen > currentTime || currentTime - f.lastSeen > maxAge)
                continue;
            if (f.lastSeen == currentTime)
                glColor4f(1.0,0,0,1.0);
            else
            {
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
        glRotatef(-90, 1, 0, 0);
    }
}

- (void)drawRect:(NSRect)bounds {
    NSLog(@"DrawRect");
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    [self transformWorld];
    [self drawGrid];
    if(features)
        [self drawFeatures];
    [path drawPath:currentTime maxAge:maxAge];
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