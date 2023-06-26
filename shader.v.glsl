#version 130

const int GRID_ORDERS = 5;

uniform vec2 origin;
uniform vec2 scale;
uniform float grid_scale[GRID_ORDERS];
uniform float grid_intensity[GRID_ORDERS];

attribute vec2 screencoord;

void main(void)
{
	gl_Position = vec4(screencoord, 0.0, 1.0);

	vec2 finalcoord = screencoord * scale + origin;
	gl_TexCoord[0] = vec4(finalcoord, 0.0, 1.0);;
}
