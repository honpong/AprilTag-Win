precision mediump float;
varying vec4 v_Color;
varying mediump vec2 coordinate;

uniform sampler2D texture_value;

void main() {
    gl_FragColor = texture2D(texture_value, coordinate) + v_Color;
}
