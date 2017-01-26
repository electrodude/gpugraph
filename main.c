#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define GL_GLEXT_PROTOTYPES
#define GLFW_INCLUDE_GLU
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>

#include "graphics.h"

#include "shader.h"

#include "axes.h"

int buttons[GLFW_MOUSE_BUTTON_LAST];
double mx, my;

static void key_callback(struct nk_glfw *nk_win, int key, int scancode, int action, int mods) {}
static void char_callback(struct nk_glfw *nk_win, unsigned int codepoint) {}
static void mousebutton_callback(struct nk_glfw *nk_win, int button, int action, int mods)
{
	if (action != GLFW_RELEASE && nk_item_is_any_active(&nk_win->ctx)) return;
	struct graphics_window *win = nk_win->userdata.ptr;
	//printf("mouse button: %d, %d\n", button, action);
	buttons[button] = action;
}
static void cursorpos_callback(struct nk_glfw *nk_win, double x, double y)
{
	struct graphics_window *win = nk_win->userdata.ptr;
	if (buttons[GLFW_MOUSE_BUTTON_LEFT])
	{
		//printf("(%g, %g), %g\n", win->axes.xmid, win->axes.ymid, win->axes.dp);
		win->axes.xmid -= (x - mx) * win->axes.dp;
		win->axes.ymid += (y - my) * win->axes.dp;
		graphics_axes_recalculate(&win->axes);
	}
	mx = x;
	my = y;
}
static void scroll_callback(struct nk_glfw *nk_win, double xoff, double yoff)
{
	if (nk_item_is_any_active(&nk_win->ctx)) return;
	struct graphics_window *win = nk_win->userdata.ptr;
	//printf("(%g, %g), %g\n", win->axes.xmid, win->axes.ymid, win->axes.dp);
	graphics_axes_zoom(&win->axes, mx * win->axes.dp + win->axes.xmin, win->axes.ymax - my * win->axes.dp, exp(-yoff*0.03));
}
static void windowsize_callback(GLFWwindow *glfw_win, int width, int height)
{
	struct graphics_window *win = ((struct nk_glfw*)glfwGetWindowUserPointer(glfw_win))->userdata.ptr;
	win->axes.width = width;
	win->axes.height = height;
	graphics_axes_recalculate(&win->axes);
}

int main(int argc, char **argv)
{
	FILE *oom_score_adj = fopen("/proc/self/oom_score_adj", "w+");
	if (oom_score_adj != NULL)
	{
		fprintf(oom_score_adj, "1000\n");
		fclose(oom_score_adj);
		printf("set own oom_score_adj to 1000\n");
	}

	graphics_init();

	struct graphics_window win =
	{
		.nk =
		{
			.width = 600, .height = 600,
			.glfw_key_callback = key_callback,
			.glfw_char_callback = char_callback,
			.glfw_mousebutton_callback = mousebutton_callback,
			.glfw_cursorpos_callback = cursorpos_callback,
			.glfw_scroll_callback = scroll_callback,
		},
		.background = nk_rgb(0, 0, 0)
	};
	graphics_window_init(&win);

	glfwSetWindowSizeCallback(win.nk.win, windowsize_callback);

	struct graphics_shader_program shaders;
	graphics_shader_program_new (&shaders);
	graphics_shader_add_file    (&shaders, GL_VERTEX_SHADER  , "shader.v.glsl");
	graphics_shader_add_file    (&shaders, GL_FRAGMENT_SHADER, "shader.f.glsl");
	graphics_shader_program_link(&shaders);
	if (shaders.status != GRAPHICS_SHADER_PROGRAM_LINKED) abort();
	printf("location(origin) = %d\n", glGetUniformLocation(shaders.program, "origin"));
	printf("location(scale) = %d\n", glGetUniformLocation(shaders.program, "scale"));
	printf("location(grid_scale) = %d\n", glGetUniformLocation(shaders.program, "grid_scale"));
	printf("location(grid_intensity) = %d\n", glGetUniformLocation(shaders.program, "grid_intensity"));
	printf("location(plot_color) = %d\n", glGetUniformLocation(shaders.program, "plot_color"));
	printf("location(time) = %d\n", glGetUniformLocation(shaders.program, "time"));
	graphics_check_gl_error("post check link");

	win.axes = (struct graphics_axes)
	{
		.xmid = 0.0, .ymid = 0.0,
		.dp = 2.0 / win.nk.display_width,
		.grid_base_x = 10.0, .grid_base_y = 10.0,
		.width  = win.nk.display_width,
		.height = win.nk.display_height,
	};
	graphics_axes_new(&win.axes);
	graphics_axes_recalculate(&win.axes);

	struct graphics_graph_parameter params[4];
	win.graph_params = params;
	win.graph_params_n = 4;
	char name[64];
	for (size_t i = 0; i < 4; i++)
	{
		snprintf(name, 64, "Param %zd ", i);
		graphics_graph_parameter_new_at(&win.graph_params[i], name, 2, 0.0f);
	}

	while (!glfwWindowShouldClose(win.nk.win))
	{
		nk_input_begin(&win.nk.ctx);
		glfwPollEvents();
		nk_input_end(&win.nk.ctx);

		win.shader_program = shaders.program;
		graphics_window_render(&win);
	}

	for (size_t i = 0; i < 4; i++)
	{
		graphics_graph_parameter_dtor(&win.graph_params[i]);
	}

	graphics_axes_dtor(&win.axes);
	graphics_shader_program_dtor(&shaders);
	graphics_window_dtor(&win);

	graphics_quit();

	return 0;
}
