uniform mat4 camera_matrix;
uniform mat4 projection_matrix;
uniform vec4 measurement;

attribute vec4 position;
attribute vec4 sourcecolor;
attribute float textureCoordinate;
attribute float perpindicular;

varying vec4 destinationcolor;
varying highp float horizcoord;
varying highp float vertcoord;

void main(void) {
    destinationcolor = sourcecolor;

    vec4 firstcam = camera_matrix * vec4(0., 0., 0., 1.);
    vec4 secondcam = camera_matrix * measurement;
    vec4 cammeas = secondcam - firstcam;
    vec4 perp = normalize(vec4(cammeas.y, -cammeas.x, 0., 0.));

    vec4 campos = camera_matrix * position;
    gl_Position = projection_matrix * (campos + perp * perpindicular);
    horizcoord = textureCoordinate;
    vertcoord = perpindicular;
}
