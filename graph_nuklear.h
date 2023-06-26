#ifndef GRAPH_NUKLEAR_H
#define GRAPH_NUKLEAR_H

#define GL_GLEXT_PROTOTYPES
#define GLFW_INCLUDE_GLU
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>

#define NK_BUTTON_TRIGGER_ON_RELEASE

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear/nuklear.h"
#include "nuklear_glfw_gl2.h"

#include <stdio.h>

static inline void graphics_check_gl_error(const char *msg)
{
	for (GLenum glerr = glGetError(); glerr; glerr = glGetError())
	{
		// We're just ignoring all GL errors.  Isn't it lovely?
		//fprintf(stderr, "%s: 0x%04x\n", msg, glerr);
		//abort();
	}
}

#endif /* GRAPH_NUKLEAR_H */
