#include <stdlib.h>
#include <stdio.h>

#include "stringbuf.h"

#include "graphics.h"

#include "shader.h"

int graphics_shader_program_new(struct graphics_shader_program *program)
{
	program->status = GRAPHICS_SHADER_PROGRAM_CONSTRUCTING;

	program->program = glCreateProgram();
}

int graphics_shader_program_link(struct graphics_shader_program *program)
{
	if (program->status != GRAPHICS_SHADER_PROGRAM_CONSTRUCTING)
	{
		fprintf(stderr, "graphics_shader_add: shader program not under construction\n");
		return -1;
	}

	program->status = GRAPHICS_SHADER_PROGRAM_LINKED;

	graphics_check_gl_error("pre link");

	glLinkProgram(program->program);

	int program_ok;
	glGetProgramiv(program->program, GL_LINK_STATUS, &program_ok);
	if (!program_ok)
	{
		fprintf(stderr, "graphics_shader_program_link: link failed:\n");

		GLint log_length;
		glGetProgramiv(program->program, GL_INFO_LOG_LENGTH, &log_length);
		char *log = malloc(log_length);
		glGetProgramInfoLog(program->program, log_length, NULL, log);
		fprintf(stderr, "%d %s", log_length, log);
		free(log);

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
	/* TODO:
	for (shader : program->shaders)
	{
		glDetachShader(program->program, shader);
	}
	*/

	if (program->status == GRAPHICS_SHADER_PROGRAM_FAIL) return 1;

	glDeleteProgram(program->program);
	program->program = 0;
	program->status = GRAPHICS_SHADER_PROGRAM_FAIL;

	return 0;
}

int graphics_shader_add(struct graphics_shader_program *program, GLenum type, struct stringslice source)
{
	if (program->status != GRAPHICS_SHADER_PROGRAM_CONSTRUCTING)
	{
		fprintf(stderr, "graphics_shader_add: shader program not under construction\n");
		return -1;
	}

	const char *p = source.start;
	GLint length = stringslice_len(&source);

	graphics_check_gl_error("pre compile");

	int shader = glCreateShader(type);
	glShaderSource(shader, 1, (const GLchar**)&p, &length);
	glCompileShader(shader);

	int shader_ok;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_ok);
	if (!shader_ok)
	{
		fprintf(stderr, "graphics_shader_add: compilation failed:\n");

		GLint log_length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
		char *log = malloc(log_length);
		glGetShaderInfoLog(shader, log_length, NULL, log);
		fprintf(stderr, "%s", log);
		free(log);

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
	if (fp == NULL)
	{
		fprintf(stderr, "graphics_shader_add_file: file not found: %s\n", path);
		return -1;
	}

	struct stringbuf source = STRINGBUF_EMPTY;
	stringbuf_file_read(&source, fp);
	fclose(fp);

	int shader = graphics_shader_add(program, type, stringslice_new_str(source));

	stringbuf_dtor(&source);

	return shader;
}
