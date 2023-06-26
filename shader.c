#include <stdlib.h>
#include <stdio.h>

#include <aem/stringbuf.h>

#include "graphics.h"

#include "shader.h"

void graphics_shader_program_new(struct graphics_shader_program *program)
{
	program->status = GRAPHICS_SHADER_PROGRAM_CONSTRUCTING;

	program->program = glCreateProgram();
}

int graphics_shader_program_link(struct graphics_shader_program *program)
{
	if (program->status != GRAPHICS_SHADER_PROGRAM_CONSTRUCTING) {
		aem_logf_ctx(AEM_LOG_ERROR, "shader program not under construction");
		return -1;
	}

	program->status = GRAPHICS_SHADER_PROGRAM_LINKED;

	graphics_check_gl_error("pre link");

	glLinkProgram(program->program);

	int program_ok;
	glGetProgramiv(program->program, GL_LINK_STATUS, &program_ok);
	if (!program_ok) {

		GLsizei log_length;
		glGetProgramiv(program->program, GL_INFO_LOG_LENGTH, &log_length);
		AEM_LOG_MULTI(out, AEM_LOG_ERROR) {
			aem_stringbuf_puts(out, "link failed:\n");
			aem_stringbuf_reserve(out, log_length);
			glGetProgramInfoLog(program->program, log_length, &log_length, aem_stringbuf_end(out));
			out->n += log_length;
		}

		glDeleteProgram(program->program);
		program->program = 0;
		program->status = GRAPHICS_SHADER_PROGRAM_FAIL;
		return -1;
	}

	graphics_check_gl_error("post link");

	return 0;
}

int graphics_shader_program_dtor(struct graphics_shader_program *program)
{
	/* Not necessary, done automatically by glDeleteProgram below
	AEM_LL_FOR_ALL(shader, &program->shaders, next) {
		glDetachShader(program->program, shader->id);
	}
	*/

	if (program->status == GRAPHICS_SHADER_PROGRAM_FAIL)
		return 1;

	glDeleteProgram(program->program);
	program->program = 0;
	program->status = GRAPHICS_SHADER_PROGRAM_FAIL;

	return 0;
}

int graphics_shader_add(struct graphics_shader_program *program, GLenum type, struct aem_stringslice source)
{
	if (program->status != GRAPHICS_SHADER_PROGRAM_CONSTRUCTING) {
		aem_logf_ctx(AEM_LOG_ERROR, "shader program not under construction");
		return -1;
	}

	const char *p = source.start;
	GLint length = aem_stringslice_len(source);

	graphics_check_gl_error("pre compile");

	int shader = glCreateShader(type);
	glShaderSource(shader, 1, (const GLchar**)&p, &length);
	glCompileShader(shader);

	int shader_ok;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_ok);
	if (!shader_ok) {

		GLsizei log_length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
		AEM_LOG_MULTI(out, AEM_LOG_ERROR) {
			aem_stringbuf_puts(out, "compilation failed:\n");
			aem_stringbuf_reserve(out, log_length);
			glGetShaderInfoLog(shader, log_length, &log_length, aem_stringbuf_end(out));
			out->n += log_length;
		}

		glDeleteShader(shader);
		program->status = GRAPHICS_SHADER_PROGRAM_FAIL;
		return -1;
	}

	graphics_check_gl_error("post compile");

	// TODO: program->? = shader;

	glAttachShader(program->program, shader);

	graphics_check_gl_error("post attach");

	// unref shader
	glDeleteShader(shader);

	graphics_check_gl_error("post delete");

	return shader;
}

int graphics_shader_add_file(struct graphics_shader_program *program, GLenum type, const char *path)
{
	FILE *fp = fopen(path, "r");
	if (!fp) {
		aem_logf_ctx(AEM_LOG_ERROR, "file not found: %s", path);
		return -1;
	}

	struct aem_stringbuf source = AEM_STRINGBUF_EMPTY;
	aem_stringbuf_file_read_all(&source, fp);
	fclose(fp);

	int shader = graphics_shader_add(program, type, aem_stringslice_new_str(&source));

	aem_stringbuf_dtor(&source);

	return shader;
}
