//
//  ViewController.m
//  RC3DKMobileVis
//
//  Created by Brian on 2/27/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

#import "ViewController.h"
#import "WorldState.h"

#define INITIAL_LIMITS 3.
#define POINT_SIZE 3.0
//#define PERSPECTIVE

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

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

@interface ViewController () {
    float xMin, xMax, yMin, yMax;
    float currentTime;
    RCViewpoint currentViewpoint;
    RCFeatureFilter featuresFilter;
    float currentScale;
    WorldState * state;

    GLuint _program;

    GLKMatrix4 _modelViewProjectionMatrix;
    GLKMatrix3 _normalMatrix;
    float _rotation;

    GLuint _vertexArray;
    GLuint _vertexBuffer;
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
    [self buildGridVertexData];
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
        if (f.lastSeen == renderTime)
            setColor(&featureVertex[idx], 255, 0, 0, 255);
        else {
            if (featuresFilter == RCFeatureFilterShowGood && !f.good)
                continue;
            setColor(&featureVertex[idx], 255, 255, 255, 255);
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
            setColor(&pathVertex[idx], 0, 0, 255, 255);
        setPosition(&featureVertex[idx], t.x, t.y, t.z);
        idx++;
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

    _rotation += self.timeSinceLastUpdate * 0.5f;

    GLKMatrix4 baseModelViewMatrix = GLKMatrix4MakeTranslation(0.0f, 0.0f, -4.0f);
    baseModelViewMatrix = GLKMatrix4Rotate(baseModelViewMatrix, _rotation, 0.0f, 1.0f, 0.0f);

    // Compute the model view matrix for the object rendered with GLKit
    GLKMatrix4 modelViewMatrix = GLKMatrix4MakeTranslation(0.0f, 0.0f, -1.5f);
    modelViewMatrix = GLKMatrix4Rotate(modelViewMatrix, _rotation, 1.0f, 1.0f, 1.0f);
    modelViewMatrix = GLKMatrix4Multiply(baseModelViewMatrix, modelViewMatrix);
    _modelViewProjectionMatrix = GLKMatrix4Multiply(projectionMatrix, modelViewMatrix);

    self.effect.transform.modelviewMatrix = modelViewMatrix;
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    glClearColor(0.65f, 0.65f, 0.65f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render the object with GLKit
    [self.effect prepareToDraw];

    glUseProgram(_program);

    glUniformMatrix4fv(uniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, _modelViewProjectionMatrix.m);
    //glUniformMatrix4fv(uniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, modelViewMatrix);

    glUniformMatrix3fv(uniforms[UNIFORM_NORMAL_MATRIX], 1, 0, _normalMatrix.m);

    DrawModel();
}

void DrawModel()
{
    //NSLog(@"Draw model with %d", ngrid);

    glVertexAttribPointer(GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), &gridVertex[0].position);
    glEnableVertexAttribArray(GLKVertexAttribPosition);
    glVertexAttribPointer(GLKVertexAttribColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexData), &gridVertex[0].color);
    glEnableVertexAttribArray(GLKVertexAttribColor);

    glLineWidth(4.0f);
    glDrawArrays(GL_LINES, 0, ngrid);

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

@end
