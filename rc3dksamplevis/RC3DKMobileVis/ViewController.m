//
//  ViewController.m
//  RC3DKMobileVis
//
//  Created by Brian on 2/27/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

#import "ViewController.h"
#import "WorldState.h"

/*
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
*/

/* TODO Need to keep state of things to draw in vertex arrays, could be used in common */
/* Need to manipulate the viewpoint more directly. */

#define INITIAL_LIMITS 3.
#define POINT_SIZE 3.0
//#define PERSPECTIVE

typedef struct _VertexData {
    GLfloat position[3];
    GLubyte color[4];
} VertexData;

static VertexData * featureVertex;
static int nfeatures;
static int nfeaturesalloc = 1000;

static VertexData * pathVertex;
static int npath;
static int npathalloc = 1000;

static VertexData * gridVertex;
static int ngrid;

@interface ViewController () {
    float xMin, xMax, yMin, yMax;
    float currentTime;
    RCViewpoint currentViewpoint;
    RCFeatureFilter featuresFilter;
    float currentScale;
    WorldState * state;
}
@property (strong, nonatomic) EAGLContext *context;
@property (strong, nonatomic) GLKBaseEffect *effect;

- (void)setupGL;
- (void)tearDownGL;

@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];

    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

    if (!self.context) {
        NSLog(@"Failed to create ES context");
    }

    self.preferredFramesPerSecond = 60;
    
    GLKView *view = (GLKView *)self.view;
    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;

    state = [[WorldState alloc] init];
    [ConnectionManager sharedInstance].delegate = state;

    [self setupGL];
}

- (void)dealloc
{    
    [self tearDownGL];
    
    if ([EAGLContext currentContext] == self.context) {
        [EAGLContext setCurrentContext:nil];
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];

    if ([self isViewLoaded] && ([[self view] window] == nil)) {
        self.view = nil;
        
        [self tearDownGL];
        
        if ([EAGLContext currentContext] == self.context) {
            [EAGLContext setCurrentContext:nil];
        }
        self.context = nil;
    }

    // Dispose of any resources that can be recreated.
}

- (void)setupGL
{
    [EAGLContext setCurrentContext:self.context];

    self.effect = [[GLKBaseEffect alloc] init];
    self.effect.light0.enabled = GL_TRUE;
    self.effect.light0.ambientColor = GLKVector4Make(1.0f, 1.0f, 1.0f, 1.0f);
    //self.effect.light0.diffuseColor = GLKVector4Make(1.0f, 0.4f, 0.4f, 1.0f);

    glEnable(GL_DEPTH_TEST);

    xMin = -INITIAL_LIMITS;
    xMax = INITIAL_LIMITS;
    yMin = INITIAL_LIMITS;
    yMax = INITIAL_LIMITS;

    pathVertex = calloc(sizeof(VertexData), npathalloc);
    featureVertex = calloc(sizeof(VertexData), nfeaturesalloc);
    [self buildGridVertexData];
    //glOrtho(xMin-xoffset,xMax+xoffset, yMin-yoffset,yMax+yoffset, 10000., -10000.);
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];
    if(pathVertex)
        free(pathVertex);
    if(featureVertex)
        free(featureVertex);
    if(gridVertex)
        free(gridVertex);
}


void setPosition(VertexData * vertex, float x, float y, float z)
{
    vertex->position[0] = x;
    vertex->position[1] = y;
    vertex->position[2] = z;
}

void setColor(VertexData * vertex, GLuint r, GLuint g, GLuint b, GLuint alpha)
{
    vertex->color[0] = r;
    vertex->color[1] = g;
    vertex->color[2] = b;
    vertex->color[3] = alpha;
}

