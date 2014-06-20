//
//  VisualizationController.m
//  RC3DKSampleApp
//
//  Created by Brian on 3/3/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "VisualizationController.h"
#import "LicenseHelper.h"
#import "ArcBall.h"
#import "WorldState.h"
#import "MBProgressHUD.h"
#import "RCSensorDelegate.h"

#define INITIAL_LIMITS 3.
#define POINT_SIZE 3.0
#define PERSPECTIVE

// Uniform index for Shader.vsh
enum
{
    UNIFORM_MODELVIEWPROJECTION_MATRIX,
    UNIFORM_NORMAL_MATRIX,
    NUM_UNIFORMS
};
GLint uniforms[NUM_UNIFORMS];

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

static VertexData axisVertex[] = {
    {{0, 0, 0}, {221, 141, 81, 255}},
    {{.5, 0, 0}, {221, 141, 81, 255}},
    {{0, 0, 0}, {0, 201, 89, 255}},
    {{0, .5, 0}, {0, 201, 89, 255}},
    {{0, 0, 0}, {247, 88, 98, 255}},
    {{0, 0, .5}, {247, 88, 98, 255}},
};

@interface VisualizationController () {
    /* RC3DK */
    RCSensorFusion* sensorFusion;
    id<RCSensorDelegate> sensorDelegate;
    bool isStarted; // Keeps track of whether the start button has been pressed

    /* OpenGL */
    float xMin, xMax, yMin, yMax;
    float currentTime;
    RCViewpoint currentViewpoint;
    RCFeatureFilter featuresFilter;
    float currentScale;
    float originalScale;
    float viewpointTime;
    WorldState * state;

    ArcBall * arcball;

    GLuint _program;

    GLKMatrix4 _modelViewProjectionMatrix;
    GLKMatrix3 _normalMatrix;
    float _rotation;

    GLuint _vertexArray;
    GLuint _vertexBuffer;
    
    MBProgressHUD* progressView;
    
    RCSensorFusionRunState currentRunState;
}

@property (strong, nonatomic) EAGLContext *context;
@property (strong, nonatomic) GLKBaseEffect *effect;

- (void)setupGL;
- (void)tearDownGL;

@end

@implementation VisualizationController
@synthesize startStopButton, statusLabel;

- (void)viewDidLoad
{
    [super viewDidLoad];

    sensorDelegate = [SensorDelegate sharedInstance];
    /* RC3DK Setup */
    sensorFusion = [RCSensorFusion sharedInstance]; // The main class of the 3DK framework
    sensorFusion.delegate = self; // Tells RCSensorFusion where to send data to

    isStarted = false;
    [startStopButton setTitle:@"Start" forState:UIControlStateNormal];

    [self doSanityCheck];

    /* OpenGL Setup */
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

    if (!self.context) {
        NSLog(@"Failed to create ES context");
    }

    self.preferredFramesPerSecond = 60;

    GLKView *view = (GLKView *)self.view;
    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
    view.drawableMultisample = GLKViewDrawableMultisample4X;

    state = [WorldState sharedInstance];

    [self setupGL];

    currentScale = 1;
    [self setViewpoint:RCViewpointManual];
    featuresFilter = RCFeatureFilterShowGood;

    arcball = [[ArcBall alloc] init];
    
    progressView = [[MBProgressHUD alloc] initWithView:self.view];
    [self.view addSubview:progressView];
    
    currentRunState = RCSensorFusionRunStateInactive;
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

- (void) viewDidAppear:(BOOL)animated
{
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(appWillResignActive)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
    
    [self showInstructions];
}

- (void) viewWillDisappear:(BOOL)animated
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void) appWillResignActive
{
    [self stopSensorFusion];
}

- (void)showProgressWithTitle:(NSString*)title
{
    progressView.mode = MBProgressHUDModeAnnularDeterminate;
    progressView.labelText = title;
    [progressView show:YES];
}

- (void)hideProgress
{
    [progressView hide:YES];
}

- (void)updateProgress:(float)progress
{
    [progressView setProgress:progress];
}

