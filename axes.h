#ifndef AXES_H
#define AXES_H

#include "aem/stringbuf.h"

#include "shader.h"

// must match value in grid_linear.f.glsl
#define GRID_ORDERS 5

extern struct aem_stringbuf graphics_axes_shader_path;

enum graphics_axes_grid_type
{
	GRID_LIN,
	GRID_LOG,
};

struct graphics_axes_axis
{
	double (*fwd)(double x);
	double (*rev)(double x); // inverse of fwd: x == fwd(rev(x)) for all x
};

struct graphics_axes
{
	// public
	double xmid;
	double ymid;
	double dp;

	int width;
	int height;

	enum graphics_axes_grid_type grid_type_x;
	enum graphics_axes_grid_type grid_type_y;

	double grid_base_x;
	double grid_base_y;

// private
	struct graphics_shader_program grid_shader;

	// updated by graphics_axes_recalculate
	double xmin;
	double xmax;
	double ymin;
	double ymax;

	double grid_stress_x;
	double grid_stress_y;

	double grid_scl_x;
	double grid_scl_y;

	float grid_scale[GRID_ORDERS];
	float grid_intensity[GRID_ORDERS];

	int uniform_origin;
	int uniform_scale;
	int uniform_grid_scale;
	int uniform_grid_intensity;
};

void graphics_axes_new(struct graphics_axes *axes);
void graphics_axes_dtor(struct graphics_axes *axes);

void graphics_axes_recalculate(struct graphics_axes *axes);
void graphics_axes_zoom(struct graphics_axes *axes, double x, double y, double factor);

void graphics_axes_shader_render(struct graphics_axes *axes);
void graphics_axes_render(struct graphics_axes *axes);

#endif /* AXES_H */
