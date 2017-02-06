#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "graphics.h"

#include "axes.h"

#include "controls.h"

#include "linked_list.h"

// misc controls

struct nk_color graphics_color_picker(struct nk_context *ctx, struct nk_color color, int *hsv)
{
	nk_layout_row_dynamic(ctx, 25, 1);
	if (nk_combo_begin_color(ctx, color, nk_vec2(nk_widget_width(ctx),400)))
	{
		nk_layout_row_dynamic(ctx, 120, 1);
		color = nk_color_picker(ctx, color, NK_RGBA);

		nk_layout_row_dynamic(ctx, 20, 2);
		if (nk_option_label(ctx, "RGB", !*hsv)) *hsv = 0;
		if (nk_option_label(ctx, "HSV",  *hsv)) *hsv = 1;

		nk_layout_row_dynamic(ctx, 20, 1);
		if (!*hsv)
		{
			color.r = (nk_byte)nk_propertyi(ctx, "#R:", 0, color.r, 255, 1, 1);
			color.r = (nk_byte)nk_slide_int(ctx,        0, color.r, 255, 1   );
			color.g = (nk_byte)nk_propertyi(ctx, "#G:", 0, color.g, 255, 1, 1);
			color.g = (nk_byte)nk_slide_int(ctx,        0, color.g, 255, 1   );
			color.b = (nk_byte)nk_propertyi(ctx, "#B:", 0, color.b, 255, 1, 1);
			color.b = (nk_byte)nk_slide_int(ctx,        0, color.b, 255, 1   );
			color.a = (nk_byte)nk_propertyi(ctx, "#A:", 0, color.a, 255, 1, 1);
			color.a = (nk_byte)nk_slide_int(ctx,        0, color.a, 255, 1   );
		}
		else
		{
			nk_float tmp[4];
			nk_color_hsva_fv(tmp, color);
			tmp[0] = nk_propertyf(ctx, "#H:", 0.0, tmp[0]*255.0, 255.0, 1.0, 1.0)/255.0;
			tmp[0] = nk_slide_float(ctx,      0.0, tmp[0]*255.0, 255.0, 1.0     )/255.0;
			tmp[1] = nk_propertyf(ctx, "#S:", 0.0, tmp[1]*255.0, 255.0, 1.0, 1.0)/255.0;
			tmp[1] = nk_slide_float(ctx,      0.0, tmp[1]*255.0, 255.0, 1.0     )/255.0;
			tmp[2] = nk_propertyf(ctx, "#V:", 0.0, tmp[2]*255.0, 255.0, 1.0, 1.0)/255.0;
			tmp[2] = nk_slide_float(ctx,      0.0, tmp[2]*255.0, 255.0, 1.0     )/255.0;
			tmp[3] = nk_propertyf(ctx, "#A:", 0.0, tmp[3]*255.0, 255.0, 1.0, 1.0)/255.0;
			tmp[3] = nk_slide_float(ctx,      0.0, tmp[3]*255.0, 255.0, 1.0     )/255.0;
			color = nk_hsva_fv(tmp);
		}

		nk_combo_end(ctx);
	}

	return color;
}

// graph parameter

struct stack graphics_graph_parameters = {0};

struct graphics_graph_parameter *graphics_graph_parameter_new(size_t count)
{
	struct graphics_graph_parameter *param = malloc(sizeof(*param));

	param->count = count;

	param->values = malloc(param->count*sizeof(float));

	for (size_t i = 0; i < param->count; i++)
	{
		param->values[i] = 0.0f;
	}

	param->refs = 0;

	param->id = graphics_graph_parameters.n;

	stack_push(&graphics_graph_parameters, param);

	return param;
}

void graphics_graph_parameter_free(struct graphics_graph_parameter *param)
{
	graphics_graph_parameters.s[param->id] = NULL;

	free(param->values);
	free(param);
}

