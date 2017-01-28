#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "graphics.h"

#include "axes.h"

#include "controls.h"

// graph parameter

struct graphics_graph_parameter *graphics_graph_parameter_new_at(struct graphics_graph_parameter *param, const char *name, size_t count, float value)
{
	param->count = count;

	param->values = malloc(param->count*sizeof(float));

	param->values[0] = value;
	for (size_t i = 1; i < param->count; i++)
	{
		param->values[i] = 0.0f;
	}

	stringbuf_new_at(&param->name);
	stringbuf_puts(&param->name, name);

	return param;
}

void graphics_graph_parameter_dtor(struct graphics_graph_parameter *param)
{
	if (param->values != NULL)
	{
		free(param->values);
		param->values = NULL;
	}

	param->count = 0;

	stringbuf_dtor(&param->name);
}

void graphics_graph_parameter_free(struct graphics_graph_parameter *param)
{
	graphics_graph_parameter_dtor(param);

	free(param);
}


void graphics_graph_parameter_update(struct graphics_graph_parameter *param, float dt)
{
	for (size_t i = 0; i < param->count - 1; i++)
	{
		param->values[i] += param->values[i+1]*dt;
	}
}

void graphics_graph_parameter_draw(struct graphics_graph_parameter *param, struct nk_context *ctx)
{
	size_t n = param->name.n;

	for (size_t i = 0; i < param->count; i++)
	{
		nk_layout_row_dynamic(ctx, 25, 1);
		nk_property_float(ctx, stringbuf_get(&param->name), -INFINITY, &param->values[i], INFINITY, 0.001f, 0.0001f);
		stringbuf_putc(&param->name, '\'');
		nk_slider_float(ctx, floor(param->values[i])-0.1f, &param->values[i], floor(param->values[i])+1.1, 0.0001f);
	}

	param->name.n = n;
}

// graph
static size_t graphics_graph_id_next = 0;

struct graphics_graph *graphics_graph_new_at(struct graphics_graph *graph)
{
	stack_new_at(&graph->params);

	graphics_shader_program_new(&graph->eqn_shader);

	stringbuf_new_prealloc_at(&graph->eqn, 512);
	stringbuf_new_prealloc_at(&graph->name, 32);

	graph->prev = NULL;
	graph->next = NULL;

	graph->id = graphics_graph_id_next++;

	graph->enable = 1;
	graph->animate = 1;

	graph->action = GRAPHICS_GRAPH_ACTION_NONE;


	stringbuf_puts(&graph->name, "Graph ");
	stringbuf_putnum(&graph->name, 10, graph->id);

	stringbuf_puts(&graph->eqn, "float x = pos.x;\nfloat y = pos.y;\ngl_FragColor = eqn(x*x + y*y - 1.0);\n"); // circle of radius 1.0

	graphics_graph_setup(graph);

	return graph;
}

void graphics_graph_dtor(struct graphics_graph *graph)
{
	LL_REMOVE(graph, prev, next);

	STACK_FOREACH(i, &graph->params)
	{
		struct graphics_graph_parameter *param = graph->params.s[i];
		if (param == NULL) continue;

		graphics_graph_parameter_free(param);
	}
	graphics_shader_program_dtor(&graph->eqn_shader);
	stack_dtor(&graph->params);
	stringbuf_dtor(&graph->name);
	stringbuf_dtor(&graph->eqn);
}

void graphics_graph_free(struct graphics_graph *graph)
{
	graphics_graph_dtor(graph);

	free(graph);
}


