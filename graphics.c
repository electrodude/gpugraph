#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

#define NK_MEMSET memset
#define NK_MEMCPY memcpy

#define NK_GLFW_GL2_IMPLEMENTATION
#include "graph_nuklear.h"

#include "aem/linked_list.h"

#include "controls.h"

#include "session_load.h"
#include "session_save.h"

#include "axes.h"

#include "graphics.h"

static void gr_error_callback(int error, const char *description)
{
	fprintf(stderr, "%d: %s\n", error, description);
}

struct graphics_window *graphics_window_curr = NULL;
struct graphics_window graphics_window_list;

int graphics_init(void)
{
	glfwSetErrorCallback(gr_error_callback);

	if (!glfwInit()) {
		glfwTerminate();
		fprintf(stderr, "glfwInit failed\n");
		return -1;
	}

	AEM_LL_INIT(&graphics_window_list, prev, next);
	graphics_window_list.id = -1;

	return 0;
}

int graphics_quit(void)
{
	graphics_window_curr = NULL;

	glfwTerminate();

	return 0;
}

static int win_id_next = 0;

int graphics_window_init(struct graphics_window *win, const char *title)
{
	struct nk_context *ctx = nk_glfw3_new(&win->nk, title);

	win->nk.userdata = nk_handle_ptr(win);

	aem_stringbuf_init_cstr(&win->title, title);

	aem_stringbuf_init(&win->eqn_pfx);

	struct nk_font_atlas *atlas;
	nk_glfw3_font_stash_begin(&win->nk, &atlas);
	// load fonts here
	nk_glfw3_font_stash_end(&win->nk);

	AEM_LL_INIT(&win->graph_list, prev, next);
	win->graph_list.id = -1;

	win->id = win_id_next++;

	AEM_LL_INSERT_BEFORE(&graphics_window_list, win, prev, next);
	AEM_LL_VERIFY(&graphics_window_list, prev, next, assert);

	return 0;
}

int graphics_window_dtor(struct graphics_window *win)
{
	struct graphics_graph *prev = NULL;
	AEM_LL_FOR_ALL(graph, &win->graph_list, prev, next) {
		if (prev)
			graphics_graph_free(prev);
		prev = graph;
	}
	if (prev)
		graphics_graph_free(prev);

	graphics_window_select(win);

	nk_glfw3_destroy(&win->nk);

	aem_stringbuf_dtor(&win->eqn_pfx);
	aem_stringbuf_dtor(&win->title);

	return 0;
}

void graphics_window_update_title(struct graphics_window *win)
{
	glfwSetWindowTitle(win->nk.win, aem_stringbuf_get(&win->title));
}

