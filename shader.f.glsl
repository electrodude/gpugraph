#version 120

const int GRID_ORDERS = 5;

uniform vec2 origin;
uniform vec2 scale;
uniform float grid_scale[GRID_ORDERS];
uniform float grid_intensity[GRID_ORDERS];

uniform vec4 plot_color;
uniform vec4 t;

const float M_PI = 3.141592653589793238;

vec3 hsv2rgb(vec3 c)
{
	vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
	return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

float func(in vec2 coord)
{
	float x = coord.x;
	float y = coord.y;

	return (x*x*sin(y+x)) - (-2*y*sin(x)+y+5*cos(x*y+t));
}

void main(void)
{
	vec2 pos = gl_TexCoord[0].st;

#if 1
	float f = func(pos);
	float sx = abs((f + dFdx(f)          ) + f);
	float sy = abs((f           + dFdy(f)) + f);
	float sb = abs((f + dFdx(f) + dFdy(f)) + f);
	float sw = abs((f + dFdx(f) + dFdy(f)) + f);
	float on = fwidth(f) * 0.5 / min(sx, sy);
	gl_FragColor = on*plot_color;
#endif
#if 0
	// Mandelbrot set
	gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);

	vec2 z = vec2(0.0, 0.0);
	vec2 c = pos;

	for (int i = 0; i < 100; i++)
	{
		z = vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + c;
		if (dot(z, z) > 4.0)
		{
			gl_FragColor = vec4(hsv2rgb(vec3(float(i)/20.0, 1.0, 0.5)), 1.0);
			c = vec2(0.0, 0.0);
			z = vec2(0.0, 0.0);
		}
	}

#endif
}
