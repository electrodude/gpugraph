#ifndef CONTROLS_H
#define CONTROLS_H

#include "stack.h"
#include "stringbuf.h"

#include "shader.h"

struct nk_context;      // from nuklear.h
struct nk_color;        // from nuklear.h
struct graphics_window; // from graphics.h

// is this OK? struct nk_color isn't defined yet
struct nk_color graphics_color_picker(struct nk_context *ctx, struct nk_color color, int *hsv);

struct graphics_graph_parameter
{
	size_t count;
	float *values;

	int id;
	size_t refs;
};

extern struct stack graphics_graph_parameters;

struct graphics_graph_parameter *graphics_graph_parameter_new(size_t count);
void graphics_graph_parameter_ref(struct graphics_graph_parameter **param_p, struct graphics_graph_parameter *param);
void graphics_graph_parameter_unref(struct graphics_graph_parameter *param);

int graphics_graph_parameter_update_all(float dt);

struct graphics_graph_parameter_view
{
	struct graphics_graph_parameter *param;
	GLint uniform;

	struct stringbuf name;
};

#define graphics_graph_parameter_view_new(name, param) (graphics_graph_parameter_view_new_at(malloc(sizeof(struct graphics_graph_parameter_view)), name, param))
struct graphics_graph_parameter_view *graphics_graph_parameter_view_new_at(struct graphics_graph_parameter_view *view, const char *name, struct graphics_graph_parameter *param);
int graphics_graph_parameter_view_dtor(struct graphics_graph_parameter_view *view);
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
	struct stack params;

	struct graphics_shader_program eqn_shader;
#if 0
	GLuint param_uniform;
#endif

	struct stringbuf eqn;
	struct stringbuf name;

	struct graphics_graph *prev;
	struct graphics_graph *next;
	int id;

	float color[4];

	int enable;
	int animate;

	int hsv;

	enum graphics_graph_action action;
};

struct nk_context;

#define graphics_graph_new() (graphics_graph_new_at(malloc(sizeof(struct graphics_graph))))
struct graphics_graph *graphics_graph_new_at(struct graphics_graph *graph);
void graphics_graph_dtor(struct graphics_graph *graph);
void graphics_graph_free(struct graphics_graph *graph);

void graphics_graph_setup(struct graphics_graph *graph);
void graphics_graph_render(struct graphics_graph *graph, struct graphics_window *win);
int graphics_graph_update(struct graphics_graph *graph);
void graphics_graph_draw(struct graphics_graph *graph, struct nk_context *ctx);

#endif /* CONTROLS_H */
