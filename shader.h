#ifndef SHADER_H
#define SHADER_H

#include <aem/stringslice.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

struct graphics_shader_program
{
	enum graphics_shader_program_status
	{
		GRAPHICS_SHADER_PROGRAM_FAIL = -1,
		GRAPHICS_SHADER_PROGRAM_CONSTRUCTING = 0,
		GRAPHICS_SHADER_PROGRAM_LINKED,
	} status;

	GLuint program;
};

void graphics_shader_program_init(struct graphics_shader_program *program);
int graphics_shader_program_link(struct graphics_shader_program *program);
int graphics_shader_program_dtor(struct graphics_shader_program *program);

int graphics_shader_add(struct graphics_shader_program *program, GLenum type, struct aem_stringslice source);
int graphics_shader_add_file(struct graphics_shader_program *program, GLenum type, const char *path);

#endif /* SHADER_H */
