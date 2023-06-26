#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "aem/linked_list.h"

#include "graphics.h"

#include "controls.h"

#include "session_load.h"
#include "session_save.h"

#include "axes.h"

#include "graph_nuklear.h"

// these two need to be local to each window
static int buttons[GLFW_MOUSE_BUTTON_LAST];
static double mx, my;

static int animate_next = 0;

static void key_callback(struct nk_glfw *nk_win, int key, int scancode, int action, int mods) {}
static void char_callback(struct nk_glfw *nk_win, unsigned int codepoint) {}
static void mousebutton_callback(struct nk_glfw *nk_win, int button, int action, int mods)
{
	if (action != GLFW_RELEASE && nk_item_is_any_active(&nk_win->ctx))
		return;
	struct graphics_window *win = nk_win->userdata.ptr;
	//printf("mouse button: %d, %d\n", button, action);
	buttons[button] = action;
	animate_next = 1;
}
static void cursorpos_callback(struct nk_glfw *nk_win, double x, double y)
{
	struct graphics_window *win = nk_win->userdata.ptr;
	if (buttons[GLFW_MOUSE_BUTTON_LEFT]) {
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
	if (nk_item_is_any_active(&nk_win->ctx))
		return;
	struct graphics_window *win = nk_win->userdata.ptr;
	//printf("(%g, %g), %g\n", win->axes.xmid, win->axes.ymid, win->axes.dp);
	graphics_axes_zoom(&win->axes, mx * win->axes.dp + win->axes.xmin, win->axes.ymax - my * win->axes.dp, exp(-yoff*0.03));
	animate_next = 1;
}
static void windowsize_callback(GLFWwindow *glfw_win, int width, int height)
{
	struct graphics_window *win = ((struct nk_glfw*)glfwGetWindowUserPointer(glfw_win))->userdata.ptr;
	win->axes.width = width;
	win->axes.height = height;
	graphics_axes_recalculate(&win->axes);
	animate_next = 1;
}

struct graphics_window *graph_window_new(void)
{
	struct graphics_window *win = malloc(sizeof(*win));
	*win = (struct graphics_window){
		.nk = {
			.width = 800, .height = 600,
			.glfw_key_callback = key_callback,
			.glfw_char_callback = char_callback,
			.glfw_mousebutton_callback = mousebutton_callback,
			.glfw_cursorpos_callback = cursorpos_callback,
			.glfw_scroll_callback = scroll_callback,
		},
		.background = nk_rgb(0, 0, 0),
		.hsv = 0,
		.grid_en = 1,
	};

	graphics_window_init(win, "gpugraph");

	aem_stringbuf_reset(&win->title);
	aem_stringbuf_puts(&win->title, "gpugraph ");
	aem_stringbuf_putnum(&win->title, 10, win->id);

	graphics_window_update_title(win);

	glfwSetWindowSizeCallback(win->nk.win, windowsize_callback);

	win->axes = (struct graphics_axes){
		.xmid = 0.0, .ymid = 0.0,
		.dp = 2.0 / win->nk.display_width,
		.grid_base_x = 10.0, .grid_base_y = 10.0,
		.width  = win->nk.display_width,
		.height = win->nk.display_height,
	};
	graphics_axes_new(&win->axes);
	graphics_axes_recalculate(&win->axes);

	return win;
}

struct aem_stringbuf graphics_axes_shader_path = {0};

int main(int argc, char **argv)
{
	FILE *oom_score_adj = fopen("/proc/self/oom_score_adj", "w+");
	if (oom_score_adj) {
		// Kill me first.
		// (Is this necessary?  Does this still actually use tons of ram, or was it just a bug?)
		fprintf(oom_score_adj, "1000\n");
		fclose(oom_score_adj);
#if 0
		fprintf(stderr, "set own oom_score_adj to 1000\n");
#endif
	}

	// should go somewhere else
	AEM_LL_INIT(&graphics_graph_parameters, prev, next);
	graphics_graph_parameters.id = -1;

	aem_stringbuf_reset(&graphics_axes_shader_path);
	aem_stringbuf_puts(&graphics_axes_shader_path, "/home/albertemanuel/code/c/gpugraph/"); // TODO: Hardcoded path!

	graphics_init();

	if (argc > 1) {
		const char *f = argv[1];

		if (session_load_path(f))
			graph_window_new();
	} else {
		graph_window_new();
	}

	while (graphics_window_list.next != &graphics_window_list) {
		int animating = animate_next;
		if (animate_next)
			animate_next--;

		static float time_old = 0.0;
		float time_new = glfwGetTime();
		float dt = time_new - time_old;
		time_old = time_new;

		if (graphics_graph_parameter_update_all(dt)) {
			//printf("animating\n");
			animating = 1;
		}

		AEM_LL_FOR_ALL(win, &graphics_window_list, prev, next) {
			if (graphics_window_draw(win)) {
				//animating = 1;
			}

			if (glfwWindowShouldClose(win->nk.win)) {
				AEM_LL_REMOVE(win, prev, next);
				graphics_axes_dtor(&win->axes);
				graphics_window_dtor(win);

				free(win);
			}
		}

		AEM_LL_FOR_ALL(win, &graphics_window_list, prev, next) {
			graphics_window_render(win);

			if (glfwWindowShouldClose(win->nk.win)) {
				AEM_LL_REMOVE(win, prev, next);
				graphics_axes_dtor(&win->axes);
				graphics_window_dtor(win);

				free(win);
			}
		}

		AEM_LL_FOR_ALL(win, &graphics_window_list, prev, next) {
			graphics_window_select(win);
			nk_input_begin(&win->nk.ctx);
		}

		if (animating) {
			glfwPollEvents();
		} else {
			glfwWaitEvents();
			glfwWaitEvents(); // TODO: fix this
		}
		graphics_check_gl_error("events");
		AEM_LL_FOR_ALL(win, &graphics_window_list, prev, next) {
			graphics_window_select(win);
			nk_input_end(&win->nk.ctx);
		}
	}

	if (graphics_window_list.next != &graphics_window_list) {
		// only autosave on quit if any windows are left
		session_save_path("session_quit.txt");
	}

	AEM_LL_FOR_ALL(win, &graphics_window_list, prev, next) {
		graphics_window_dtor(win);
	}

	graphics_quit();

	return 0;
}
