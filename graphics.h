#ifndef GRAPHICS_H
#define GRAPHICS_H

#define GL_GLEXT_PROTOTYPES

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear/nuklear.h"
#include "nuklear_glfw_gl2.h"

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


static inline void graphics_check_gl_error(const char *msg)
{
	for (GLenum glerr = glGetError(); glerr; glerr = glGetError())
	{
		// We're just ignoring all GL errors.  Isn't it lovely?
		//fprintf(stderr, "%s: 0x%04x\n", msg, glerr);
		//abort();
	}
}

#endif /* GRAPHICS_H */
