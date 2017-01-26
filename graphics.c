#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "graphics.h"

#include "axes.h"

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

#define NK_MEMSET memset
#define NK_MEMCPY memcpy

#define NK_GLFW_GL2_IMPLEMENTATION
#include "nuklear/nuklear.h"
#include "nuklear_glfw_gl2.h"

static void gr_error_callback(int error, const char *description)
{
	fputs(description, stderr);
}

int graphics_init(void)
{
	glfwSetErrorCallback(gr_error_callback);

	if (!glfwInit())
	{
		glfwTerminate();
		fprintf(stderr, "glfwInit failed\n");
		return -1;
	}

	return 0;
}

int graphics_quit(void)
{
	glfwTerminate();

	return 0;
}

struct graphics_window *graphics_window_curr = NULL;

int graphics_window_init(struct graphics_window *win)
{
	struct nk_context *ctx = nk_glfw3_new(&win->nk);

	win->nk.userdata = nk_handle_ptr(win);

	struct nk_font_atlas *atlas;
	nk_glfw3_font_stash_begin(&win->nk, &atlas);
	// load fonts here
	nk_glfw3_font_stash_end(&win->nk);

	return 0;
}

int graphics_window_dtor(struct graphics_window *win)
{
	graphics_window_select(win);

	nk_glfw3_destroy(&win->nk);

	return 0;
}

int graphics_window_render(struct graphics_window *win)
{
	graphics_window_select(win);

	nk_glfw3_new_frame(&win->nk);

	graphics_check_gl_error("pre gl setup");
	float bg[4];
	nk_color_fv(bg, win->background);
	glViewport(0, 0, win->nk.display_width, win->nk.display_height);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(bg[0], bg[1], bg[2], bg[3]);

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

	static int count = 4;
	static struct nk_color plot_color = {.r = 255, .g = 255, .b = 255, .a = 255};

	static float time_old = 0.0;
	float time_new = glfwGetTime();
	float dt = time_new - time_old;
	for (size_t i = 0; i < win->graph_params_n; i++)
	{
		graphics_graph_parameter_update(&win->graph_params[i], dt);
	}
	time_old = time_new;

	graphics_check_gl_error("pre glUseProgram");
	glUseProgram(win->shader_program);
	graphics_check_gl_error("post glUseProgram");

	float plot_color_rgba[4];
	nk_color_fv(plot_color_rgba, plot_color);
	glUniform4fv(2, 1, plot_color_rgba);
	glUniform4f(3, win->graph_params[0].values[0], win->graph_params[1].values[0], win->graph_params[2].values[0], win->graph_params[3].values[0]);
	graphics_axes_shader_render(&win->axes);

	graphics_axes_render(&win->axes);

	glUseProgram(0);

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	struct nk_context *ctx = &win->nk.ctx;

	if (nk_begin(ctx, "Settings", nk_rect(50, 50, 230, 250),
	             NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
	             NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
	{
		nk_layout_row_static(ctx, 30, 80, 1);
		if (nk_button_label(ctx, "Quit"))
		{
			fprintf(stdout, "quit button pressed\n");
			glfwSetWindowShouldClose(win->nk.win, 1);
		}

		static int grid_base = 10;
		nk_layout_row_dynamic(ctx, 25, 1);
		nk_property_int(ctx, "Grid Base:", 2, &grid_base, 100, 1, 1);
		nk_slider_int(ctx, 2, &grid_base, 100, 1);
		if (win->axes.grid_base_x != grid_base)
		{
			win->axes.grid_base_x = grid_base;
			win->axes.grid_base_y = grid_base;
			graphics_axes_recalculate(&win->axes);
		}

		/*
		nk_layout_row_dynamic(ctx, 25, 1);
		nk_property_int(ctx, "Count:", 3, &count, 50, 1, 1);
		nk_slider_int(ctx, 3, &count, 50, 1);
		*/

		for (size_t i = 0; i < win->graph_params_n; i++)
		{
			graphics_graph_parameter_draw(&win->graph_params[i], ctx);
		}

		if (nk_tree_push(ctx, NK_TREE_TAB, "Colors", NK_MINIMIZED))
		{
			nk_layout_row_dynamic(ctx, 20, 1);
			nk_label(ctx, "plot color:", NK_TEXT_LEFT);
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

			nk_layout_row_dynamic(ctx, 20, 1);
			nk_label(ctx, "background:", NK_TEXT_LEFT);
			nk_layout_row_dynamic(ctx, 25, 1);
			if (nk_combo_begin_color(ctx, win->background, nk_vec2(nk_widget_width(ctx),400)))
			{
				nk_layout_row_dynamic(ctx, 120, 1);
				win->background = nk_color_picker(ctx, win->background, NK_RGBA);
				nk_layout_row_dynamic(ctx, 25, 1);
				win->background.r = (nk_byte)nk_propertyi(ctx, "#R:", 0, win->background.r, 255, 1,1);
				win->background.g = (nk_byte)nk_propertyi(ctx, "#G:", 0, win->background.g, 255, 1,1);
				win->background.b = (nk_byte)nk_propertyi(ctx, "#B:", 0, win->background.b, 255, 1,1);
				win->background.a = (nk_byte)nk_propertyi(ctx, "#A:", 0, win->background.a, 255, 1,1);
				nk_combo_end(ctx);
			}
			nk_tree_pop(ctx);
		}
	}
	nk_end(ctx);


	/* IMPORTANT: `nk_glfw_render` modifies some global OpenGL state
	 * with blending, scissor, face culling and depth test and defaults everything
	 * back into a default state. Make sure to either save and restore or
	 * reset your own state after drawing rendering the UI. */
	graphics_check_gl_error("pre nk");
	nk_glfw3_render(&win->nk, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
	graphics_check_gl_error("post nk");
	glfwSwapBuffers(win->nk.win);
}
