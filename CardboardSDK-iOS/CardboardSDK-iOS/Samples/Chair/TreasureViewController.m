//
//  TreasureViewController.m
//  CardboardSDK-iOS
//
//

#import "TreasureViewController.h"

#include "CardboardSDK.h"

#import <AudioToolbox/AudioServices.h>
#import <OpenGLES/ES2/glext.h>
#import "RCGLShaderProgram.h"
#include "chair_chesterfield.h"


@interface TreasureRenderer : NSObject <StereoRendererDelegate>
{
    GLuint _chairVertexArray;
    GLuint _chairVertexBuffer;
    GLuint _chairColorBuffer;
    GLuint _chairNormalBuffer;
    
    GLuint _floorVertexArray;
    GLuint _floorVertexBuffer;
    GLuint _floorColorBuffer;
    GLuint _floorNormalBuffer;

    GLuint _ceilingColorBuffer;

    GLuint _floorProgram;
    

    GLint _floorPositionLocation;
    GLint _floorNormalLocation;
    GLint _floorColorLocation;
    
    GLint _floorModelLocation;
    GLint _floorModelViewLocation;
    GLint _floorModelViewProjectionLocation;
    GLint _floorLightPositionLocation;

    GLKMatrix4 _perspective;
    GLKMatrix4 _modelChair;
    GLKMatrix4 _camera;
    GLKMatrix4 _view;
    GLKMatrix4 _modelViewProjection;
    GLKMatrix4 _modelView;
    GLKMatrix4 _modelFloor;
    GLKMatrix4 _modelCeiling;
    GLKMatrix4 _headView;
    
    float _zNear;
    float _zFar;
    
    float _cameraZ;
    float _timeDelta;
    
    float _yawLimit;
    float _pitchLimit;
    
    int _coordsPerVertex;
    
    GLKVector4 _lightPositionInWorldSpace;
    GLKVector4 _lightPositionInEyeSpace;
    
    int _score;
    float _objectDistance;
    float _floorDepth;
    
    RCGLShaderProgram *chairProgram;
    GLKTextureInfo *texture;

    BOOL isSensorFusionRunning;
}

@property (weak, nonatomic) CardboardViewController *cardboard;

@end


@implementation TreasureRenderer

- (instancetype)init
{
    self = [super init];
    if (!self) { return nil; }
    
    _objectDistance = 1.5f;
    _floorDepth = 1.5f;
    
    _zNear = 0.1f;
    _zFar = 100.0f;
    
    _cameraZ = 0.01f;
    
    _timeDelta = 1.0f;
    
    _yawLimit = 0.12f;
    _pitchLimit = 0.12f;
    
    _coordsPerVertex = 3;
    
    // We keep the light always position just above the user.
    _lightPositionInWorldSpace = GLKVector4Make(0.0f, 2.0f, 0.0f, 1.0f);
    _lightPositionInEyeSpace = GLKVector4Make(0.0f, 0.0f, 0.0f, 0.0f);
    
    isSensorFusionRunning = NO;

    return self;
}

- (void)setupRendererWithView:(GLKView *)GLView
{
    [EAGLContext setCurrentContext:GLView.context];

    [self setupPrograms];

    [self setupVAOS];
    
    // Etc
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.2f, 0.2f, 0.5f); // Dark background so text shows up well.
    
    // Object first appears directly in front of user.
    _modelChair = GLKMatrix4Identity;
    _modelChair = GLKMatrix4Translate(_modelChair, 0, -_floorDepth, -_objectDistance);
    _modelChair = GLKMatrix4Scale(_modelChair, 1.3, 1.3, 1.3);
    _modelChair = GLKMatrix4Translate(_modelChair, 0., .5, 0.);
    _modelChair = GLKMatrix4RotateY(_modelChair, M_PI);

    _modelFloor = GLKMatrix4Identity;
    _modelFloor = GLKMatrix4Translate(_modelFloor, 0, -_floorDepth, 0); // Floor appears below user.

    _modelCeiling = GLKMatrix4Identity;
    _modelCeiling = GLKMatrix4Translate(_modelFloor, 0, 2. * _floorDepth, 0); // Ceiling appears above user.

    GLCheckForError();
    
    UITapGestureRecognizer *tapRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTap:)];
    tapRecognizer.numberOfTapsRequired = 1;
    [GLView addGestureRecognizer:tapRecognizer];
}