#define GLSL(code) #code
void graphics_graph_setup(struct graphics_graph *graph)
{
	struct stringbuf frag_shader;
	stringbuf_new_prealloc_at(&frag_shader, 4096);
	stringbuf_puts(&frag_shader, "#version 120\n" GLSL(
		const int GRID_ORDERS = 5;

		uniform vec2 origin;
		uniform vec2 scale;
		uniform float grid_scale[GRID_ORDERS];
		uniform float grid_intensity[GRID_ORDERS];

		uniform vec4 plot_color;
	));
	stringbuf_putc(&frag_shader, '\n');
	STACK_FOREACH(i, &graph->params)
	{
		struct graphics_graph_parameter *param = graph->params.s[i];
		if (param == NULL) continue;

		stringbuf_puts(&frag_shader, "uniform float ");
		stringbuf_append(&frag_shader, &param->name);
		stringbuf_puts(&frag_shader, ";\n");
	}
	stringbuf_puts(&frag_shader, GLSL(
		const float M_PI = 3.141592653589793238;

		vec3 hsv2rgb(vec3 c)
		{
			vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
			vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
			return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
		}

		vec4 eqn(in float f)
		{
			float sx = abs((f + dFdx(f)          ) + f);
			float sy = abs((f           + dFdy(f)) + f);
			float sb = abs((f + dFdx(f) + dFdy(f)) + f);
			float sw = abs((f + dFdx(f) + dFdy(f)) + f);
			float on = fwidth(f) * 0.5 / min(sx, sy);
			return clamp(on, 0.0, 1.0)*plot_color;
		}

		void main()
		{
			vec2 pos = gl_TexCoord[0].st;
	));
	stringbuf_putc(&frag_shader, '\n');
	stringbuf_append(&frag_shader, &graph->eqn);
	stringbuf_puts(&frag_shader, GLSL(
		}
	));

	puts("new graph fragment shader:");
	puts(stringbuf_get(&frag_shader));

	graphics_shader_program_dtor(&graph->eqn_shader);
	graphics_shader_program_new (&graph->eqn_shader);
	graphics_shader_add_file    (&graph->eqn_shader, GL_VERTEX_SHADER  , "shader.v.glsl");
	graphics_shader_add         (&graph->eqn_shader, GL_FRAGMENT_SHADER, stringslice_new_str(frag_shader));
	stringbuf_dtor(&frag_shader);
	graphics_shader_program_link(&graph->eqn_shader);
	if (graph->eqn_shader.status != GRAPHICS_SHADER_PROGRAM_LINKED) return;

#if 0
	STACK_FOREACH(i, &graph->params)
	{
		struct graphics_graph_parameter *param = graph->params.s[i];
		if (param == NULL) continue;

		graphics_graph_parameter_free(param);
	}
	graph->params.n = 0;
#endif

	GLint max_len;
	glGetProgramiv(graph->eqn_shader.program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_len);
	char *name = malloc(max_len);

	GLuint n_uniforms;
	glGetProgramiv(graph->eqn_shader.program, GL_ACTIVE_UNIFORMS, &n_uniforms);
	for (GLuint i = 0; i < n_uniforms; i++)
	{
		GLint size;
		GLenum type;
		GLsizei n;
		glGetActiveUniform(graph->eqn_shader.program, i, max_len, &n, &size, &type, name);

		printf("uniform %d: type %u, size %u: %s\n", i, type, size, name);

#if 0
		stack_push(&graph->params, graphics_graph_parameter_new(name, 2, 0.0));
#endif
	}
	free(name);

	STACK_FOREACH(i, &graph->params)
	{
		struct graphics_graph_parameter *param = graph->params.s[i];
		if (param == NULL) continue;

		const char *name = stringbuf_get(&param->name);
		GLint position = glGetUniformLocation(graph->eqn_shader.program, name);
		param->uniform = position;
		printf("%s = %d\n", name, position);
	}

#if 0
	graph->param_uniform = 3;
#endif
}

void graphics_graph_render(struct graphics_graph *graph, struct graphics_window *win)
{
	if (!graph->enable) return;
	if (graph->eqn_shader.status != GRAPHICS_SHADER_PROGRAM_LINKED) return;

	glUseProgram(graph->eqn_shader.program);

#if 0
	float u[4] = {0};
	for (size_t i = 0, j = graph->param_uniform; i < graph->params.n; j++)
	{
		for (size_t k = 0; k < 1 && i < graph->params.n; i++, k++)
		{
			struct graphics_graph_parameter *param = graph->params.s[i++];
			u[k] = param->values[0];
		}
		printf("glUniform4fv(%zd, 1, {%f, %f, %f, %f})\n", j, u[0], u[1], u[2], u[3]);
		glUniform4fv(j, 1, u);
	}
#else
	glUniform4fv(2, 1, graph->color);
	STACK_FOREACH(i, &graph->params)
	{
		struct graphics_graph_parameter *param = graph->params.s[i];
		if (param == NULL) continue;

		if (param->uniform != -1)
		{
			glUniform1f(param->uniform, param->values[0]);
		}
	}
#endif

	graphics_axes_shader_render(&win->axes);
}

void graphics_graph_update(struct graphics_graph *graph, float dt)
{
	if (!graph->animate) return;

	STACK_FOREACH(i, &graph->params)
	{
		struct graphics_graph_parameter *param = graph->params.s[i];
		if (param == NULL) continue;

		graphics_graph_parameter_update(param, dt);
	}
}