- (void) showInstructions
{
    [self showMessage:@"Point the camera straight ahead and press Start. For best results, hold the device firmly with two hands." autoHide:NO];
}

- (void) doSanityCheck
{
    if ([RCDeviceInfo getDeviceType] == DeviceTypeUnknown)
    {
        [self showMessage:@"Warning: This device is not supported by 3DK." autoHide:YES];
        return;
    }

#ifdef DEBUG
    [self showMessage:@"Warning: You are running a debug build. The performance will be better with an optimized build." autoHide:YES];
#endif
}

- (void)startSensorFusion
{
    [state reset];
    [self beginHoldingPeriod];
    [sensorDelegate startAllSensors];
    [startStopButton setTitle:@"Stop" forState:UIControlStateNormal];
    isStarted = YES;
    statusLabel.text = @"";
    [[RCSensorFusion sharedInstance] startSensorFusionWithDevice:[[RCAVSessionManager sharedInstance] videoDevice]];
}

- (void) beginHoldingPeriod
{
    [self hideMessage];
    [self showProgressWithTitle:@"Hold still"];
}

- (void) endHoldingPeriod
{
    [self hideProgress];
    [self showMessage:@"Move around. The blue line is the path the device traveled. The dots are visual features being tracked. The grid lines are 1 meter apart." autoHide:NO];
}

- (void)stopSensorFusion
{
    [sensorFusion stopSensorFusion];
    [sensorDelegate stopAllSensors];
    [startStopButton setTitle:@"Start" forState:UIControlStateNormal];
    [self showInstructions];
    isStarted = NO;
    [self hideProgress];
}

#pragma mark - RCSensorFusionDelegate

// RCSensorFusionDelegate delegate method. Called after each video frame is processed ~ 30hz.
- (void)sensorFusionDidUpdateData:(RCSensorFusionData *)data
{
    [self updateVisualization:data];
}

// RCSensorFusionDelegate delegate method. Called when sensor fusion status changes.
- (void)sensorFusionDidChangeStatus:(RCSensorFusionStatus*)status
{
    if ([status.error isKindOfClass:[RCSensorFusionError class]])
    {
        [self handleSensorFusionError:(RCSensorFusionError*)status.error];
    }
    else if ([status.error isKindOfClass:[RCLicenseError class]])
    {
        [self handleLicenseProblem:(RCLicenseError*)status.error];
    }
    else if(status.runState == RCSensorFusionRunStateSteadyInitialization)
    {
        [self updateProgress:status.progress];
    }
    else if(currentRunState == RCSensorFusionRunStateSteadyInitialization && status.runState != currentRunState)
    {
        [self endHoldingPeriod];
    }
    
    currentRunState = status.runState;
}

#pragma mark -

- (void) handleSensorFusionError:(RCSensorFusionError*)error
{
    switch (error.code)
    {
        case RCSensorFusionErrorCodeVision:
            [self showMessage:@"Warning: The camera cannot see well enough. Could be too dark, camera blocked, or featureless scene." autoHide:YES];
            break;
        case RCSensorFusionErrorCodeTooFast:
            [self stopSensorFusion];
            [self showMessage:@"Error: The device was moved too fast. Try moving slower and more smoothly." autoHide:NO];
            break;
        case RCSensorFusionErrorCodeOther:
            [self stopSensorFusion];
            [self showMessage:@"Error: A fatal error has occured." autoHide:NO];
            break;
        default:
            [self stopSensorFusion];
            [self showMessage:@"Error: Unknown." autoHide:NO];
            break;
    }
}

- (void) handleLicenseProblem:(RCLicenseError*)error
{
    statusLabel.text = @"Error: License problem";
    [LicenseHelper showLicenseValidationError:error];
    [self stopSensorFusion];
}

