#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "graph_nuklear.h"

#include <aem/stringbuf.h>

#include "axes.h"

void graphics_axes_new(struct graphics_axes *axes)
{
	graphics_shader_program_init(&axes->grid_shader);
	AEM_STRINGBUF_ON_STACK(path, graphics_axes_shader_path.n+16);
	aem_stringbuf_append(&path, &graphics_axes_shader_path);
	size_t i = path.n;
	aem_stringbuf_puts(&path, "shader.v.glsl");
	graphics_shader_add_file    (&axes->grid_shader, GL_VERTEX_SHADER  , aem_stringbuf_get(&path));
	path.n = i;
	aem_stringbuf_puts(&path, "grid_linear.f.glsl");
	graphics_shader_add_file    (&axes->grid_shader, GL_FRAGMENT_SHADER, aem_stringbuf_get(&path));
	aem_stringbuf_dtor(&path);
	graphics_shader_program_link(&axes->grid_shader);
	axes->uniform_origin = glGetUniformLocation(axes->grid_shader.program, "origin");
	axes->uniform_scale = glGetUniformLocation(axes->grid_shader.program, "scale");
	axes->uniform_grid_scale = glGetUniformLocation(axes->grid_shader.program, "grid_scale");
	axes->uniform_grid_intensity = glGetUniformLocation(axes->grid_shader.program, "grid_intensity");

	aem_logf_ctx(AEM_LOG_DEBUG, "axes location(origin) = %d", axes->uniform_origin);
	aem_logf_ctx(AEM_LOG_DEBUG, "axes location(scale) = %d", axes->uniform_scale);
	aem_logf_ctx(AEM_LOG_DEBUG, "axes location(grid_scale) = %d", axes->uniform_grid_scale);
	aem_logf_ctx(AEM_LOG_DEBUG, "axes location(grid_intensity) = %d", axes->uniform_grid_intensity);

	aem_assert(axes->grid_shader.status == GRAPHICS_SHADER_PROGRAM_LINKED);
}

void graphics_axes_dtor(struct graphics_axes *axes)
{
	graphics_shader_program_dtor(&axes->grid_shader);
}

void graphics_axes_recalculate(struct graphics_axes *axes)
{
	axes->xmin = axes->xmid - 0.5*axes->width*axes->dp;
	axes->xmax = axes->xmid + 0.5*axes->width*axes->dp;

	axes->ymin = axes->ymid - 0.5*axes->height*axes->dp;
	axes->ymax = axes->ymid + 0.5*axes->height*axes->dp;

	double grid_level_x = log(axes->dp) / log(axes->grid_base_x);
	axes->grid_stress_x = ceil(grid_level_x) - grid_level_x;
	axes->grid_scl_x = pow(axes->grid_base_x, ceil(grid_level_x));

	double grid_level_y = log(axes->dp) / log(axes->grid_base_y);
	axes->grid_stress_y = ceil(grid_level_y) - grid_level_y;
	axes->grid_scl_y = pow(axes->grid_base_y, ceil(grid_level_y));

	axes->grid_scale[0] = 1.0 / axes->grid_scl_x;
	axes->grid_intensity[0] = axes->grid_stress_x / (GRID_ORDERS - 1);
	float intensity_step = 1.0 / (GRID_ORDERS - 1);

#if 0
	aem_logf_ctx(AEM_LOG_DEBUG2, "grid_scl_x        = %g", axes->grid_scl_x   );
	aem_logf_ctx(AEM_LOG_DEBUG2, "grid_stress       = %g", axes->grid_stress_x);
	for (size_t i = 0; i < GRID_ORDERS; i++) {
		aem_logf_ctx(AEM_LOG_DEBUG2, "grid_scale    [%zd] = %g", i, axes->grid_scale[i]);
		aem_logf_ctx(AEM_LOG_DEBUG2, "grid_intensity[%zd] = %g", i, axes->grid_intensity[i]);
	}
#endif

	for (size_t i = 1; i < GRID_ORDERS; i++) {
		axes->grid_scale    [i] = axes->grid_scale    [i-1] / axes->grid_base_x;
		axes->grid_intensity[i] = axes->grid_intensity[i-1]  + intensity_step;
	}
}

void graphics_axes_zoom(struct graphics_axes *axes, double x, double y, double factor)
{
	axes->xmid = (axes->xmid - x) * factor + x;
	axes->ymid = (axes->ymid - y) * factor + y;
	axes->dp *= factor;
	graphics_axes_recalculate(axes);
}

static const GLfloat vertices[8] =
{
	-1.0f, -1.0f,
	 1.0f, -1.0f,
	-1.0f,  1.0f,
	 1.0f,  1.0f,
};

void graphics_axes_shader_render(struct graphics_axes *axes)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer  (2, GL_FLOAT, 2*sizeof(GLfloat), vertices);
	glTexCoordPointer(2, GL_FLOAT, 2*sizeof(GLfloat), vertices);

	graphics_check_gl_error("graphics_axes_shader_render: pre glUniform*");
	glUniform2f(axes->uniform_origin, axes->xmid, axes->ymid);
	glUniform2f(axes->uniform_scale, axes->dp*axes->width*0.5, axes->dp*axes->height*0.5);
	glUniform1fv(axes->uniform_grid_scale, GRID_ORDERS, axes->grid_scale);
	glUniform1fv(axes->uniform_grid_intensity, GRID_ORDERS, axes->grid_intensity);
	graphics_check_gl_error("graphics_axes_shader_render: post glUniform*");

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void graphics_axes_render(struct graphics_axes *axes)
{
	glUseProgram(axes->grid_shader.program);

	graphics_axes_shader_render(axes);
}
