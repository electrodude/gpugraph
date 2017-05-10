#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "linked_list.h"

#include "stringbuf.h"

#include "graphics.h"
#include "controls.h"

#include "session_load.h"

#define SESSION_LOAD_DEBUG 0
#if SESSION_LOAD_DEBUG
#define dprintf(...) fprintf(stderr, __VA_ARGS__)
#else
#define dprintf(...)
#endif

#define SESSION_LOAD_EXPECT(p, what, token) \
	if (!stringslice_match(p, token)) \
	{ \
		fprintf(stderr, "failed parsing " #what ": expected " token "\n"); \
		return 1; \
	}

void session_load_consume_whitespace(struct stringslice *p)
{
	while (stringslice_ok(p))
	{
		if (isspace(*p->start))
		{
			p->start++;
		}
		else if (stringslice_match(p, "//"))
		{
			stringslice_match_line(p);
		}
		else if (stringslice_match(p, "/*"))
		{
			while (stringslice_ok(p) && !stringslice_match(p, "*/")) p->start++;
		}
		else
		{
			break;
		}
	}
}

int session_load_parse_boolean(struct stringslice *p, int *flag)
{
	dprintf("parsing boolean\n");

	session_load_consume_whitespace(p);
	struct stringslice token = stringslice_match_word(p);
	     if (stringslice_match(&token, "t")) *flag = 1;
	else if (stringslice_match(&token, "f")) *flag = 0;
	else if (stringslice_match(&token, "1")) *flag = 1;
	else if (stringslice_match(&token, "0")) *flag = 0;
	else if (stringslice_match(&token, "y")) *flag = 1;
	else if (stringslice_match(&token, "n")) *flag = 0;
	else
	{
		fprintf(stderr, "failed parsing boolean\n");
		return 1;
	}


	dprintf("done parsing boolean\n");

	return 0;
}

int session_load_parse_color(struct stringslice *p, float (*color)[4])
{
	dprintf("parsing color\n");

	session_load_consume_whitespace(p);
	if (stringslice_match(p, "#"))
	{
		for (size_t i = 0; i < 4; i++)
		{
			session_load_consume_whitespace(p);
			int c = stringslice_match_hexbyte(p);
			if (c == -1)
			{
				dprintf("failed parsing color: expected hex number\n");
				return 1;
			}
			(*color)[i] = c / 255.0;
			dprintf("color[%zd] = %d\n", i, c);
		}
	}
	else
	{
		for (size_t i = 0; i < 4; i++)
		{
			session_load_consume_whitespace(p);
			char *pe;
			(*color)[i] = strtof(p->start, &pe);
			if (p->start == pe)
			{
				fprintf(stderr, "failed parsing color: expected float\n");
				return 1;
			}
			p->start = pe;
			dprintf("color[%zd] = %g\n", i, (*color)[i]);
		}
	}

	dprintf("done parsing color\n");

	return 0;
}

int session_load_parse_param(struct stringslice *p)
{
	dprintf("parsing param\n");

	session_load_consume_whitespace(p);
	char *pe;
	size_t count = strtol(p->start, &pe, 10);
	if (p->start == pe)
	{
		fprintf(stderr, "failed parsing param: expected integer\n");
		return 1;
	}
	p->start = pe;
	dprintf("count %zd\n", count);

	struct graphics_graph_parameter *param = graphics_graph_parameter_new(count);

	for (size_t i = 0; i < count; i++)
	{
		session_load_consume_whitespace(p);
		char *pe;
		param->values[i] = strtof(p->start, &pe);
		if (p->start == pe)
		{
			fprintf(stderr, "failed parsing param: expected float\n");
			return 1;
		}
		p->start = pe;
		dprintf("values[%zd] = %g\n", i, param->values[i]);
	}

	session_load_consume_whitespace(p);
	struct stringslice name = stringslice_match_line(p);
#if SESSION_LOAD_DEBUG
	dprintf("name ");
	stringslice_file_write(&name, stderr);
	dprintf("\n");
#endif

	stringbuf_reset(&param->name);
	stringbuf_append_stringslice(&param->name, name);

	dprintf("done parsing param\n");

	return 0;
}

int session_load_parse_view(struct stringslice *p, struct graphics_graph *graph)
{
	dprintf("parsing view\n");

	session_load_consume_whitespace(p);
	struct stringslice name = stringslice_match_word(p);
#if SESSION_LOAD_DEBUG
	dprintf("name \"");
	stringslice_file_write(&name, stderr);
	dprintf("\"\n");
#endif

	session_load_consume_whitespace(p);
	SESSION_LOAD_EXPECT(p, view, "=");

	session_load_consume_whitespace(p);
	struct stringslice param_name = stringslice_match_line(p);
#if SESSION_LOAD_DEBUG
	dprintf("param name \"");
	stringslice_file_write(&param_name, stderr);
	dprintf("\"\n");
#endif

	struct graphics_graph_parameter *param = graphics_graph_parameter_lookup(param_name);
	if (param == NULL)
	{
		fprintf(stderr, "failed parsing view: no such param: \"");
		stringslice_file_write(&param_name, stderr);
		fprintf(stderr, "\"\n");
		return 1;
	}
	struct graphics_graph_parameter_view *view = graphics_graph_parameter_view_new(name, param);
	LL_INSERT_BEFORE(&graph->params, view, prev, next);

	dprintf("done parsing view\n");

	return 0;
}

int session_load_parse_shader(struct stringslice *p, struct graphics_graph *graph)
{
	dprintf("parsing shader\n");

	session_load_consume_whitespace(p);
	struct stringslice type = stringslice_match_line(p);

#if SESSION_LOAD_DEBUG
	dprintf("type \"");
	stringslice_file_write(&type, stderr);
	dprintf("\"\n");
#endif

	session_load_consume_whitespace(p);
	SESSION_LOAD_EXPECT(p, shader, "{");

	stringslice_match_ws(p);

	struct stringslice shader;
	shader.start = p->start;

	size_t depth = 0;
	while (stringslice_ok(p))
	{
		session_load_consume_whitespace(p);

		char c = *p->start;

		if (c == '{')
		{
			depth++;
		}
		else if (c == '}')
		{
			if (depth == 0) break;
			depth--;
		}
		// TODO: does GLSL have string literals or anything that can contain curly brackets?

		p->start++;
	}

	shader.end = p->start;
	while (stringslice_ok(&shader) && isspace(shader.end[-1])) shader.end--;

	stringbuf_reset(&graph->eqn);

	// unindent shader code
	while (stringslice_ok(&shader))
	{
		struct stringslice line = stringslice_match_line(&shader);
		stringslice_match(&shader, "\r");
		stringslice_match(&shader, "\n");

		stringslice_match(&line, "\t\t\t");

		stringbuf_append_stringslice(&graph->eqn, line);
		stringbuf_putc(&graph->eqn, '\n');
	}

	dprintf("done parsing shader\n");
	dprintf("next line: \"");
	session_load_consume_whitespace(p);
	struct stringslice line = stringslice_match_line(p);
#if SESSION_LOAD_DEBUG
	stringslice_file_write(&line, stderr);
#endif
	dprintf("\"\n");

	return 0;
}

int session_load_parse_window_view(struct stringslice *p, struct graphics_window *win)
{
	dprintf("parsing window view\n");

	session_load_consume_whitespace(p);
	{
		char *pe;
		win->axes.xmid = strtod(p->start, &pe);
		if (p->start == pe)
		{
			fprintf(stderr, "failed parsing window view: expected double\n");
			return 1;
		}
		p->start = pe;
		dprintf("xmid %g\n", win->axes.xmid);
	}
	session_load_consume_whitespace(p);
	SESSION_LOAD_EXPECT(p, shader, ",");
	session_load_consume_whitespace(p);
	{
		char *pe;
		win->axes.ymid = strtod(p->start, &pe);
		if (p->start == pe)
		{
			fprintf(stderr, "failed parsing window view: expected double\n");
			return 1;
		}
		p->start = pe;
		dprintf("ymid %g\n", win->axes.ymid);
	}
	session_load_consume_whitespace(p);
	SESSION_LOAD_EXPECT(p, shader, ";");
	session_load_consume_whitespace(p);
	{
		char *pe;
		win->axes.dp = strtod(p->start, &pe);
		if (p->start == pe)
		{
			fprintf(stderr, "failed parsing window view: expected double\n");
			return 1;
		}
		p->start = pe;
		dprintf("dp %g\n", win->axes.dp);
	}

	dprintf("done parsing window view\n");

	return 0;
}

int session_load_parse_graph(struct stringslice *p, struct graphics_window *win)
{
	dprintf("parsing graph\n");

	session_load_consume_whitespace(p);
	struct stringslice type = stringslice_match_word(p);

#if SESSION_LOAD_DEBUG
	dprintf("type \"");
	stringslice_file_write(&type, stderr);
	dprintf("\"\n");
#endif

	session_load_consume_whitespace(p);
	struct stringslice name = stringslice_match_line(p);

#if SESSION_LOAD_DEBUG
	dprintf("name \"");
	stringslice_file_write(&name, stderr);
	dprintf("\"\n");
#endif

	session_load_consume_whitespace(p);
	SESSION_LOAD_EXPECT(p, graph, "{");

	struct graphics_graph *graph = graphics_graph_new();
	LL_INSERT_BEFORE(&win->graph_list, graph, prev, next);
	LL_VERIFY(&win->graph_list, prev, next, assert);

	graph->dock = 1;

	stringbuf_reset(&graph->name);
	stringbuf_append_stringslice(&graph->name, name);

	while (stringslice_ok(p))
	{
		session_load_consume_whitespace(p);

		if (stringslice_match(p, "param"))
		{
			int rc = session_load_parse_view(p, graph);
			if (rc) goto fail;
		}
		else if (stringslice_match(p, "color"))
		{
			int rc = session_load_parse_color(p, &graph->color);
			if (rc) goto fail;
		}
		else if (stringslice_match(p, "hsv"))
		{
			int rc = session_load_parse_boolean(p, &graph->hsv);
			if (rc) goto fail;
		}
		else if (stringslice_match(p, "enable"))
		{
			int rc = session_load_parse_boolean(p, &graph->enable);
			if (rc) goto fail;
		}
		else if (stringslice_match(p, "dock"))
		{
			int rc = session_load_parse_boolean(p, &graph->dock);
			if (rc) goto fail;
		}
		else if (stringslice_match(p, "shader"))
		{
			int rc = session_load_parse_shader(p, graph);
			if (rc) goto fail;
		}
		else if (stringslice_match(p, "}"))
		{
			break;
		}
		else
		{
			fprintf(stderr, "failed parsing graph\n");
			goto fail;
		}
	}

	graphics_graph_setup(graph);

	dprintf("done parsing graph\n");

	return 0;

fail:
	graph->eqn_shader.status = GRAPHICS_SHADER_PROGRAM_FAIL;

	return 1;
}

int session_load_parse_window(struct stringslice *p)
{
	dprintf("parsing window\n");

	session_load_consume_whitespace(p);
	struct stringslice title = stringslice_match_line(p);

	dprintf("title \"");
#if SESSION_LOAD_DEBUG
	stringslice_file_write(&title, stderr);
#endif
	dprintf("\"\n");

	session_load_consume_whitespace(p);
	SESSION_LOAD_EXPECT(p, window, "{");

	struct graphics_window *graph_window_new(void); // from main.c
	struct graphics_window *win = graph_window_new();

	stringbuf_reset(&win->title);
	stringbuf_append_stringslice(&win->title, title);

	while (stringslice_ok(p))
	{
		session_load_consume_whitespace(p);
		if (stringslice_match(p, "graph"))
		{
			int rc = session_load_parse_graph(p, win);
			if (rc) return rc;
		}
		else if (stringslice_match(p, "view"))
		{
			int rc = session_load_parse_window_view(p, win);
			if (rc) return rc;
		}
		else if (stringslice_match(p, "color"))
		{
			float color[4];
			int rc = session_load_parse_color(p, &color);
			if (rc) return rc;
			win->background = nk_hsva_fv(color);
		}
		else if (stringslice_match(p, "hsv"))
		{
			int rc = session_load_parse_boolean(p, &win->hsv);
			if (rc) return rc;
		}
		else if (stringslice_match(p, "}"))
		{
			break;
		}
		else
		{
			fprintf(stderr, "failed parsing window\n");
			return 1;
		}
	}

	graphics_axes_recalculate(&win->axes);

	dprintf("done parsing window\n");

	return 0;
}


int session_load_parse(struct stringslice *p)
{
	dprintf("parsing file\n");
	while (stringslice_ok(p))
	{
		session_load_consume_whitespace(p);
		if (stringslice_match(p, "param"))
		{
			int rc = session_load_parse_param(p);
			if (rc) goto fail;
		}
		else if (stringslice_match(p, "window"))
		{
			int rc = session_load_parse_window(p);
			if (rc) goto fail;
		}
		else if (stringslice_ok(p))
		{
			fprintf(stderr, "failed parsing\n");
			goto fail;
		}
	}

	dprintf("done parsing file\n");

	return 0;

fail:
	fprintf(stderr, "failed parsing file at \"");
	struct stringslice line = stringslice_match_line(p);
	stringslice_file_write(&line, stderr);
	fprintf(stderr, "\"\n");

	return 1;
}

int session_load_file(FILE *fp)
{
	struct stringbuf str;
	stringbuf_new_prealloc_at(&str, 4096);

	stringbuf_file_read(&str, fp);
	struct stringslice slice = stringslice_new_str(str);

	int rc = session_load_parse(&slice);

	stringbuf_dtor(&str);

	return rc;
}

int session_load_path(const char *path)
{
	stringbuf_reset(&session_path);
	stringbuf_puts(&session_path, path);

	FILE *fp = fopen(path, "r");
	if (fp == NULL)
	{
		fprintf(stderr, "failed to load %s\n", path);
		return 1;
	}

	int rc = session_load_file(fp);
	fclose(fp);

	return rc;
}

int session_load_curr(void)
{
	if (session_path.n == 0) return 1;

	return session_load_path(stringbuf_get(&session_path));
}
