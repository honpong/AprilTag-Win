//
//  VisualizationController.m
//  RC3DKSampleApp
//
//  Created by Brian on 3/3/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "VisualizationController.h"
#import "MBProgressHUD.h"
#import "RC3DK.h"
#include "world_state.h"
#include "world_state_render.h"
#include "arcball.h"

#define INITIAL_LIMITS 3.
#define PERSPECTIVE

@interface VisualizationController () {
    /* RC3DK */
    RCSensorManager* sensorManager;
    RCSensorFusion* sensorFusion;
    bool isStarted; // Keeps track of whether the start button has been pressed

    /* OpenGL */
    float xMin, xMax, yMin, yMax;
    float currentTime;
    RCViewpoint currentViewpoint;
    RCFeatureFilter featuresFilter;
    float currentScale;
    float viewpointTime;
    world_state state;

    arcball arc;

    GLKMatrix4 _modelViewMatrix;
    GLKMatrix4 _projectionMatrix;
    GLKMatrix3 _normalMatrix;
    float _rotation;

    MBProgressHUD* progressView;
}
@property (strong, nonatomic) EAGLContext *context;

- (void)setupGL;
- (void)tearDownGL;

@end

@implementation VisualizationController
@synthesize startStopButton, statusLabel;

- (void)viewDidLoad
{
    [super viewDidLoad];

    /* RC3DK Setup */
    sensorManager = [RCSensorManager sharedInstance];
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

    [self setupGL];

    currentScale = 1;
    [self setViewpoint:RCViewpointManual];
    featuresFilter = RCFeatureFilterShowGood;

    progressView = [[MBProgressHUD alloc] initWithView:self.view];
    [self.view addSubview:progressView];
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
    [super viewDidAppear:animated];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(appWillResignActive)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
}

- (void) viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void) appWillResignActive
{
    [self stopSensorFusion];
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
    state.reset();
    [self beginHoldingPeriod];
    [sensorManager startAllSensors];
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
    [self showMessage:@"Move around. The blue line is the path the device traveled. The dots are visual features being tracked. The grid lines are 1 meter apart." autoHide:YES];
}

- (void)stopSensorFusion
{
    [sensorFusion stopSensorFusion];
    [sensorManager stopAllSensors];
    [startStopButton setTitle:@"Start" forState:UIControlStateNormal];
    isStarted = NO;
    [self hideProgress];
}

// RCSensorFusionDelegate delegate method. Called after each video frame is processed ~ 30hz.
- (void)sensorFusionDidUpdateData:(RCSensorFusionData *)data
{
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
            state.observe_feature(sensor_clock::micros_to_tp(data.timestamp), p.id, x, y, z, p.x, p.y, p.x, p.y, 0, 0, 0,good, false);
        }
    }
    RCRotation * R = data.transformation.rotation;
    RCTranslation * T = data.transformation.translation;
    state.observe_position(sensor_clock::micros_to_tp(data.timestamp), T.x, T.y, T.z, R.quaternionW, R.quaternionX, R.quaternionY, R.quaternionZ);
}

// RCSensorFusionDelegate delegate method. Called when sensor fusion is in an error state.
- (void) sensorFusionDidChangeStatus:(RCSensorFusionStatus *)status
{
    if (status.runState == RCSensorFusionRunStateSteadyInitialization)
    {
        [self updateProgress:status.progress];
    }
    else if (status.runState == RCSensorFusionRunStateRunning)
    {
        [self endHoldingPeriod];
    }
    
    if ([status.error isKindOfClass:[RCSensorFusionError class]])
    {
        switch (status.error.code)
        {
            case RCSensorFusionErrorCodeVision:
                [self showMessage:@"Error: The camera cannot see well enough. Could be too dark, camera blocked, or featureless scene." autoHide:YES];
                break;
            case RCSensorFusionErrorCodeTooFast:
                [self showMessage:@"Error: The device was moved too fast. Try moving slower and smoother." autoHide:YES];
                state.reset();
                break;
            case RCSensorFusionErrorCodeOther:
                [self showMessage:@"Error: A fatal error has occured." autoHide:YES];
                state.reset();
                break;
            case RCSensorFusionErrorCodeLicense:
                [self showMessage:@"Error: License was not validated before startProcessingVideo was called." autoHide:YES];
                break;
            case RCSensorFusionErrorCodeNone:
                break;
            default:
                [self showMessage:@"Error: Unknown." autoHide:YES];
                break;
        }
    }
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
    [self setViewpoint:(RCViewpoint)nextView];
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer
{
    return YES;
}

- (IBAction)handlePanGesture:(UIPanGestureRecognizer*)sender
{
    CGPoint translation = [sender translationInView:self.view];
    
    if (sender.state == UIGestureRecognizerStateBegan)
        arc.start_rotation(translation.x, translation.y);
    else if (sender.state == UIGestureRecognizerStateChanged)
        arc.continue_rotation(translation.x, translation.y);
}

- (IBAction)handlePinchGesture:(UIPinchGestureRecognizer*)sender
{
    UIPinchGestureRecognizer *pg = (UIPinchGestureRecognizer *)sender;
    
    if (pg.state == UIGestureRecognizerStateChanged)
    {
        if (pg.scale > 1)
        {
            float delta = (pg.scale - 1);
            if (delta > .05) delta = .05;
            if (currentScale < 4) currentScale += delta;
        }
        else
        {
            float delta = (1 - pg.scale);
            if (delta > .05) delta = .05;
            if (currentScale > .3) currentScale -= delta;
        }
    }
}

- (IBAction)handleRotationGesture:(UIRotationGestureRecognizer*)sender
{
    if(sender.state == UIGestureRecognizerStateBegan)
        arc.start_view_rotation(sender.rotation);
    else if(sender.state == UIGestureRecognizerStateChanged)
        arc.continue_view_rotation(sender.rotation);
}

#pragma OpenGL Visualization

- (void)setupGL
{
    [EAGLContext setCurrentContext:self.context];

    world_state_render_init();

    glEnable(GL_DEPTH_TEST);

    xMin = -INITIAL_LIMITS;
    xMax = INITIAL_LIMITS;
    yMin = -INITIAL_LIMITS;
    yMax = INITIAL_LIMITS;
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];

    world_state_render_teardown();
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
        quaternion q = arc.get_quaternion();
        GLKMatrix4 user_rotation = GLKMatrix4MakeWithQuaternion(GLKQuaternionMake(q.x(), q.y(), q.z(), q.w()));

        modelView = GLKMatrix4Multiply(modelView, user_rotation);
        modelView = GLKMatrix4Multiply(GLKMatrix4MakeTranslation(0, 0, -6), modelView);
    }

    return modelView;
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)update
{

    state.update_vertex_arrays(featuresFilter == RCFeatureFilterShowGood);

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


    GLKMatrix4 modelViewMatrix = GLKMatrix4MakeScale(currentScale, currentScale, currentScale);
    modelViewMatrix = [self rotateCamera:self.timeSinceLastUpdate withModelView:modelViewMatrix];
    _modelViewMatrix = modelViewMatrix;
    _projectionMatrix = projectionMatrix;
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    glClearColor(.274, .286, .349, 1.0f); // background color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    world_state_render(&state, _modelViewMatrix.m, _projectionMatrix.m);
}

- (void)setViewpoint:(RCViewpoint)viewpoint
{
    switch (viewpoint) {
        case RCViewpointManual:
            [self showMessage:@"View Mode: Manual" autoHide:YES];
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

@end
