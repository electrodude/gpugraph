#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "axes.h"

#include "controls.h"

int graphics_init(void);
int graphics_quit(void);

struct graphics_window
{
	struct nk_glfw nk;
	struct nk_color background;

	int hsv;

	struct stringbuf title;

	struct graphics_axes axes;

	struct graphics_graph graph_list;

	struct graphics_window *prev;
	struct graphics_window *next;
	int id;
};

extern struct graphics_window *graphics_window_curr;
extern struct graphics_window graphics_window_list;

int graphics_window_init(struct graphics_window *win, const char *title);
int graphics_window_dtor(struct graphics_window *win);

static inline void graphics_window_select(struct graphics_window *win);

void graphics_window_update_title(struct graphics_window *win);

int graphics_window_render(struct graphics_window *win);


// implementations

static inline void graphics_window_select(struct graphics_window *win)
{
	if (graphics_window_curr != win)
	{
		graphics_window_curr = win;
		glfwMakeContextCurrent(win->nk.win);
	}
}

#endif /* GRAPHICS_H */