- (void)updateVisualization:(RCSensorFusionData *)data
{
    double time = data.timestamp / 1.0e6;

    for (id object in data.featurePoints)
    {
        RCFeaturePoint * p = object;
        if([p initialized])
        {
            //NSDictionary * worldPoint = [f objectForKey:@"worldPoint"];
            float x = p.worldPoint.x;
            float y = p.worldPoint.y;
            float z = p.worldPoint.z;
            float depth = p.originalDepth.scalar;
            float stddev = p.originalDepth.standardDeviation;
            bool good = stddev / depth < .02;
            [state observeFeatureWithId:p.id x:x y:y z:z lastSeen:time good:good];
        }
    }
    [state observePathWithTranslationX:data.transformation.translation.x y:data.transformation.translation.y z:data.transformation.translation.z time:time];
    [state observeTime:time];
}

// Event handler for the start/stop button
- (IBAction)startButtonTapped:(id)sender
{
    if (isStarted)
    {
        [self stopSensorFusion];
    }
    else
    {
        [self startSensorFusion];
    }
}

- (IBAction)changeViewButtonTapped:(id)sender
{
    int nextView = currentViewpoint + 1;
    if(nextView > RCViewpointSide) nextView = 0;
    [self setViewpoint:nextView];
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer
{
    return YES;
}

- (IBAction)handlePanGesture:(UIPanGestureRecognizer*)sender
{
    CGPoint translation = [sender translationInView:self.view];
    
    if (sender.state == UIGestureRecognizerStateBegan)
        [arcball startRotation:translation];
    else if (sender.state == UIGestureRecognizerStateChanged)
        [arcball continueRotation:translation];
}

- (IBAction)handlePinchGesture:(UIPinchGestureRecognizer*)sender
{
    UIPinchGestureRecognizer *pg = (UIPinchGestureRecognizer *)sender;
    
    if (pg.state == UIGestureRecognizerStateBegan)
    {
        originalScale = currentScale;
    }
    if (pg.state == UIGestureRecognizerStateChanged)
    {
        float prev = currentScale;
        currentScale = pg.scale * originalScale;
        if(prev <= .3 && currentScale > .3) {
            [self buildGridVertexDataWithScale:1];
            [self showMessage:@"Scale changed to 1 meter per gridline." autoHide:true];
        }
        else if(prev > .3 && currentScale <= .3) {
            [self buildGridVertexDataWithScale:10];
            [self showMessage:@"Scale changed to 10 meters per gridline." autoHide:true];
        }
    }
}

- (IBAction)handleRotationGesture:(UIRotationGestureRecognizer*)sender
{
    if(sender.state == UIGestureRecognizerStateBegan)
        [arcball startViewRotation:sender.rotation];
    else if(sender.state == UIGestureRecognizerStateChanged)
        [arcball continueViewRotation:sender.rotation];
}

#pragma OpenGL Visualization

- (void)setupGL
{
    [EAGLContext setCurrentContext:self.context];

    [self loadShaders];

    self.effect = [[GLKBaseEffect alloc] init];
    self.effect.light0.enabled = GL_FALSE;
    self.effect.light0.ambientColor = GLKVector4Make(1.0f, 1.0f, 1.0f, 1.0f);
    self.effect.lightModelTwoSided = GL_TRUE;

    glEnable(GL_DEPTH_TEST);

    xMin = -INITIAL_LIMITS;
    xMax = INITIAL_LIMITS;
    yMin = -INITIAL_LIMITS;
    yMax = INITIAL_LIMITS;

    pathVertex = calloc(sizeof(VertexData), npathalloc);
    featureVertex = calloc(sizeof(VertexData), nfeaturesalloc);
    [self buildGridVertexDataWithScale:1];
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

    if (_program) {
        glDeleteProgram(_program);
        _program = 0;
    }
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

- (void)buildGridVertexDataWithScale:(float)scale
{
    ngrid = 21*16; /* -10 to 10 with 16 each iteration */
    gridVertex = calloc(sizeof(VertexData), ngrid);
    /* Grid */
    int idx = 0;
    GLuint gridColor[4] = {122, 126, 146, 255};
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
}

- (GLKMatrix4)animateCamera:(float)timeSinceLastUpdate withModelView:(GLKMatrix4)modelView
{
    // Rotate 90 degrees, wait 1 second, rotate 90 degrees, wait 1 second, rotate back, wait 3 seconds
    float scale = 1;

    viewpointTime += timeSinceLastUpdate;
    float time = viewpointTime;

    if(time < 2*scale)
    {
        modelView = GLKMatrix4Multiply(GLKMatrix4MakeRotation(-M_PI_2 * time / (2*scale), 1, 0, 0), modelView);
    }
    else if(time >= 2*scale && time < 3*scale)
    {
        // do nothing
        modelView = GLKMatrix4Multiply(GLKMatrix4MakeRotation(-M_PI_2, 1, 0, 0), modelView);
    }
    else if(time >= 3*scale && time < 5*scale)
    {
        GLKMatrix4 firstRotation = GLKMatrix4Multiply(GLKMatrix4MakeRotation(-M_PI_2, 1, 0, 0), modelView);
        GLKMatrix4 secondRotation = GLKMatrix4MakeRotation(-M_PI_2 * (time-(3*scale))/(2*scale), 0, 1, 0);
        modelView = GLKMatrix4Multiply(secondRotation, firstRotation);
    }
    else if(time >= 5*scale && time < 6*scale)
    {
        GLKMatrix4 firstRotation = GLKMatrix4Multiply(GLKMatrix4MakeRotation(-M_PI_2, 1, 0, 0),  modelView);
        GLKMatrix4 secondRotation = GLKMatrix4MakeRotation(-M_PI_2, 0, 1, 0);
        modelView = GLKMatrix4Multiply(secondRotation, firstRotation);
    }
    else if(time >= 6*scale && time < 8*scale)
    {
        GLKMatrix4 firstRotation = GLKMatrix4Multiply(GLKMatrix4MakeRotation(-M_PI_2 * (8*scale - time)/(2*scale), 1, 0, 0), modelView);
        GLKMatrix4 secondRotation = GLKMatrix4MakeRotation(-M_PI_2 * (8*scale - time)/(2*scale), 0, 1, 0);
        modelView = GLKMatrix4Multiply(secondRotation, firstRotation);
    }
    else if(time >= 8*scale)
    {
        // do nothing
        modelView = GLKMatrix4Multiply(GLKMatrix4MakeTranslation(0, 0, 0), modelView);
    }
    else
        NSLog(@"Animated rendering didn't know what to do for time %f", time);

    modelView = GLKMatrix4Multiply(GLKMatrix4MakeTranslation(0, 0, -6.), modelView);

    return modelView;
}

- (GLKMatrix4)rotateCamera:(float)timeSinceLastUpdate withModelView:(GLKMatrix4)modelView
{
    if(currentViewpoint == RCViewpointTopDown)
        modelView = GLKMatrix4Multiply(GLKMatrix4MakeTranslation(0.0f, 0.0f, -6.f), modelView);
    else if(currentViewpoint == RCViewpointSide) {
        modelView = GLKMatrix4Multiply(GLKMatrix4MakeRotation(-M_PI_2, 0, 1, 0), modelView);
        modelView = GLKMatrix4Multiply(GLKMatrix4MakeRotation(-M_PI_2, 0, 0, 1), modelView);
        modelView = GLKMatrix4Multiply(GLKMatrix4MakeTranslation(0, 0, -6), modelView);
    }
    else if(currentViewpoint == RCViewpointAnimating) {
        modelView = [self animateCamera:timeSinceLastUpdate withModelView:modelView];
    }
    else { // currentViewpoint == RCViewpointManual
        modelView = [arcball rotateMatrixByArcBall:modelView];
        modelView = GLKMatrix4Multiply(GLKMatrix4MakeTranslation(0, 0, -6), modelView);
    }

    return modelView;
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)update
{
    /*
     * Build vertex arrays for feature and path data
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
        if (f.lastSeen == renderTime)
            setColor(&featureVertex[idx], 247, 88, 98, 255); // bad feature
        else {
            if (featuresFilter == RCFeatureFilterShowGood && !f.good)
                continue;
            setColor(&featureVertex[idx], 255, 255, 255, 255); // good feature
        }
        setPosition(&featureVertex[idx], f.x, f.y, f.z);
        idx++;
    }
    nfeatures = idx;

    idx = 0;
    for(id location in path)
    {
        Translation t;
        NSValue * value = location;
        [value getValue:&t];
        if (t.time == renderTime)
            setColor(&pathVertex[idx], 0, 255, 0, 255);
        else
            setColor(&pathVertex[idx], 0, 178, 206, 255); // path color
        setPosition(&pathVertex[idx], t.x, t.y, t.z);
        idx++;
    }
    npath = idx;

    /*
     * Build modelView matrix
     */

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


    GLKMatrix4 modelViewMatrix = GLKMatrix4MakeScale(currentScale, currentScale, currentScale);
    modelViewMatrix = [self rotateCamera:self.timeSinceLastUpdate withModelView:modelViewMatrix];
    _modelViewProjectionMatrix = GLKMatrix4Multiply(projectionMatrix, modelViewMatrix);

    self.effect.transform.modelviewMatrix = modelViewMatrix;
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    glClearColor(.274, .286, .349, 1.0f); // background color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render the object with GLKit
    [self.effect prepareToDraw];

    glUseProgram(_program);

    glUniformMatrix4fv(uniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, _modelViewProjectionMatrix.m);

    glUniformMatrix3fv(uniforms[UNIFORM_NORMAL_MATRIX], 1, 0, _normalMatrix.m);

    DrawModel();
}

void DrawModel()
{
    glVertexAttribPointer(GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), &gridVertex[0].position);
    glEnableVertexAttribArray(GLKVertexAttribPosition);
    glVertexAttribPointer(GLKVertexAttribColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexData), &gridVertex[0].color);
    glEnableVertexAttribArray(GLKVertexAttribColor);

    glLineWidth(4.0f);
    glDrawArrays(GL_LINES, 0, ngrid);

    glVertexAttribPointer(GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), &axisVertex[0].position);
    glEnableVertexAttribArray(GLKVertexAttribPosition);
    glVertexAttribPointer(GLKVertexAttribColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexData), &axisVertex[0].color);
    glEnableVertexAttribArray(GLKVertexAttribColor);

    glLineWidth(8.0f);
    glDrawArrays(GL_LINES, 0, 6);

    glVertexAttribPointer(GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), &featureVertex[0].position);
    glEnableVertexAttribArray(GLKVertexAttribPosition);
    glVertexAttribPointer(GLKVertexAttribColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexData), &featureVertex[0].color);
    glEnableVertexAttribArray(GLKVertexAttribColor);

    glDrawArrays(GL_POINTS, 0, nfeatures);

    glVertexAttribPointer(GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), &pathVertex[0].position);
    glEnableVertexAttribArray(GLKVertexAttribPosition);
    glVertexAttribPointer(GLKVertexAttribColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexData), &pathVertex[0].color);
    glEnableVertexAttribArray(GLKVertexAttribColor);

    glDrawArrays(GL_POINTS, 0, npath);
}

