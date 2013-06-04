uniform mat4 camera_matrix;
uniform mat4 projection_matrix;

attribute vec4 position;
attribute vec4 sourcecolor;

varying vec4 destinationcolor;

void main(void) {
    destinationcolor = sourcecolor;
    gl_Position = projection_matrix * (camera_matrix * position);
}
