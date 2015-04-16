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
#include "arcball.h"

#define INITIAL_LIMITS 3.
#define PERSPECTIVE

// Uniform index for Shader.vsh
enum
{
    UNIFORM_MODELVIEWPROJECTION_MATRIX,
    UNIFORM_NORMAL_MATRIX,
    NUM_UNIFORMS
};
GLint uniforms[NUM_UNIFORMS];

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

    GLuint _program;

    GLKMatrix4 _modelViewProjectionMatrix;
    GLKMatrix3 _normalMatrix;
    float _rotation;

    GLuint _vertexArray;
    GLuint _vertexBuffer;
    
    MBProgressHUD* progressView;
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

    //state = [WorldState sharedInstance];

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
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(appWillResignActive)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
}

- (void) viewWillDisappear:(BOOL)animated
{
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
    [self showMessage:@"Move around. The blue line is the path the device traveled. The dots are visual features being tracked. The grid lines are 1 meter apart." autoHide:NO];
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
            state.observe_feature(data.timestamp, p.id, x, y, z, good);
        }
    }
    RCRotation * R = data.transformation.rotation;
    RCTranslation * T = data.transformation.translation;
    state.observe_position(data.timestamp, T.x, T.y, T.z, R.quaternionW, R.quaternionX, R.quaternionY, R.quaternionZ);
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
        [self hideProgress];
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
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];

    if (_program) {
        glDeleteProgram(_program);
        _program = 0;
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

    [self DrawModel];
}

- (void) DrawModel
{
    state.display_lock.lock();
    glVertexAttribPointer(GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), &state.grid_vertex[0].position);
    glEnableVertexAttribArray(GLKVertexAttribPosition);
    glVertexAttribPointer(GLKVertexAttribColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexData), &state.grid_vertex[0].color);
    glEnableVertexAttribArray(GLKVertexAttribColor);

    glLineWidth(4.0f);
    glDrawArrays(GL_LINES, 0, state.grid_vertex_num);

    glVertexAttribPointer(GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), &state.axis_vertex[0].position);
    glEnableVertexAttribArray(GLKVertexAttribPosition);
    glVertexAttribPointer(GLKVertexAttribColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexData), &state.axis_vertex[0].color);
    glEnableVertexAttribArray(GLKVertexAttribColor);

    glLineWidth(8.0f);
    glDrawArrays(GL_LINES, 0, state.axis_vertex_num);

    glVertexAttribPointer(GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), &state.feature_vertex[0].position);
    glEnableVertexAttribArray(GLKVertexAttribPosition);
    glVertexAttribPointer(GLKVertexAttribColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexData), &state.feature_vertex[0].color);
    glEnableVertexAttribArray(GLKVertexAttribColor);

    glDrawArrays(GL_POINTS, 0, state.feature_vertex_num);

    glVertexAttribPointer(GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), &state.path_vertex[0].position);
    glEnableVertexAttribArray(GLKVertexAttribPosition);
    glVertexAttribPointer(GLKVertexAttribColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexData), &state.path_vertex[0].color);
    glEnableVertexAttribArray(GLKVertexAttribColor);

    glDrawArrays(GL_POINTS, 0, state.path_vertex_num);
    state.display_lock.unlock();
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

#pragma mark -  OpenGL ES 2 shader compilation

- (BOOL)loadShaders
{
    GLuint vertShader, fragShader;
    NSString *vertShaderPathname, *fragShaderPathname;

    // Create shader program.
    _program = glCreateProgram();

    // Create and compile vertex shader.
    vertShaderPathname = [[NSBundle mainBundle] pathForResource:@"shader" ofType:@"vsh"];
    if (![self compileShader:&vertShader type:GL_VERTEX_SHADER file:vertShaderPathname]) {
        NSLog(@"Failed to compile vertex shader");
        return NO;
    }

    // Create and compile fragment shader.
    fragShaderPathname = [[NSBundle mainBundle] pathForResource:@"shader" ofType:@"fsh"];
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