- (void)setViewpoint:(RCViewpoint)viewpoint
{
    switch (viewpoint) {
        case RCViewpointManual:
            [self showMessage:@"View Mode: Tap and drag." autoHide:YES];
            break;
        case RCViewpointAnimating:
            [self showMessage:@"View Mode: Animation" autoHide:YES];
            break;
        case RCViewpointTopDown:
            [self showMessage:@"View Mode: Top Down" autoHide:YES];
            break;
        case RCViewpointSide:
            [self showMessage:@"View Mode: Side" autoHide:YES];
            break;
            
        default:
            break;
    }
    
    viewpointTime = 0;
    currentViewpoint = viewpoint;
}

#pragma mark -  OpenGL ES 2 shader compilation

- (BOOL)loadShaders
{
    GLuint vertShader, fragShader;
    NSString *vertShaderPathname, *fragShaderPathname;

    // Create shader program.
    _program = glCreateProgram();

    // Create and compile vertex shader.
    vertShaderPathname = [[NSBundle mainBundle] pathForResource:@"Shader" ofType:@"vsh"];
    if (![self compileShader:&vertShader type:GL_VERTEX_SHADER file:vertShaderPathname]) {
        NSLog(@"Failed to compile vertex shader");
        return NO;
    }

    // Create and compile fragment shader.
    fragShaderPathname = [[NSBundle mainBundle] pathForResource:@"Shader" ofType:@"fsh"];
    if (![self compileShader:&fragShader type:GL_FRAGMENT_SHADER file:fragShaderPathname]) {
        NSLog(@"Failed to compile fragment shader");
        return NO;
    }

    // Attach vertex shader to program.
    glAttachShader(_program, vertShader);

    // Attach fragment shader to program.
    glAttachShader(_program, fragShader);

    // Bind attribute locations.
    // This needs to be done prior to linking.
    glBindAttribLocation(_program, GLKVertexAttribPosition, "position");
    glBindAttribLocation(_program, GLKVertexAttribColor, "color");
    glBindAttribLocation(_program, GLKVertexAttribNormal, "normal");

    // Link program.
    if (![self linkProgram:_program]) {
        NSLog(@"Failed to link program: %d", _program);

        if (vertShader) {
            glDeleteShader(vertShader);
            vertShader = 0;
        }
        if (fragShader) {
            glDeleteShader(fragShader);
            fragShader = 0;
        }
        if (_program) {
            glDeleteProgram(_program);
            _program = 0;
        }

        return NO;
    }

    // Get uniform locations.
    uniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation(_program, "modelViewProjectionMatrix");
    uniforms[UNIFORM_NORMAL_MATRIX] = glGetUniformLocation(_program, "normalMatrix");

    // Release vertex and fragment shaders.
    if (vertShader) {
        glDetachShader(_program, vertShader);
        glDeleteShader(vertShader);
    }
    if (fragShader) {
        glDetachShader(_program, fragShader);
        glDeleteShader(fragShader);
    }

    return YES;
}

