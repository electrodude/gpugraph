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

	int grid_en;

	struct aem_stringbuf title;

	struct graphics_axes axes;

	struct graphics_graph graph_list;

	struct graphics_window *win_prev;
	struct graphics_window *win_next;
	int id;

	struct aem_stringbuf eqn_pfx;
};

extern struct graphics_window graphics_window_list;

int graphics_window_init(struct graphics_window *win, const char *title);
void graphics_window_free(struct graphics_window *win);

static inline void graphics_window_select(struct graphics_window *win);

void graphics_window_update_title(struct graphics_window *win);

int graphics_window_draw(struct graphics_window *win);
void graphics_window_render(struct graphics_window *win);


// implementations

static inline void graphics_window_select(struct graphics_window *win)
{
	glfwMakeContextCurrent(win->nk.win);
}


#endif /* GRAPHICS_H */