int graphics_window_draw(struct graphics_window *win)
{
	struct nk_context *ctx = &win->nk.ctx;

	int animating = 0;

	AEM_LL_FOR_ALL(graph, &win->graph_list, prev, next) {
		graphics_graph_draw(graph, win);
		if (graphics_graph_update(graph))
			animating = 1;
	}

	struct graphics_graph graph_freelist;
	AEM_LL_INIT(&graph_freelist, prev, next);
	AEM_LL_FOR_ALL(graph, &win->graph_list, prev, next) {
		enum graphics_graph_action action = graph->action;
		graph->action = GRAPHICS_GRAPH_ACTION_NONE;
		switch (action) {
			case GRAPHICS_GRAPH_ACTION_MOVE_UP:
				if (graph->next == &win->graph_list)
					break; // already at top; do nothing
				AEM_LL_REMOVE(graph, prev, next); // temporarily remove node from list
				AEM_LL_VERIFY(&win->graph_list, prev, next, assert);
				AEM_LL_INSERT_BEFORE(graph->next->next, graph, prev, next); // and reinsert it below the one below this one
				AEM_LL_VERIFY(&win->graph_list, prev, next, assert);
				graph = graph->prev->prev; // go back to get the one we moved before this one without processing
				// this one gets processed again after the previous one, but it's fine
				break;

			case GRAPHICS_GRAPH_ACTION_MOVE_DOWN:
				if (graph->prev == &win->graph_list)
					break; // already at bottom; do nothing
				AEM_LL_REMOVE(graph, prev, next); // temporarily remove node from list
				AEM_LL_VERIFY(&win->graph_list, prev, next, assert);
				AEM_LL_INSERT_BEFORE(graph->prev, graph, prev, next); // and reinsert it above the one above this one
				AEM_LL_VERIFY(&win->graph_list, prev, next, assert);
				// repeats the one that used to come before this one, but it's fine
				break;

			case GRAPHICS_GRAPH_ACTION_CLOSE:
				AEM_LL_REMOVE(graph, prev, next);
				AEM_LL_VERIFY(&win->graph_list, prev, next, assert);
				struct graphics_graph *graph_prev = graph->prev;
				AEM_LL_INSERT_BEFORE(&graph_freelist, graph, prev, next);
				graph = graph_prev;
				AEM_LL_VERIFY(&graph_freelist, prev, next, assert);
				break;
		}
	}

	while (graph_freelist.next != &graph_freelist)
		AEM_LL_REMOVE(graph_freelist.next, prev, next);

	if (nk_begin(ctx, "Window Settings", nk_rect(0, 0, 230, 240),
	             NK_WINDOW_BORDER|NK_WINDOW_SCALABLE|
	             NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
	{
		if (nk_tree_push(ctx, NK_TREE_NODE, "Session", NK_MINIMIZED)) {
			{
				nk_layout_row_dynamic(ctx, 15, 1);
				nk_label(ctx, "Path", NK_TEXT_LEFT);
				nk_layout_row_dynamic(ctx, 25, 1);
				aem_stringbuf_reserve(&session_path, win->title.n);
				int n = session_path.n;
				int updated = nk_edit_string(ctx, NK_EDIT_FIELD, session_path.s, &n, session_path.maxn-1, nk_filter_default);
				session_path.n = n;
			}
			nk_layout_row_dynamic(ctx, 20, 2);
			if (nk_button_label(ctx, "Save")) {
				session_save_curr();
			}
			if (nk_button_label(ctx, "Load")) {
				//graphics_reset();
				AEM_LL_FOR_ALL(win, &graphics_window_list, prev, next) {
					glfwSetWindowShouldClose(win->nk.win, 1);
				}

				session_load_curr();
				graphics_window_select(win);
			}
			nk_layout_row_dynamic(ctx, 20, 2);
			if (nk_button_label(ctx, "New")) {
				//graphics_reset();
				AEM_LL_FOR_ALL(win, &graphics_window_list, prev, next) {
					glfwSetWindowShouldClose(win->nk.win, 1);
				}

				aem_stringbuf_reset(&session_path);
				struct graphics_window *graph_window_new(void); // from main.c
				graph_window_new();
				graphics_window_select(win);
			}
			if (nk_button_label(ctx, "Quit")) {
				fprintf(stderr, "NYI\n");
			}

			nk_tree_pop(ctx);
		}
		if (nk_tree_push(ctx, NK_TREE_NODE, "Window Settings", NK_MAXIMIZED)) {
			nk_layout_row_dynamic(ctx, 20, 2);
			if (nk_button_label(ctx, "New Graph")) {
				struct graphics_graph *graph = graphics_graph_new(win);
				AEM_LL_INSERT_BEFORE(&win->graph_list, graph, prev, next);
				AEM_LL_VERIFY(&win->graph_list, prev, next, assert);
			}
			if (nk_button_label(ctx, "New Window")) {
				struct graphics_window *graph_window_new(void); // from main.c
				struct graphics_window *win2 = graph_window_new();
			}

			{
				nk_layout_row_dynamic(ctx, 15, 1);
				nk_label(ctx, "Window Title:", NK_TEXT_LEFT);
				nk_layout_row_dynamic(ctx, 25, 1);
				aem_stringbuf_reserve(&win->title, win->title.n);
				int n = win->title.n;
				int updated = nk_edit_string(ctx, NK_EDIT_FIELD, win->title.s, &n, win->title.maxn-1, nk_filter_default);
				win->title.n = n;

				if (updated)
					graphics_window_update_title(win);
			}

			nk_layout_row_dynamic(ctx, 15, 1);
			nk_label(ctx, "Window Prefix:", NK_TEXT_LEFT);
			nk_layout_row_dynamic(ctx, 25, 1);
			aem_stringbuf_reserve(&win->eqn_pfx, win->eqn_pfx.n);
			int n = win->eqn_pfx.n;
			nk_edit_string(ctx, NK_EDIT_FIELD, win->eqn_pfx.s, &n, win->eqn_pfx.maxn-1, nk_filter_default);
			win->eqn_pfx.n = n;

			nk_layout_row_dynamic(ctx, 20, 2);
			if (nk_button_label(ctx, "Update Pfx")) {
				AEM_LL_FOR_ALL(graph, &win->graph_list, prev, next) {
					graphics_graph_setup(graph);
				}

			}

			static int grid_base = 10;
			int size = win->nk.width > win->nk.height ? win->nk.width : win->nk.height;
			int gridbase_max = ceil(sqrt(size));
			nk_checkbox_label(ctx, "Grid Enable", &win->grid_en);
			nk_layout_row_dynamic(ctx, 20, 1);
			nk_property_int(ctx, "Grid Base:", 2, &grid_base, gridbase_max, 1, 1);
			nk_layout_row_dynamic(ctx, 20, 1);
			nk_slider_int(ctx, 2, &grid_base, gridbase_max, 1);
			if (win->axes.grid_base_x != grid_base) {
				win->axes.grid_base_x = grid_base;
				win->axes.grid_base_y = grid_base;
				graphics_axes_recalculate(&win->axes);
			}

			nk_layout_row_dynamic(ctx, 15, 1);
			nk_label(ctx, "Background Color:", NK_TEXT_LEFT);
			win->background = graphics_color_picker(ctx, win->background, &win->hsv);

			nk_tree_pop(ctx);
		}

		AEM_LL_FOR_ALL(graph, &win->graph_list, prev, next) {
			graphics_graph_draw_dock(graph, ctx);
		}
	}
	nk_end(ctx);

	if (nk_begin(ctx, "Parameters", nk_rect(win->nk.display_width - 230, 0, 230, 240),
	             NK_WINDOW_BORDER|NK_WINDOW_SCALABLE|
	             NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
	{
		struct nk_rect bounds = nk_window_get_bounds(ctx);
		if (ctx->input.mouse.buttons[0].clicked)
			bounds.x = win->nk.display_width - bounds.w;

		bounds = graphics_util_nk_rect_check(bounds, nk_vec2(win->nk.display_width, win->nk.display_height));
		nk_window_set_bounds(ctx, bounds);

		if (nk_tree_push(ctx, NK_TREE_NODE, "Parameter Settings", NK_MINIMIZED)) {
			AEM_LL_FOR_ALL(param, &graphics_graph_parameters, prev, next) {
				graphics_graph_parameter_draw_settings(param, ctx);
			}

			nk_tree_pop(ctx);
		}

		AEM_LL_FOR_ALL(param, &graphics_graph_parameters, prev, next) {
			graphics_graph_parameter_draw(param, &param->name, ctx);
		}
	}
	nk_end(ctx);

	return animating;
}

void graphics_window_render(struct graphics_window *win)
{
	//graphics_window_select(win);

	nk_glfw3_new_frame(&win->nk);

	graphics_check_gl_error("pre gl setup");
	float bg[4];
	nk_color_fv(bg, win->background);
	glViewport(0, 0, win->nk.display_width, win->nk.display_height);
	glClearColor(bg[0], bg[1], bg[2], bg[3]);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	graphics_check_gl_error("gl projection");
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	float aspect_ratio = win->nk.display_width / (float)win->nk.display_height;
	glOrtho(0.0f, aspect_ratio, 1.0f, 0.0f, -1.0f, 1.0f);
	graphics_check_gl_error("gl modelview");

	AEM_LL_FOR_ALL(graph, &win->graph_list, prev, next) {
		graphics_graph_render(graph, win);
	}

	if (win->grid_en)
		graphics_axes_render(&win->axes);

	glUseProgram(0);

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();


	/* IMPORTANT: `nk_glfw_render` modifies some global OpenGL state
	 * with blending, scissor, face culling and depth test and defaults everything
	 * back into a default state. Make sure to either save and restore or
	 * reset your own state after drawing rendering the UI. */
	graphics_check_gl_error("pre nk");
	nk_glfw3_render(&win->nk, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
	graphics_check_gl_error("post nk");
	glfwSwapBuffers(win->nk.win);
}