- (void)buildGridVertexData {
    float scale = 1; /* meter */
    ngrid = 21*16 + 6; /* -10 to 10 with 16 each iteration + 6 for axis */
    gridVertex = calloc(sizeof(VertexData), ngrid);
    /* Grid */
    int idx = 0;
    GLuint gridColor[4] = {255, 255, 255, 255};
    for(float x = -10*scale; x < 11*scale; x += scale)
    {
        setColor(&gridVertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        setPosition(&gridVertex[idx++], x, -10*scale, 0);
        setColor(&gridVertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        setPosition(&gridVertex[idx++], x, 10*scale, 0);
        setColor(&gridVertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        setPosition(&gridVertex[idx++], -10*scale, x, 0);
        setColor(&gridVertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        setPosition(&gridVertex[idx++], 10*scale, x, 0);
        setColor(&gridVertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        setPosition(&gridVertex[idx++], -0, -10*scale, 0);
        setColor(&gridVertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        setPosition(&gridVertex[idx++], -0, 10*scale, 0);
        setColor(&gridVertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        setPosition(&gridVertex[idx++], -10*scale, -0, 0);
        setColor(&gridVertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        setPosition(&gridVertex[idx++], 10*scale, -0, 0);

        setColor(&gridVertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        setPosition(&gridVertex[idx++], 0, -.1*scale, x);
        setColor(&gridVertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        setPosition(&gridVertex[idx++], 0, .1*scale, x);
        setColor(&gridVertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        setPosition(&gridVertex[idx++], -.1*scale, 0, x);
        setColor(&gridVertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        setPosition(&gridVertex[idx++], .1*scale, 0, x);
        setColor(&gridVertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        setPosition(&gridVertex[idx++], 0, -.1*scale, -x);
        setColor(&gridVertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        setPosition(&gridVertex[idx++], 0, .1*scale, -x);
        setColor(&gridVertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        setPosition(&gridVertex[idx++], -.1*scale, 0, -x);
        setColor(&gridVertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        setPosition(&gridVertex[idx++], .1*scale, 0, -x);
    }
    /* Axes */
    setColor(&gridVertex[idx], 255, 0, 0, 255);
    setPosition(&gridVertex[idx++], 0, 0, 0);
    setColor(&gridVertex[idx], 255, 0, 0, 255);
    setPosition(&gridVertex[idx++], 1, 0, 0);


    setColor(&gridVertex[idx], 0, 255, 0, 255);
    setPosition(&gridVertex[idx++], 0, 0, 0);
    setColor(&gridVertex[idx], 0, 255, 0, 255);
    setPosition(&gridVertex[idx++], 0, 1, 0);


    setColor(&gridVertex[idx], 0, 0, 255, 255);
    setPosition(&gridVertex[idx++], 0, 0, 0);
    setColor(&gridVertex[idx], 0, 0, 255, 255);
    setPosition(&gridVertex[idx++], 1, 0, 0);
}



#pragma mark - GLKView and GLKViewController delegate methods

- (void)update
{
    /* Build arrays for timeSinceLastUpdate
     */
    NSDictionary * features = [state getFeatures];
    NSArray *path = [state getPath];
    float renderTime = [state getTime];
    int idx;

    // reallocate if we now have more data than room for vertices
    if(nfeaturesalloc < [features count]) {
        featureVertex = realloc(featureVertex, sizeof(VertexData)*[features count]*1.5);
    }
    if(npathalloc < [path count]) {
        pathVertex = realloc(pathVertex, sizeof(VertexData)*[path count]*1.5);
    }

    idx = 0;
    for(id key in features) {
        Feature f;
        NSValue * value = [features objectForKey:key];
        [value getValue:&f];
        if (f.lastSeen == renderTime) {
            featureVertex[idx].color[0] = 255;
            featureVertex[idx].color[1] = 0;
            featureVertex[idx].color[2] = 0;
            featureVertex[idx].color[3] = 255;
        }
        else {
            if (featuresFilter == RCFeatureFilterShowGood && !f.good)
                continue;
            featureVertex[idx].color[0] = 255;
            featureVertex[idx].color[1] = 0;
            featureVertex[idx].color[2] = 0;
            featureVertex[idx].color[3] = 255;
        }
        featureVertex[idx].position[0] = f.x;
        featureVertex[idx].position[1] = f.y;
        featureVertex[idx].position[2] = f.z;
        idx++;
    }
    nfeatures = idx;

    idx = 0;
    for(id location in path)
    {
        Translation t;
        NSValue * value = location;
        [value getValue:&t];
        if (t.time == renderTime) {
            pathVertex[idx].color[0] = 0;
            pathVertex[idx].color[1] = 255;
            pathVertex[idx].color[2] = 0;
            pathVertex[idx].color[3] = 255;
        }
        else {
            pathVertex[idx].color[0] = 0;
            pathVertex[idx].color[1] = 0;
            pathVertex[idx].color[2] = 255;
            pathVertex[idx].color[3] = 255;
        }
        pathVertex[idx].position[0] = t.x;
        pathVertex[idx].position[1] = t.y;
        pathVertex[idx].position[2] = t.z;
    }
    npath = idx;


    //NSLog(@"Nfeatures %d npath %d", nfeatures, npath);

    float aspect = self.view.bounds.size.width / self.view.bounds.size.height;
#ifdef PERSPECTIVE
    aspect = fabsf(aspect);
    GLKMatrix4 projectionMatrix = GLKMatrix4MakePerspective(GLKMathDegreesToRadians(65.0f), aspect, 0.1f, 100.0f);
#else
    float xoffset = 0, yoffset = 0;
    float dx = xMax - xMin;
    float dy = yMax - yMin;
    float dy_scaled = (xMax - xMin) * 1./aspect;
    float dx_scaled = (yMax - yMin) * aspect;
    if(dx_scaled > dx)
        xoffset = (dx_scaled - dx)/2.;
    else if (dy_scaled > dy)
        yoffset = (dy_scaled - dy)/2.;

    GLKMatrix4 projectionMatrix = GLKMatrix4MakeOrtho(xMin-xoffset,xMax+xoffset, yMin-yoffset,yMax+yoffset, 10000., -10000.);
#endif
    self.effect.transform.projectionMatrix = projectionMatrix;

    static float _rotation = 0;
    _rotation += self.timeSinceLastUpdate * 0.5f;

    GLKMatrix4 baseModelViewMatrix = GLKMatrix4MakeTranslation(0.0f, 0.0f, -4.0f);
    baseModelViewMatrix = GLKMatrix4Rotate(baseModelViewMatrix, _rotation, 0.0f, 1.0f, 0.0f);

    // Compute the model view matrix for the object rendered with GLKit
    GLKMatrix4 modelViewMatrix = GLKMatrix4MakeTranslation(0.0f, 0.0f, -1.5f);
    modelViewMatrix = GLKMatrix4Rotate(modelViewMatrix, _rotation, 1.0f, 1.0f, 1.0f);
    modelViewMatrix = GLKMatrix4Multiply(baseModelViewMatrix, modelViewMatrix);

    self.effect.transform.modelviewMatrix = modelViewMatrix;
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
//    glClearColor(0.65f, 0.65f, 0.65f, 1.0f);
    glClearColor(0., 0., 0., 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render the object with GLKit
    [self.effect prepareToDraw];

    //currentScale = 0.5;
    //glDrawArrays(GL_TRIANGLES, 0, 36);

    DrawModel();
}

void DrawModel()
{
    //NSLog(@"Draw model with %d", ngrid);
    enum {ATTRIB_POSITION, ATTRIB_COLOR};

    glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), &gridVertex[0].position);
    glEnableVertexAttribArray(ATTRIB_POSITION);
    glVertexAttribPointer(ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexData), &gridVertex[0].color);
    glEnableVertexAttribArray(ATTRIB_COLOR);

    glLineWidth(8.0f);
    glDrawArrays(GL_LINES, 0, ngrid);

    glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), &featureVertex[0].position);
    glEnableVertexAttribArray(ATTRIB_POSITION);
    glVertexAttribPointer(ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexData), &featureVertex[0].color);
    glEnableVertexAttribArray(ATTRIB_COLOR);

    glPointSize(8.0f);
    glDrawArrays(GL_POINTS, 0, nfeatures);

    glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), &pathVertex[0].position);
    glEnableVertexAttribArray(ATTRIB_POSITION);
    glVertexAttribPointer(ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexData), &pathVertex[0].color);
    glEnableVertexAttribArray(ATTRIB_COLOR);

    glPointSize(8.0f);
    glDrawArrays(GL_POINTS, 0, npath);
}

@end