- (BOOL)setupPrograms
{
    NSString *path = nil;
    
    GLuint vertexShader = 0;
    path = [[NSBundle mainBundle] pathForResource:@"light_vertex" ofType:@"shader"];
    if (!GLCompileShaderFromFile(&vertexShader, GL_VERTEX_SHADER, path)) {
        NSLog(@"Failed to compile shader at %@", path);
        return NO;
    }
    
    GLuint gridVertexShader = 0;
    path = [[NSBundle mainBundle] pathForResource:@"light_vertex_grid" ofType:@"shader"];
    if (!GLCompileShaderFromFile(&gridVertexShader, GL_VERTEX_SHADER, path)) {
        NSLog(@"Failed to compile shader at %@", path);
        return NO;
    }
    
    GLuint gridFragmentShader = 0;
    path = [[NSBundle mainBundle] pathForResource:@"grid_fragment" ofType:@"shader"];
    if (!GLCompileShaderFromFile(&gridFragmentShader, GL_FRAGMENT_SHADER, path)) {
        NSLog(@"Failed to compile shader at %@", path);
        return NO;
    }
    
    GLuint passthroughFragmentShader = 0;
    path = [[NSBundle mainBundle] pathForResource:@"passthrough_fragment" ofType:@"shader"];
    if (!GLCompileShaderFromFile(&passthroughFragmentShader, GL_FRAGMENT_SHADER, path)) {
        NSLog(@"Failed to compile shader at %@", path);
        return NO;
    }
    
    GLuint highlightFragmentShader = 0;
    path = [[NSBundle mainBundle] pathForResource:@"highlight_fragment" ofType:@"shader"];
    if (!GLCompileShaderFromFile(&highlightFragmentShader, GL_FRAGMENT_SHADER, path)) {
        NSLog(@"Failed to compile shader at %@", path);
        return NO;
    }
    
    chairProgram = [[RCGLShaderProgram alloc] init];
    [chairProgram buildWithVertexFileName:@"shader.vsh" withFragmentFileName:@"shader.fsh"];
    glUseProgram(chairProgram.program);
    
    GLCheckForError();
    
    _floorProgram = glCreateProgram();
    glAttachShader(_floorProgram, gridVertexShader);
    glAttachShader(_floorProgram, gridFragmentShader);
    GLLinkProgram(_floorProgram);
    glUseProgram(_floorProgram);
    
    GLCheckForError();
    
    glUseProgram(0);
    
    texture = [GLKTextureLoader textureWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"chair_chesterfield_d.png" ofType: nil] options:NULL error:NULL];
    
    return YES;
}