void graphics_graph_parameter_ref(struct graphics_graph_parameter **param_p, struct graphics_graph_parameter *param)
{
	if (param != NULL)
	{
		param->refs++;
	}

	if (*param_p != NULL)
	{
		graphics_graph_parameter_unref(*param_p);
	}

	*param_p = param;
}

void graphics_graph_parameter_unref(struct graphics_graph_parameter *param)
{
	if (--param->refs > 0) return;

	graphics_graph_parameter_free(param);
}

int graphics_graph_parameter_unique(struct graphics_graph_parameter **param_p)
{
	struct graphics_graph_parameter *param = *param_p;

	if (param->refs <= 1) return 0;

	struct graphics_graph_parameter *param_new = graphics_graph_parameter_new(param->count);

	for (size_t i = 0; i < param_new->count; i++)
	{
		param_new->values[i] = param->values[i];
	}

	graphics_graph_parameter_ref(param_p, param_new);

	return 1;
}

int graphics_graph_parameter_update(struct graphics_graph_parameter *param, float dt)
{
	int animating = 0;
	for (size_t i = 0; i < param->count - 1; i++)
	{
		param->values[i] += param->values[i+1]*dt;
		if (param->values[i+1] != 0.0) animating = 1;
	}

	return animating;
}

int graphics_graph_parameter_update_all(float dt)
{
	int animating = 0;

	STACK_FOREACH(i, &graphics_graph_parameters)
	{
		struct graphics_graph_parameter *param = graphics_graph_parameters.s[i];
		if (param == NULL) continue;

		if (graphics_graph_parameter_update(param, dt))
		{
			animating = 1;
		}
	}

	return animating;
}

// graph parameter view

struct graphics_graph_parameter_view *graphics_graph_parameter_view_new_at(struct graphics_graph_parameter_view *view, const char *name, struct graphics_graph_parameter *param)
{
	view->param = NULL;
	graphics_graph_parameter_ref(&view->param, param);

	stringbuf_new_at(&view->name);
	stringbuf_puts(&view->name, name);

	return view;
}

int graphics_graph_parameter_view_dtor(struct graphics_graph_parameter_view *view)
{
	graphics_graph_parameter_unref(view->param);

	stringbuf_dtor(&view->name);

	return 0;
}

void graphics_graph_parameter_view_free(struct graphics_graph_parameter_view *view)
{
	graphics_graph_parameter_view_dtor(view);

	free(view);
}


void graphics_graph_parameter_view_draw(struct graphics_graph_parameter_view *view, struct nk_context *ctx)
{
	struct graphics_graph_parameter *param = view->param;

	size_t n = view->name.n;

	nk_layout_row_dynamic(ctx, 20, 1);
	for (size_t i = 0; i < param->count; i++)
	{
		nk_property_float(ctx, stringbuf_get(&view->name), -INFINITY, &param->values[i], INFINITY, 0.001f, 0.0001f);
		stringbuf_putc(&view->name, '\'');
		nk_slider_float(ctx, floor(param->values[i])-0.1f, &param->values[i], floor(param->values[i])+1.1, 0.0001f);
	}

	view->name.n = n;
}

