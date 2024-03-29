#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>

#include <aem/linked_list.h>

#include "graphics.h"

#include "axes.h"

#include "controls.h"

// misc controls

struct nk_color graphics_color_picker(struct nk_context *ctx, struct nk_color color, int *hsv)
{
	nk_layout_row_dynamic(ctx, 25, 1);
	if (nk_combo_begin_color(ctx, color, nk_vec2(nk_widget_width(ctx),400))) {
		nk_layout_row_dynamic(ctx, 120, 1);
		color = nk_color_picker(ctx, color, NK_RGBA);

		nk_layout_row_dynamic(ctx, 20, 2);
		if (nk_option_label(ctx, "RGB", !*hsv))
			*hsv = 0;
		if (nk_option_label(ctx, "HSV",  *hsv))
			*hsv = 1;

		nk_layout_row_dynamic(ctx, 20, 1);
		if (!*hsv) {
			color.r = (nk_byte)nk_propertyi(ctx, "#R:", 0, color.r, 255, 1, 1);
			color.r = (nk_byte)nk_slide_int(ctx,        0, color.r, 255, 1   );
			color.g = (nk_byte)nk_propertyi(ctx, "#G:", 0, color.g, 255, 1, 1);
			color.g = (nk_byte)nk_slide_int(ctx,        0, color.g, 255, 1   );
			color.b = (nk_byte)nk_propertyi(ctx, "#B:", 0, color.b, 255, 1, 1);
			color.b = (nk_byte)nk_slide_int(ctx,        0, color.b, 255, 1   );
			color.a = (nk_byte)nk_propertyi(ctx, "#A:", 0, color.a, 255, 1, 1);
			color.a = (nk_byte)nk_slide_int(ctx,        0, color.a, 255, 1   );
		} else {
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

struct nk_rect graphics_util_nk_rect_check(struct nk_rect bounds, struct nk_vec2 size)
{
	int ox = 30;
	int oy = 30;
	if (bounds.x > size.x - ox)
		bounds.x = size.x - ox;
	if (bounds.y > size.y - oy)
		bounds.y = size.y - oy;

	if (bounds.x < ox - bounds.w)
		bounds.x = ox - bounds.w;
	if (bounds.y < oy - bounds.h)
		bounds.y = oy - bounds.h;

	return bounds;
}

// graph parameter

struct graphics_graph_parameter graphics_graph_parameters;
static int param_id_next = 0;

struct graphics_graph_parameter *graphics_graph_parameter_lookup(struct aem_stringslice name)
{
	AEM_LL2_FOR_ALL(param, &graphics_graph_parameters, param) {
		if (!aem_stringslice_cmp(aem_stringslice_new_str(&param->name), name))
			return param;
	}

	return NULL;
}

struct graphics_graph_parameter *graphics_graph_parameter_new(size_t count)
{
	struct graphics_graph_parameter *param = malloc(sizeof(*param));
	aem_assert(param);

	aem_stringbuf_init(&param->name);

	param->count = count;

	param->values = malloc(param->count*sizeof(float));
	aem_assert(param->values);

	for (size_t i = 0; i < param->count; i++)
		param->values[i] = 0.0f;

	param->refs = 0;

	param->id = param_id_next++;

	aem_stringbuf_putc(&param->name, 't');
	aem_stringbuf_putnum(&param->name, 10, param->id);

	param->enable = 1;
	param->animate = 1;

	AEM_LL2_INSERT_BEFORE(&graphics_graph_parameters, param, param);

	aem_logf_ctx(AEM_LOG_DEBUG, "param %p: new", param);

	return param;
}

void graphics_graph_parameter_free(struct graphics_graph_parameter *param)
{
	AEM_LL2_REMOVE(param, param);

	aem_logf_ctx(AEM_LOG_DEBUG, "param %p: free", param);

	free(param->values);

	aem_stringbuf_dtor(&param->name);

	free(param);
}

void graphics_graph_parameter_ref(struct graphics_graph_parameter **param_p, struct graphics_graph_parameter *param)
{
	if (param) {
		aem_logf_ctx(AEM_LOG_DEBUG, "param %p: %zd -> %zd refs", param, param->refs, param->refs+1);
		param->refs++;
	}

	if (*param_p)
		graphics_graph_parameter_unref(*param_p);

	*param_p = param;
}

void graphics_graph_parameter_unref(struct graphics_graph_parameter *param)
{
	aem_logf_ctx(AEM_LOG_DEBUG, "param %p: %zd -> %zd refs", param, param->refs, param->refs-1);

	if (--param->refs > 0)
		return;

	graphics_graph_parameter_free(param);
}

int graphics_graph_parameter_unique(struct graphics_graph_parameter **param_p)
{
	struct graphics_graph_parameter *param = *param_p;

	if (param->refs <= 1)
		return 0;

	struct graphics_graph_parameter *param_new = graphics_graph_parameter_new(param->count);

	for (size_t i = 0; i < param_new->count; i++)
		param_new->values[i] = param->values[i];

	graphics_graph_parameter_ref(param_p, param_new);

	return 1;
}

/*
void graphics_graph_parameter_set_count(struct graphics_graph_parameter *param, size_t count)
{
	param->values = realloc(param->values, count * sizeof(float));

	for (size_t i = param->count + 1; i < count; i++)
		param->values[i] = 0.0;

	param->count = count;
}
*/

void graphics_graph_parameter_draw(struct graphics_graph_parameter *param, struct aem_stringbuf *name, struct nk_context *ctx)
{
	if (!param->enable)
		return;

	size_t n = name->n;

	nk_layout_row_dynamic(ctx, 20, 1);
	for (size_t i = 0; i < param->count; i++) {
		nk_property_float(ctx, aem_stringbuf_get(name), -INFINITY, &param->values[i], INFINITY, 0.001f, 0.000001f);
		aem_stringbuf_putc(name, '\'');
		nk_slider_float(ctx, roundf(param->values[i])-0.6f, &param->values[i], roundf(param->values[i])+0.6f, 0.0000001f);

		if (!param->animate)
			break;
	}

	name->n = n;
}

void graphics_graph_parameter_draw_settings(struct graphics_graph_parameter *param, struct nk_context *ctx)
{
	nk_layout_row_dynamic(ctx, 20, 2);
	nk_checkbox_label(ctx, "Enable", &param->enable);
	nk_checkbox_label(ctx, "Animate", &param->animate);
}

int graphics_graph_parameter_update(struct graphics_graph_parameter *param, float dt)
{
	if (!param->animate)
		return 0;

	int animating = 0;
	for (size_t i = 0; i < param->count - 1; i++) {
		param->values[i] += param->values[i+1]*dt;
		if (param->values[i+1] != 0.0) {
			animating = 1;
		}
	}

	return animating;
}

int graphics_graph_parameter_update_all(float dt)
{
	int animating = 0;

	AEM_LL2_FOR_ALL(param, &graphics_graph_parameters, param) {
		if (graphics_graph_parameter_update(param, dt)) {
			animating = 1;
		}
	}

	return animating;
}

// graph parameter view

struct graphics_graph_parameter_view *graphics_graph_parameter_view_new(struct aem_stringslice name, struct graphics_graph_parameter *param)
{
	struct graphics_graph_parameter_view *view = malloc(sizeof(*view));
	aem_assert(view);

	view->param = NULL;
	graphics_graph_parameter_ref(&view->param, param);

	aem_stringbuf_init(&view->name);
	aem_stringbuf_putss(&view->name, name);

	return view;
}

void graphics_graph_parameter_view_free(struct graphics_graph_parameter_view *view)
{
	AEM_LL2_REMOVE(view, param_view);

	graphics_graph_parameter_unref(view->param);

	aem_stringbuf_dtor(&view->name);

	free(view);
}


void graphics_graph_parameter_view_draw(struct graphics_graph_parameter_view *view, struct nk_context *ctx)
{
	graphics_graph_parameter_draw(view->param, &view->name, ctx);
}

// graph
static int graphics_graph_id_next = 0;

struct graphics_graph *graphics_graph_new(struct graphics_window *win)
{
	struct graphics_graph *graph = malloc(sizeof(*graph));
	aem_assert(graph);

	AEM_LL2_INIT(&graph->params, param_view);

	graph->win = win;
	aem_assert(graph->win);
	graphics_window_select(graph->win);

	graphics_shader_program_init(&graph->eqn_shader);

	aem_stringbuf_init_prealloc(&graph->eqn, 512);
	aem_stringbuf_init_prealloc(&graph->name, 32);

	AEM_LL2_INIT(graph, graph);

	graph->id = graphics_graph_id_next++;

	graph->enable = 1;
	graph->dock = 0;

	graph->hsv = 0;

	graph->color[0] = 1.0f;
	graph->color[1] = 1.0f;
	graph->color[2] = 1.0f;
	graph->color[3] = 1.0f;

	graph->action = GRAPHICS_GRAPH_ACTION_NONE;

	aem_stringbuf_puts(&graph->name, "Graph ");
	aem_stringbuf_putnum(&graph->name, 10, graph->id);

	aem_stringbuf_puts(&graph->eqn, "float x = pos.x;\nfloat y = pos.y;\ngl_FragColor = eqn(x*x + y*y - 1.0);\n"); // circle of radius 1.0

	graphics_graph_setup(graph);

	return graph;
}

void graphics_graph_free(struct graphics_graph *graph)
{
	AEM_LL_WHILE_FIRST(view, &graph->params, param_view_next) {
		graphics_graph_parameter_view_free(view);
	}

	AEM_LL2_REMOVE(graph, graph);

	graphics_shader_program_dtor(&graph->eqn_shader);
	aem_stringbuf_dtor(&graph->name);
	aem_stringbuf_dtor(&graph->eqn);

	free(graph);
}

#define GLSL(code) #code
void graphics_graph_setup(struct graphics_graph *graph)
{
	aem_assert(graph);

	AEM_LOG_MULTI(out, AEM_LOG_DEBUG2) {
		aem_stringbuf_puts(out, "new function for ");
		aem_stringbuf_append(out, &graph->name);
		aem_stringbuf_puts(out, ":\n");
		aem_stringbuf_append(out, &graph->win->eqn_pfx);
		aem_stringbuf_puts(out, "\n");
		aem_stringbuf_append(out, &graph->eqn);
	}

	struct aem_stringbuf frag_shader;
	aem_stringbuf_init_prealloc(&frag_shader, 4096);
	aem_stringbuf_puts(&frag_shader, "#version 130\n" GLSL(
		const int GRID_ORDERS = 5;

		uniform vec2 origin;
		uniform vec2 scale;
		uniform float grid_scale[GRID_ORDERS];
		uniform float grid_intensity[GRID_ORDERS];

		uniform vec4 plot_color;
	));
	aem_stringbuf_putc(&frag_shader, '\n');
	AEM_LL2_FOR_ALL(view, &graph->params, param_view) {
		struct graphics_graph_parameter *param = view->param;

		aem_stringbuf_puts(&frag_shader, "uniform float ");
		aem_stringbuf_append(&frag_shader, &view->name);
		aem_stringbuf_puts(&frag_shader, ";\n");
	}
	aem_stringbuf_puts(&frag_shader, GLSL(
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

		vec2 mapinf2unit(vec2 x)
		{
			//return 1.0 - exp(-x*0.2);
			return 1.0 - 1.0 / (1.0 + sqrt(x));
			//return (2.0/M_PI)*atan(x);
		}

		vec4 cpx_plot(in vec2 c)
		{
			float intensity = mapinf2unit(length(c));
			return vec4(hsv2rgb(vec3(degrees(atan(c.y, c.x)) / 360.0, 1.0 - 2.0*abs(intensity - 0.5), intensity)), plot_color.a);
		}

		float eqn_f(in float f, in float r)
		{
			float on = fwidth(f) / abs(f) * r;

			return clamp(on, 0.0, 1.0);
		}

		vec4 eqn(in float f)
		{
			return eqn_f(f, 1.0)*plot_color;
		}

		vec4 eqn_cpx_w(in vec2 a, in float w)
		{
			float p = mapinf2unit( max(a.y, 0.0));
			float n = mapinf2unit(-min(a.y, 0.0));
			float r = n;
			float g = 1.0 - max(p, n);
			float b = p;
			return eqn_f(a.x, w)*plot_color*vec4(r, g, b , 1.0);
		}

		vec4 eqn_cpx(in vec2 a)
		{
			return eqn_cpx_w(a, 0.5);
		}

		vec4 root_locus(in vec2 GH)
		{
			//float angle = cpx_exp(0.5*cpx_log(-GH)).y;
			float angle = fract(atan(GH.y, -GH.x)/(M_PI*2.0) + 0.5) - 0.5;
			float r = length(GH);
			return eqn_cpx_w(vec2(angle, log(r)), 1.5);
		}

		vec4 level4(vec4 z)
		{
			int base = 10;
			vec4 scl10 = log(fwidth(z))/log(base);
			ivec4 i0 = ivec4(floor(scl10)) + ivec4(3); // TODO: what is 3?
			vec4 stress = fract(scl10);
			ivec4 stress_max = ivec4(base);
			for (int i = 2; i >= 0; i--) {
				vec4 p = pow(vec4(base), i-i0);
				vec4 v = fract(z*p) - fwidth(z)*p;
				if (v.x < 0.0)
					stress_max.x = i;
				if (v.y < 0.0)
					stress_max.y = i;
				if (v.z < 0.0)
					stress_max.z = i;
				if (v.w < 0.0)
					stress_max.w = i;
			}

			return 1.0 - (stress_max+stress)/3.0;
		}

		float level(float z) { return level4(vec4(z, 0.0, 0.0, 0.0)).x  ; }
		vec2  level2(vec2 z) { return level4(vec4(z, 0.0, 0.0     )).xy ; }
		vec3  level3(vec3 z) { return level4(vec4(z, 0.0          )).xyz; }

		vec4 grid1(float z)
		{
			float c = level(z);
			return vec4(plot_color.rgb, plot_color.a*c);
		}

		vec4 grid(vec2 z)
		{
			vec2 c = level2(z);
			return vec4(c/max(c.x, c.y), 0.0, max(c.x, c.y));
		}

		vec4 grid3(vec3 z)
		{
			vec3 c = level3(z);
			float cmax = max(max(c.x, c.y), c.z);
			return vec4(c/cmax, cmax);
		}

		void main()
		{
			vec2 pos = gl_TexCoord[0].st;
	));
	aem_stringbuf_putc(&frag_shader, '\n');
	aem_stringbuf_append(&frag_shader, &graph->win->eqn_pfx);
	aem_stringbuf_append(&frag_shader, &graph->eqn);
	aem_stringbuf_puts(&frag_shader, GLSL(
		}
	));

	AEM_LOG_MULTI(out, AEM_LOG_DEBUG2) {
		aem_stringbuf_puts(out, "new graph fragment shader:\n");
		aem_stringbuf_append(out, &frag_shader);
	}

	graphics_window_select(graph->win);
	graphics_shader_program_dtor(&graph->eqn_shader);
	graphics_shader_program_init(&graph->eqn_shader);
	AEM_STRINGBUF_ON_STACK(path, graphics_axes_shader_path.n+16);
	aem_stringbuf_append(&path, &graphics_axes_shader_path);
	size_t i = path.n;
	aem_stringbuf_puts(&path, "shader.v.glsl");
	graphics_shader_add_file    (&graph->eqn_shader, GL_VERTEX_SHADER  , aem_stringbuf_get(&path));
	aem_stringbuf_dtor(&path);
	graphics_shader_add         (&graph->eqn_shader, GL_FRAGMENT_SHADER, aem_stringslice_new_str(&frag_shader));
	aem_stringbuf_dtor(&frag_shader);
	graphics_shader_program_link(&graph->eqn_shader);
	if (graph->eqn_shader.status != GRAPHICS_SHADER_PROGRAM_LINKED)
		return;

#if 0
	AEM_LL2_FOR_ALL(view, &graph->params, param_view) {
		struct graphics_graph_parameter *param = view->param;

		graphics_graph_parameter_free(param);
	}
	graph->params.n = 0;
#endif

	graphics_axes_uniforms_setup(&graph->uniforms, &graph->eqn_shader);

	graph->uniform_plot_color = glGetUniformLocation(graph->eqn_shader.program, "plot_color");
	aem_logf_ctx(graph->uniform_plot_color != -1 ? AEM_LOG_DEBUG2 : AEM_LOG_ERROR, "plot_color = %d", graph->uniform_plot_color);

	GLint max_len;
	glGetProgramiv(graph->eqn_shader.program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_len);

	struct aem_stringbuf name = {0};
	aem_stringbuf_reserve(&name, max_len);
	// TODO: Why is this signed?
	GLint n_uniforms;
	glGetProgramiv(graph->eqn_shader.program, GL_ACTIVE_UNIFORMS, &n_uniforms);
	for (GLint i = 0; i < n_uniforms; i++) {
		GLint size;
		GLenum type;
		GLsizei n;
		glGetActiveUniform(graph->eqn_shader.program, i, name.maxn, &n, &size, &type, name.s);
		name.n = n;

		aem_logf_ctx(AEM_LOG_DEBUG2, "uniform %d: type %u, size %u: %s", i, type, size, name.s);

	}
	aem_stringbuf_dtor(&name);

	AEM_LL2_FOR_ALL(view, &graph->params, param_view) {
		struct graphics_graph_parameter *param = view->param;

		const char *name = aem_stringbuf_get(&view->name);
		GLint position = glGetUniformLocation(graph->eqn_shader.program, name);
		view->uniform = position;
		aem_logf_ctx(AEM_LOG_DEBUG2, "%s = %d", name, position);
	}

#if 0
	graph->param_uniform = 3;
#endif
}

void graphics_graph_render(struct graphics_graph *graph, struct graphics_window *win)
{
	if (!graph->enable)
		return;
	if (graph->eqn_shader.status != GRAPHICS_SHADER_PROGRAM_LINKED)
		return;

	glUseProgram(graph->eqn_shader.program);

	glUniform4fv(graph->uniform_plot_color, 1, graph->color);
	AEM_LL2_FOR_ALL(view, &graph->params, param_view) {
		struct graphics_graph_parameter *param = view->param;

		if (view->uniform != -1)
			glUniform1f(view->uniform, param->values[0]);
	}

	graphics_axes_shader_render(&win->axes, &graph->uniforms);
}

int graphics_graph_update(struct graphics_graph *graph)
{
	int animating = 0;

	AEM_LL2_FOR_ALL(view, &graph->params, param_view) {
		struct graphics_graph_parameter *param = view->param;

		if (param->count > 0 && param->values[0] != 0.0) { // TODO: not good enough: what if it's just passing through precisely 0?
			animating = 1;
		}
	}

	return animating;
}

void graphics_graph_draw(struct graphics_graph *graph, struct graphics_window *win)
{
	struct nk_context *ctx = &win->nk.ctx;
	if (graph->dock)
		return;

	char id[64];
	snprintf(id, 64, "%x", graph->id);
	if (nk_begin_titled(ctx, id, aem_stringbuf_get(&graph->name), nk_rect(70, 70, 360, 430),
	             NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
	             NK_WINDOW_MINIMIZABLE|NK_WINDOW_CLOSABLE|NK_WINDOW_TITLE))
	{
		struct nk_rect bounds = nk_window_get_bounds(ctx);
		bounds = graphics_util_nk_rect_check(bounds, nk_vec2(win->nk.display_width, win->nk.display_height));
		nk_window_set_bounds(ctx, bounds);

		if (nk_tree_push(ctx, NK_TREE_NODE, "Setup", NK_MAXIMIZED)) {
			float ratio_buttons[4] = {0.25f, 0.25f, 0.25f, 0.25f};
			nk_layout_row(ctx, NK_DYNAMIC, 20, 4, ratio_buttons);
			nk_checkbox_label(ctx, "Enable", &graph->enable);
			if (nk_button_label(ctx, "Up"))
				graph->action = GRAPHICS_GRAPH_ACTION_MOVE_UP;
			if (nk_button_label(ctx, "Down"))
				graph->action = GRAPHICS_GRAPH_ACTION_MOVE_DOWN;
			if (nk_button_label(ctx, "Delete"))
				graph->action = GRAPHICS_GRAPH_ACTION_CLOSE;

			{
				aem_stringbuf_reserve(&graph->name, graph->name.n);
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
				aem_stringbuf_reserve(&graph->eqn, graph->eqn.n);
				nk_layout_row_dynamic(ctx, 200, 1);
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

			AEM_LL2_FOR_ALL(view, &graph->params, param_view) {
				struct graphics_graph_parameter *param = view->param;

				aem_stringbuf_reserve(&view->name, view->name.n);
				nk_layout_row_dynamic(ctx, 20, 2);
				int n = view->name.n;
				nk_edit_string(ctx, NK_EDIT_FIELD, view->name.s, &n, view->name.maxn, nk_filter_default);
				view->name.n = n;
				if (view->param->refs == 1) {
					aem_stringbuf_reset(&view->param->name);
					aem_stringbuf_append(&view->param->name, &view->name);
				}
				if (nk_button_label(ctx, "Delete"))
					graphics_graph_parameter_view_free(view);
			}

			nk_layout_row_dynamic(ctx, 20, 2);
			if (nk_combo_begin_label(ctx, "Reference Parameter", nk_vec2(150,200))) {
				AEM_LL2_FOR_ALL(param, &graphics_graph_parameters, param) {
					nk_layout_row_dynamic(ctx, 20, 1);
					if (nk_combo_item_text(ctx, param->name.s, param->name.n, NK_TEXT_ALIGN_LEFT)) {
						struct graphics_graph_parameter_view *view2 = graphics_graph_parameter_view_new(aem_stringslice_new_str(&param->name), param);
						AEM_LL2_INSERT_BEFORE(&graph->params, view2, param_view);
					}
				}

				nk_combo_end(ctx);
			}

			if (nk_button_label(ctx, "Add Parameter")) {
				struct graphics_graph_parameter *param = graphics_graph_parameter_new(2);
				struct graphics_graph_parameter_view *view = graphics_graph_parameter_view_new(aem_stringslice_new_str(&param->name), param);
				AEM_LL2_INSERT_BEFORE(&graph->params, view, param_view);
			}

			nk_tree_pop(ctx);
		}

		if (nk_tree_push(ctx, NK_TREE_NODE, "Parameters", NK_MINIMIZED)) {
			AEM_LL2_FOR_ALL(view, &graph->params, param_view) {
				graphics_graph_parameter_view_draw(view, ctx);
			}

			nk_tree_pop(ctx);
		}
	}
	nk_end(ctx);

	if (nk_window_is_closed(ctx, id))
		graph->dock = 1;
}

void graphics_graph_draw_dock(struct graphics_graph *graph, struct nk_context *ctx)
{
#if 0
	if (nk_tree_push_id(ctx, NK_TREE_NODE, aem_stringbuf_get(&graph->name), NK_MINIMIZED, graph->id)) {
		nk_checkbox_label(ctx, "Docked", &graph->dock);

		nk_tree_pop(ctx);
	}
#else
	nk_layout_row_dynamic(ctx, 20, 2);
	int undock = !graph->dock;
	nk_checkbox_label(ctx, aem_stringbuf_get(&graph->name), &undock);
	graph->dock = !undock;
	nk_checkbox_label(ctx, "Enable", &graph->enable);
#endif
}
