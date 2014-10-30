attribute vec4 position;
attribute mediump vec4 texture_coordinate;
varying mediump vec2 coordinate;

void main()
{
	gl_Position = position;
	coordinate = textureCoordinate.xy;
}