- (BOOL)compileShader:(GLuint *)shader type:(GLenum)type file:(NSString *)file
{
    GLint status;
    const GLchar *source;

    source = (GLchar *)[[NSString stringWithContentsOfFile:file encoding:NSUTF8StringEncoding error:nil] UTF8String];
    if (!source) {
        NSLog(@"Failed to load vertex shader");
        return NO;
    }

    *shader = glCreateShader(type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);

#if defined(DEBUG)
    GLint logLength;
    glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetShaderInfoLog(*shader, logLength, &logLength, log);
        NSLog(@"Shader compile log:\n%s", log);
        free(log);
    }
#endif

    glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
    if (status == 0) {
        glDeleteShader(*shader);
        return NO;
    }

    return YES;
}

- (BOOL)linkProgram:(GLuint)prog
{
    GLint status;
    glLinkProgram(prog);

#if defined(DEBUG)
    GLint logLength;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        NSLog(@"Program link log:\n%s", log);
        free(log);
    }
#endif

    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status == 0) {
        return NO;
    }

    return YES;
}

- (BOOL)validateProgram:(GLuint)prog
{
    GLint logLength, status;

    glValidateProgram(prog);
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        NSLog(@"Program validate log:\n%s", log);
        free(log);
    }
    
    glGetProgramiv(prog, GL_VALIDATE_STATUS, &status);
    if (status == 0) {
        return NO;
    }
    
    return YES;
}

- (void)showMessage:(NSString*)message autoHide:(BOOL)hide
{
    if (message && message.length > 0)
    {
        self.statusLabel.hidden = NO;
        self.statusLabel.alpha = 1;
        self.statusLabel.text = message ? message : @"";
        
        if (hide) [self fadeOut:self.statusLabel withDuration:2 andWait:3];
    }
    else
    {
        [self hideMessage];
    }
}

- (void)hideMessage
{
    [self fadeOut:self.statusLabel withDuration:0.5 andWait:0];
}

-(void)fadeOut:(UIView*)viewToDissolve withDuration:(NSTimeInterval)duration andWait:(NSTimeInterval)wait
{
    [UIView beginAnimations: @"Fade Out" context:nil];
    
    // wait for time before begin
    [UIView setAnimationDelay:wait];
    
    // duration of animation
    [UIView setAnimationDuration:duration];
    viewToDissolve.alpha = 0.0;
    [UIView commitAnimations];
}

@end