- (void)setupVAOS
{
    const GLfloat floorVertices[] =
    {
        5.0f,  0.0f, -5.0f,
        -5.0f,  0.0f, -5.0f,
        -5.0f,  0.0f,  5.0f,
        5.0f,  0.0f, -5.0f,
        -5.0f,  0.0f,  5.0f,
        5.0f,  0.0f,  5.0f,
    };
    
    const GLfloat floorNormals[] =
    {
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
    };
    
    const GLfloat floorColors[] =
    {
        0.0f, 0.3398f, 0.9023f, 1.0f,
        0.0f, 0.3398f, 0.9023f, 1.0f,
        0.0f, 0.3398f, 0.9023f, 1.0f,
        0.0f, 0.3398f, 0.9023f, 1.0f,
        0.0f, 0.3398f, 0.9023f, 1.0f,
        0.0f, 0.3398f, 0.9023f, 1.0f,
    };
    
    const GLfloat ceilingColors[] =
    {
        0.0f, 0.6f, 0.0f, 1.0f,
        0.0f, 0.6f, 0.0f, 1.0f,
        0.0f, 0.6f, 0.0f, 1.0f,
        0.0f, 0.6f, 0.0f, 1.0f,
        0.0f, 0.6f, 0.0f, 1.0f,
        0.0f, 0.6f, 0.0f, 1.0f,
    };

    // Chair VAO setup
    glGenVertexArraysOES(1, &_chairVertexArray);
    glBindVertexArrayOES(_chairVertexArray);
    
    glEnableVertexAttribArray([chairProgram getAttribLocation:@"position"]);
    glEnableVertexAttribArray([chairProgram getAttribLocation:@"normal"]);
    glEnableVertexAttribArray([chairProgram getAttribLocation:@"texture_coordinate"]);
    
    glGenBuffers(1, &_chairVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _chairVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(chair_chesterfieldVerts), chair_chesterfieldVerts, GL_STATIC_DRAW);
    
    // Set the position of the cube
    glVertexAttribPointer([chairProgram getAttribLocation:@"position"], _coordsPerVertex, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    glGenBuffers(1, &_chairNormalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _chairNormalBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(chair_chesterfieldNormals), chair_chesterfieldNormals, GL_STATIC_DRAW);
    
    // Set the normal positions of the cube, again for shading
    glVertexAttribPointer([chairProgram getAttribLocation:@"normal"], 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    glGenBuffers(1, &_chairColorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _chairColorBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(chair_chesterfieldTexCoords), chair_chesterfieldTexCoords, GL_STATIC_DRAW);
    
    glVertexAttribPointer([chairProgram getAttribLocation:@"texture_coordinate"], 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    GLCheckForError();
    
    glBindVertexArrayOES(0);

    
    
    // Floor VAO setup
    glGenVertexArraysOES(1, &_floorVertexArray);
    glBindVertexArrayOES(_floorVertexArray);
    
    _floorModelLocation = glGetUniformLocation(_floorProgram, "u_Model");
    _floorModelViewLocation = glGetUniformLocation(_floorProgram, "u_MVMatrix");
    _floorModelViewProjectionLocation = glGetUniformLocation(_floorProgram, "u_MVP");
    _floorLightPositionLocation = glGetUniformLocation(_floorProgram, "u_LightPos");
    
    _floorPositionLocation = glGetAttribLocation(_floorProgram, "a_Position");
    _floorNormalLocation = glGetAttribLocation(_floorProgram, "a_Normal");
    _floorColorLocation = glGetAttribLocation(_floorProgram, "a_Color");
    
    glEnableVertexAttribArray(_floorPositionLocation);
    glEnableVertexAttribArray(_floorNormalLocation);
    glEnableVertexAttribArray(_floorColorLocation);
    
    glGenBuffers(1, &_floorVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _floorVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);
    
    // Set the position of the floor
    glVertexAttribPointer(_floorPositionLocation, _coordsPerVertex, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    glGenBuffers(1, &_floorNormalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _floorNormalBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorNormals), floorNormals, GL_STATIC_DRAW);
    
    // Set the normal positions of the floor, again for shading
    glVertexAttribPointer(_floorNormalLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    glGenBuffers(1, &_floorColorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _floorColorBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorColors), floorColors, GL_STATIC_DRAW);
    
    glGenBuffers(1, &_ceilingColorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _ceilingColorBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ceilingColors), ceilingColors, GL_STATIC_DRAW);

    GLCheckForError();
    
    glBindVertexArrayOES(0);
}

- (void)shutdownRendererWithView:(GLKView *)GLView
{
}

- (void)renderViewDidChangeSize:(CGSize)size
{
}

- (void)prepareNewFrameWithHeadViewMatrix:(GLKMatrix4)headViewMatrix
{
    // Build the Model part of the ModelView matrix
    //_modelCube = GLKMatrix4Rotate(_modelCube, GLKMathDegreesToRadians(_timeDelta), 0.5f, 0.5f, 1.0f);
    
    // Build the camera matrix and apply it to the ModelView.
    _camera = GLKMatrix4MakeLookAt(0, 0, _cameraZ,
                                   0, 0, 0,
                                   0, 1.0f, 0);
    
    _headView = headViewMatrix;
    
    GLCheckForError();
}

- (void)drawEyeWithEye:(EyeWrapper *)eye
{
    // DLog(@"%ld %@", eye.type, NSStringFromGLKMatrix4([eye eyeViewMatrix]));

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    GLCheckForError();
    
    // Apply the eye transformation to the camera
    _view = GLKMatrix4Multiply([eye eyeViewMatrix], _camera);
    
    // Set the position of the light
    _lightPositionInEyeSpace = GLKMatrix4MultiplyVector4(_view, _lightPositionInWorldSpace);
    
    const float zNear = 0.1f;
    const float zFar = 100.0f;
    _perspective = [eye perspectiveMatrixWithZNear:zNear zFar:zFar];
    
    [self drawFloorAndCeiling];

    [self drawChair];
}

- (void)finishFrameWithViewportRect:(CGRect)viewPort
{
}

// Draw the floor.
// This feeds in data for the floor into the shader. Note that this doesn't feed in data about
// position of the light, so if we rewrite our code to draw the floor first, the lighting might
// look strange.
- (void)drawFloorAndCeiling
{
    glUseProgram(_floorProgram);
    glBindVertexArrayOES(_floorVertexArray);

    
    // Floor
    // Set mModelView for the floor, so we draw floor in the correct location
    _modelView = GLKMatrix4Multiply(_view, _modelFloor);
    _modelViewProjection = GLKMatrix4Multiply(_perspective, _modelView);

    // Set ModelView, MVP, position, normals, and color.
    glUniform3f(_floorLightPositionLocation,
                _lightPositionInEyeSpace.x,
                _lightPositionInEyeSpace.y,
                _lightPositionInEyeSpace.z);
    glUniformMatrix4fv(_floorModelLocation, 1, GL_FALSE, _modelFloor.m);
    glUniformMatrix4fv(_floorModelViewLocation, 1, GL_FALSE, _modelView.m);
    glUniformMatrix4fv(_floorModelViewProjectionLocation, 1, GL_FALSE, _modelViewProjection.m);
    
    glBindBuffer(GL_ARRAY_BUFFER, _floorColorBuffer);
    glVertexAttribPointer(_floorColorLocation, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    GLCheckForError();

    
    // Ceiling
    _modelView = GLKMatrix4Multiply(_view, _modelCeiling);
    _modelViewProjection = GLKMatrix4Multiply(_perspective, _modelView);
    
    glUniformMatrix4fv(_floorModelViewProjectionLocation, 1, GL_FALSE, _modelViewProjection.m);

    glBindBuffer(GL_ARRAY_BUFFER, _ceilingColorBuffer);
    glVertexAttribPointer(_floorColorLocation, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    GLCheckForError();
    
    
    glBindVertexArrayOES(0);
    glUseProgram(0);
}

// Draw the cube.
// We've set all of our transformation matrices. Now we simply pass them into the shader.
- (void)drawChair
{
    // Build the ModelView and ModelViewProjection matrices
    // for calculating cube position and light.
    _modelView = GLKMatrix4Multiply(_view, _modelChair);
    _modelViewProjection = GLKMatrix4Multiply(_perspective, _modelView);
    
    glUseProgram(chairProgram.program);
    
    glBindVertexArrayOES(_chairVertexArray);
    
    
    glUniformMatrix4fv([chairProgram getUniformLocation:@"projection_matrix"], 1, false, _perspective.m);
    
    glUniformMatrix4fv([chairProgram getUniformLocation:@"camera_matrix"], 1, false, _view.m);

    glUniformMatrix4fv([chairProgram getUniformLocation:@"model_matrix"], 1, false, _modelChair.m);
    
    glUniform3f([chairProgram getUniformLocation:@"light_direction"], 0, 0, 1);
    glUniform4f([chairProgram getUniformLocation:@"light_ambient"], .8, .8, .8, 1);
    glUniform4f([chairProgram getUniformLocation:@"light_diffuse"], .8, .8, .8, 1);
    glUniform4f([chairProgram getUniformLocation:@"light_specular"], .8, .8, .8, 1);
    glUniform4f([chairProgram getUniformLocation:@"material_specular"], .1, .1, .1, 1);
    glUniform1f([chairProgram getUniformLocation:@"material_shininess"], 20.);
    
    
    GLuint tloc = [chairProgram getUniformLocation:@"texture_value"];
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(texture.target, texture.name);
    glUniform1i(tloc, 0);
    
    glDrawArrays(GL_TRIANGLES, 0, chair_chesterfieldNumVerts);
    
    GLCheckForError();
    
    glBindVertexArrayOES(0);
    glUseProgram(0);
}

- (void)magneticTriggerPressed
{
    [self toggleSensorFusion];
}

- (void)handleTap:(UITapGestureRecognizer *)sender
{
    if (sender.state == UIGestureRecognizerStateEnded)
    {
        [self toggleSensorFusion];
    }
}

- (void) toggleSensorFusion
{
    if (isSensorFusionRunning)
        [self stopSensorFusion];
    else
        [self startSensorFusion];
}

- (void)startSensorFusion
{
    if (!isSensorFusionRunning)
    {
        [self.cardboard startTracking];

        isSensorFusionRunning = YES;
    }
}

- (void)stopSensorFusion
{
    if (isSensorFusionRunning)
    {
        [self.cardboard stopTracking];
        isSensorFusionRunning = NO;
    }
}

@end


@interface TreasureViewController()

@property (nonatomic) TreasureRenderer *treasureRenderer;

@end


@implementation TreasureViewController

- (instancetype)init
{
    self = [super init];
    if (!self) {return nil; }
    
    self.treasureRenderer = [TreasureRenderer new];
    self.stereoRendererDelegate = self.treasureRenderer;
    self.treasureRenderer.cardboard = self;
    
    return self;
}

- (NSUInteger)supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskLandscapeRight;
}

- (BOOL)shouldAutorotate { return YES; }

@end
