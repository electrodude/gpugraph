#ifndef CONTROLS_H
#define CONTROLS_H

#include "stack.h"
#include "stringbuf.h"

#include "shader.h"

struct nk_context;      // from nuklear.h
struct graphics_window; // from graphics.h

struct graphics_graph_parameter
{
	size_t count;
	float *values;
	GLint uniform;

	struct stringbuf name;
};

#define graphics_graph_parameter_new(name, count, value) (graphics_graph_parameter_new_at(malloc(sizeof(struct graphics_graph_parameter)), name, count, value))
struct graphics_graph_parameter *graphics_graph_parameter_new_at(struct graphics_graph_parameter *param, const char *name, size_t count, float value);
void graphics_graph_parameter_dtor(struct graphics_graph_parameter *param);
void graphics_graph_parameter_free(struct graphics_graph_parameter *param);

void graphics_graph_parameter_update(struct graphics_graph_parameter *param, float dt);
void graphics_graph_parameter_draw(struct graphics_graph_parameter *param, struct nk_context *ctx);

struct graphics_graph
{
	struct stack params;

	struct graphics_shader_program eqn_shader;
#if 0
	GLuint param_uniform;
#endif

	struct stringbuf eqn;
	struct stringbuf name;

	size_t id;

	float color[4];

	int valid : 1;
};

struct nk_context;

#define graphics_graph_new() (graphics_graph_new_at(malloc(sizeof(struct graphics_graph))))
struct graphics_graph *graphics_graph_new_at(struct graphics_graph *graph);
void graphics_graph_dtor(struct graphics_graph *graph);
void graphics_graph_free(struct graphics_graph *graph);

void graphics_graph_setup(struct graphics_graph *graph);
void graphics_graph_render(struct graphics_graph *graph, struct graphics_window *win);
void graphics_graph_update(struct graphics_graph *graph, float dt);
void graphics_graph_draw(struct graphics_graph *graph, struct nk_context *ctx);

#endif /* CONTROLS_H */
