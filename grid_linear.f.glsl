#version 120

const int GRID_ORDERS = 5;

uniform vec2 origin;
uniform vec2 scale;
uniform float grid_scale[GRID_ORDERS];
uniform float grid_intensity[GRID_ORDERS];

vec3 hsv2rgb(vec3 c)
{
	vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
	return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

float gridcomponent(vec2 pos, float size)
{
	pos = pos * size;
	vec2 grid = floor(fract(pos) + fwidth(pos));
	return max(grid.x, grid.y);
}

void main(void)
{
	vec2 pos = gl_TexCoord[0].st;

	float line = 0.0;
	for (int i = 0; i < GRID_ORDERS; i++)
	{
		line = max(line, grid_intensity[i] * gridcomponent(pos, grid_scale[i]));
	}

	gl_FragColor = vec4(line);
	//gl_FragColor = vec4(vec3(sqrt(line)),line);
	//gl_FragColor = vec4(1.0, 1.0, 1.0, line);
	//gl_FragColor = vec4(hsv2rgb(vec3(line, 1.0, 0.5)), line);
}