// graph
static int graphics_graph_id_next = 0;

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

	graph->hsv = 0;

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
		struct graphics_graph_parameter_view *view = graph->params.s[i];
		if (view == NULL) continue;
		struct graphics_graph_parameter *param = view->param;

		graphics_graph_parameter_view_free(view);
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
		struct graphics_graph_parameter_view *view = graph->params.s[i];
		if (view == NULL) continue;
		struct graphics_graph_parameter *param = view->param;

		stringbuf_puts(&frag_shader, "uniform float ");
		stringbuf_append(&frag_shader, &view->name);
		stringbuf_puts(&frag_shader, ";\n");
	}
	stringbuf_puts(&frag_shader, GLSL(
		const float M_PI = 3.141592653589793238;

		vec2 cpx_conj(in vec2 a)
		{
			return vec2(a.x, -a.y);
		}

		vec2 cpx_mul(in vec2 a, in vec2 b)
		{
			return vec2(dot(a, cpx_conj(b)), dot(a, b.yx));
		}

		vec2 cpx_div(in vec2 a, in vec2 b)
		{
			return cpx_mul(a, cpx_conj(b)) / dot(b, b);
		}

		vec2 cpx_exp(in vec2 a)
		{
			float r = exp(a.x);
			float theta = a.y;
			return r*vec2(cos(theta), sin(theta));
		}

		vec2 cpx_log(in vec2 a)
		{
			return vec2(log(length(a)), atan(a.y, a.x));
		}

		vec3 hsv2rgb(in vec3 c)
		{
			vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
			vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
			return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
		}

		float mapinf2unit(float x)
		{
			//return 1.0 - exp(-x*0.2);
			return 1.0 - 1.0 / (1.0 + sqrt(x));
			//return (2.0/M_PI)*atan(sqrt(x));
		}

		vec4 cpx_plot(in vec2 c)
		{
			float intensity = mapinf2unit(length(c));
			return vec4(hsv2rgb(vec3(degrees(atan(c.y, c.x)) / 360.0, 1.0 - 2.0*abs(intensity - 0.5), intensity)), plot_color.a);
		}

		float eqn_f(in float f)
		{
			float sx = abs((f + dFdx(f)          ) + f);
			float sy = abs((f           + dFdy(f)) + f);
			float sb = abs((f + dFdx(f) + dFdy(f)) + f);
			float sw = abs((f + dFdx(f) + dFdy(f)) + f);
			float on = fwidth(f) * 0.5 / min(sx, sy);
			return clamp(on, 0.0, 1.0);
		}

		vec4 eqn(in float f)
		{
			return eqn_f(f)*plot_color;
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
		struct graphics_graph_parameter_view *view = graph->params.s[i];
		if (view == NULL) continue;
		struct graphics_graph_parameter *param = view->param;

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
		struct graphics_graph_parameter_view *view = graph->params.s[i];
		if (view == NULL) continue;
		struct graphics_graph_parameter *param = view->param;

		const char *name = stringbuf_get(&view->name);
		GLint position = glGetUniformLocation(graph->eqn_shader.program, name);
		view->uniform = position;
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

	glUniform4fv(2, 1, graph->color);
	STACK_FOREACH(i, &graph->params)
	{
		struct graphics_graph_parameter_view *view = graph->params.s[i];
		if (view == NULL) continue;
		struct graphics_graph_parameter *param = view->param;

		if (view->uniform != -1)
		{
			glUniform1f(view->uniform, param->values[0]);
		}
	}

	graphics_axes_shader_render(&win->axes);
}

int graphics_graph_update(struct graphics_graph *graph)
{
	if (!graph->animate) return 0;

	int animating = 0;

	STACK_FOREACH(i, &graph->params)
	{
		struct graphics_graph_parameter_view *view = graph->params.s[i];
		if (view == NULL) continue;
		struct graphics_graph_parameter *param = view->param;

		if (param->count > 0 && param->values[0] != 0.0) // TODO: not good enough: what if it's just passing by 0?
		{
			animating = 1;
		}
	}

	return animating;
}

void graphics_graph_draw(struct graphics_graph *graph, struct nk_context *ctx)
{
	char id[64];
	snprintf(id, 64, "%x", graph->id);
	if (nk_begin_titled(ctx, id, stringbuf_get(&graph->name), nk_rect(70, 70, 360, 375),
	             NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
	             NK_WINDOW_MINIMIZABLE|NK_WINDOW_CLOSABLE|NK_WINDOW_TITLE))
	{
		if (nk_tree_push(ctx, NK_TREE_NODE, "Setup", NK_MAXIMIZED))
		{
			nk_layout_row_dynamic(ctx, 20, 4);
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
				nk_layout_row_dynamic(ctx, 20, 1);
				int n = graph->name.n;
				nk_edit_string(ctx, NK_EDIT_FIELD, graph->name.s, &n, graph->name.maxn-1, nk_filter_default);
				graph->name.n = n;
			}

			{
				struct nk_color plot_color = nk_rgba_fv(graph->color);
				plot_color = graphics_color_picker(ctx, plot_color, &graph->hsv);
				nk_color_fv(graph->color, plot_color);
			}

			{
				stringbuf_reserve(&graph->eqn, graph->eqn.n);
				nk_layout_row_dynamic(ctx, 150, 1);
				int n = graph->eqn.n;
				nk_flags active = nk_edit_string(ctx, NK_EDIT_BOX, graph->eqn.s, &n, graph->eqn.maxn, nk_filter_default);
				graph->eqn.n = n;

				nk_layout_row_dynamic(ctx, 20, 1);
				if (nk_button_label(ctx, "Update") ||
				   (active & NK_EDIT_COMMITED))
				{
					graphics_graph_setup(graph);
				}
			}

			STACK_FOREACH(i, &graph->params)
			{
				struct graphics_graph_parameter_view *view = graph->params.s[i];
				if (view == NULL) continue;
				struct graphics_graph_parameter *param = view->param;

				stringbuf_reserve(&view->name, view->name.n);
				nk_layout_row_dynamic(ctx, 20, 2);
				int n = view->name.n;
				nk_edit_string(ctx, NK_EDIT_FIELD, view->name.s, &n, view->name.maxn, nk_filter_default);
				view->name.n = n;
				if (nk_button_label(ctx, "Delete"))
				{
					graph->params.s[i] = NULL;
					graphics_graph_parameter_view_free(view);
				}
			}

			nk_layout_row_dynamic(ctx, 20, 2);
			if (nk_combo_begin_label(ctx, "Reference Parameter", nk_vec2(200,200)))
			{
				STRINGBUF_ON_STACK(ref_name, 128);
				LL_FOR_ALL(win2, &graphics_window_list, prev, next)
				{
					stringbuf_reset(&ref_name);
					stringbuf_append(&ref_name, &win2->title);
					stringbuf_putc(&ref_name, '/');
					size_t n1 = ref_name.n;

					if (win2->id == -1) continue;
					LL_FOR_ALL(graph2, &win2->graph_list, prev, next)
					{
						if (graph2->id == -1) continue;

						stringbuf_append(&ref_name, &graph2->name);
						stringbuf_putc(&ref_name, '/');
						size_t n2 = ref_name.n;

						STACK_FOREACH(i, &graph2->params)
						{
							struct graphics_graph_parameter_view *view = graph2->params.s[i];
							if (view == NULL) continue;

							stringbuf_append(&ref_name, &view->name);

							nk_layout_row_dynamic(ctx, 20, 1);
							if (nk_combo_item_text(ctx, ref_name.s, ref_name.n, NK_TEXT_ALIGN_LEFT))
							{
								struct graphics_graph_parameter *param = view->param;
								stack_push(&graph->params, graphics_graph_parameter_view_new(stringbuf_get(&view->name), param));
							}

							ref_name.n = n2;
						}
						ref_name.n = n1;
					}
				}

				stringbuf_dtor(&ref_name);

				nk_combo_end(ctx);
			}
			if (nk_button_label(ctx, "Add Parameter"))
			{
				struct graphics_graph_parameter *param = graphics_graph_parameter_new(2);
				stack_push(&graph->params, graphics_graph_parameter_view_new("t", param));
			}

			nk_tree_pop(ctx);
		}

		STACK_FOREACH(i, &graph->params)
		{
			struct graphics_graph_parameter_view *view = graph->params.s[i];
			if (view == NULL) continue;

			graphics_graph_parameter_view_draw(view, ctx);
		}
	}
	nk_end(ctx);

	if (nk_window_is_closed(ctx, id))
	{
		graph->action = GRAPHICS_GRAPH_ACTION_CLOSE;
	}
}
