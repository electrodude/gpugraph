#ifndef CONTROLS_H
#define CONTROLS_H

#include <aem/stringbuf.h>
#include <aem/stringslice.h>

#include "axes.h"
#include "shader.h"

#include "graph_nuklear.h"

struct nk_context;      // from nuklear.h
struct nk_color;        // from nuklear.h
struct graphics_window; // from graphics.h

struct nk_color graphics_color_picker(struct nk_context *ctx, struct nk_color color, int *hsv);

struct nk_rect graphics_util_nk_rect_check(struct nk_rect bounds, struct nk_vec2 size);


struct graphics_graph_parameter
{
	struct aem_stringbuf name;

	size_t count;
	float *values;

	size_t refs;

	struct graphics_graph_parameter *param_prev;
	struct graphics_graph_parameter *param_next;
	int id;

	int enable;
	int animate;
};

extern struct graphics_graph_parameter graphics_graph_parameters;
struct graphics_graph_parameter *graphics_graph_parameter_lookup(struct aem_stringslice name);

struct graphics_graph_parameter *graphics_graph_parameter_new(size_t count);
void graphics_graph_parameter_ref(struct graphics_graph_parameter **param_p, struct graphics_graph_parameter *param);
void graphics_graph_parameter_unref(struct graphics_graph_parameter *param);

//void graphics_graph_parameter_set_count(struct graphics_graph_parameter *param, size_t count);

void graphics_graph_parameter_draw(struct graphics_graph_parameter *param, struct aem_stringbuf *name, struct nk_context *ctx);
void graphics_graph_parameter_draw_settings(struct graphics_graph_parameter *param, struct nk_context *ctx);

int graphics_graph_parameter_update_all(float dt);

struct graphics_graph_parameter_view
{
	struct aem_stringbuf name;

	struct graphics_graph_parameter *param;
	GLint uniform;

	struct graphics_graph_parameter_view *param_view_prev;
	struct graphics_graph_parameter_view *param_view_next;
};

struct graphics_graph_parameter_view *graphics_graph_parameter_view_new(struct aem_stringslice name, struct graphics_graph_parameter *param);
void graphics_graph_parameter_view_free(struct graphics_graph_parameter_view *view);

void graphics_graph_parameter_view_draw(struct graphics_graph_parameter_view *view, struct nk_context *ctx);

enum graphics_graph_action
{
	GRAPHICS_GRAPH_ACTION_NONE = 0,
	GRAPHICS_GRAPH_ACTION_CLOSE,
	GRAPHICS_GRAPH_ACTION_MOVE_UP,
	GRAPHICS_GRAPH_ACTION_MOVE_DOWN,
};

struct graphics_graph
{
	struct graphics_window *win;

	struct graphics_graph_parameter_view params;

	struct graphics_shader_program eqn_shader;

	struct graphics_axes_uniforms uniforms;
	int uniform_plot_color;
#if 0
	GLuint param_uniform;
#endif

	struct aem_stringbuf eqn;
	struct aem_stringbuf name;

	struct graphics_graph *graph_prev;
	struct graphics_graph *graph_next;
	int id;

	float color[4];

	enum graphics_graph_action action;

	int enable;
	int dock;
	int hsv;
};

struct nk_context;

struct graphics_graph *graphics_graph_new(struct graphics_window *win);
void graphics_graph_free(struct graphics_graph *graph);

void graphics_graph_setup(struct graphics_graph *graph);
void graphics_graph_render(struct graphics_graph *graph, struct graphics_window *win);
int graphics_graph_update(struct graphics_graph *graph);
void graphics_graph_draw(struct graphics_graph *graph, struct graphics_window *win);
void graphics_graph_draw_dock(struct graphics_graph *graph, struct nk_context *ctx);

#endif /* CONTROLS_H */