void graphics_graph_draw(struct graphics_graph *graph, struct nk_context *ctx)
{
	char id[64];
	snprintf(id, 64, "%zx", graph->id);
	if (nk_begin_titled(ctx, id, stringbuf_get(&graph->name), nk_rect(70, 70, 360, 375),
	             NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
	             NK_WINDOW_MINIMIZABLE|NK_WINDOW_CLOSABLE|NK_WINDOW_TITLE))
	{
		if (nk_tree_push(ctx, NK_TREE_NODE, "Setup", NK_MAXIMIZED))
		{
			nk_layout_row_dynamic(ctx, 25, 4);
			nk_checkbox_label(ctx, "Enable", &graph->enable);
			nk_checkbox_label(ctx, "Animate", &graph->animate);
			if (nk_button_label(ctx, "Up"))
			{
				graph->action = GRAPHICS_GRAPH_ACTION_MOVE_UP;
			}
			if (nk_button_label(ctx, "Down"))
			{
				graph->action = GRAPHICS_GRAPH_ACTION_MOVE_DOWN;
			}

			{
				stringbuf_reserve(&graph->name, graph->name.n);
				nk_layout_row_dynamic(ctx, 25, 1);
				int n = graph->name.n;
				nk_edit_string(ctx, NK_EDIT_FIELD, graph->name.s, &n, graph->name.maxn-1, nk_filter_default);
				graph->name.n = n;
			}

			{
				struct nk_color plot_color = nk_rgba_fv(graph->color);
				nk_layout_row_dynamic(ctx, 25, 1);
				if (nk_combo_begin_color(ctx, plot_color, nk_vec2(nk_widget_width(ctx),400)))
				{
					nk_layout_row_dynamic(ctx, 120, 1);
					plot_color = nk_color_picker(ctx, plot_color, NK_RGBA);
					nk_layout_row_dynamic(ctx, 25, 1);
					plot_color.r = (nk_byte)nk_propertyi(ctx, "#R:", 0, plot_color.r, 255, 1,1);
					plot_color.g = (nk_byte)nk_propertyi(ctx, "#G:", 0, plot_color.g, 255, 1,1);
					plot_color.b = (nk_byte)nk_propertyi(ctx, "#B:", 0, plot_color.b, 255, 1,1);
					plot_color.a = (nk_byte)nk_propertyi(ctx, "#A:", 0, plot_color.a, 255, 1,1);
					nk_combo_end(ctx);
				}
				nk_color_fv(graph->color, plot_color);
			}

			{
				stringbuf_reserve(&graph->eqn, graph->eqn.n);
				nk_layout_row_dynamic(ctx, 150, 1);
				int n = graph->eqn.n;
				nk_flags active = nk_edit_string(ctx, NK_EDIT_BOX, graph->eqn.s, &n, graph->eqn.maxn, nk_filter_default);
				graph->eqn.n = n;

				nk_layout_row_dynamic(ctx, 25, 1);
				if (nk_button_label(ctx, "Update") ||
				   (active & NK_EDIT_COMMITED))
				{
					graphics_graph_setup(graph);
				}
			}

			STACK_FOREACH(i, &graph->params)
			{
				struct graphics_graph_parameter *param = graph->params.s[i];
				if (param == NULL) continue;

				stringbuf_reserve(&param->name, param->name.n);
				nk_layout_row_dynamic(ctx, 25, 2);
				int n = param->name.n;
				nk_edit_string(ctx, NK_EDIT_FIELD, param->name.s, &n, param->name.maxn, nk_filter_default);
				param->name.n = n;
				if (nk_button_label(ctx, "Update"))
				{
					if (param->name.n == 0)
					{
						graphics_graph_parameter_free(param);
						graph->params.s[i] = NULL;
					}
				}
			}

			nk_layout_row_dynamic(ctx, 25, 1);
			if (nk_button_label(ctx, "New Param"))
			{
				stack_push(&graph->params, graphics_graph_parameter_new("t", 2, 0.0));
			}

			nk_tree_pop(ctx);
		}

		STACK_FOREACH(i, &graph->params)
		{
			struct graphics_graph_parameter *param = graph->params.s[i];
			if (param == NULL) continue;

			graphics_graph_parameter_draw(param, ctx);
		}
	}
	nk_end(ctx);

	if (nk_window_is_closed(ctx, id))
	{
		graph->action = GRAPHICS_GRAPH_ACTION_CLOSE;
	}
}
