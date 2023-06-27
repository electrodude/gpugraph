#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include <aem/linked_list.h>
#include <aem/log.h>
#include <aem/stringbuf.h>

#include "graphics.h"
#include "controls.h"

#include "session_load.h"

#define SESSION_LOAD_EXPECT(p, what, token) \
	if (!aem_stringslice_match(p, token)) \
	{ \
		aem_logf_ctx(AEM_LOG_ERROR, "failed parsing %s: expected %s", #what, token); \
		return 1; \
	}

void session_load_consume_whitespace(struct aem_stringslice *p)
{
	while (aem_stringslice_ok(*p)) {
		if (isspace(*p->start)) {
			p->start++;
		} else if (aem_stringslice_match(p, "//")) {
			aem_stringslice_match_line(p);
		} else if (aem_stringslice_match(p, "/*")) {
			while (aem_stringslice_ok(*p) && !aem_stringslice_match(p, "*/"))
				p->start++;
		} else {
			break;
		}
	}
}

int session_load_parse_boolean(struct aem_stringslice *p, int *flag)
{
	aem_logf_ctx(AEM_LOG_DEBUG, "parsing boolean");

	session_load_consume_whitespace(p);
	struct aem_stringslice token = aem_stringslice_match_word(p);
	     if (aem_stringslice_match(&token, "t")) *flag = 1;
	else if (aem_stringslice_match(&token, "f")) *flag = 0;
	else if (aem_stringslice_match(&token, "1")) *flag = 1;
	else if (aem_stringslice_match(&token, "0")) *flag = 0;
	else if (aem_stringslice_match(&token, "y")) *flag = 1;
	else if (aem_stringslice_match(&token, "n")) *flag = 0;
	else
	{
		aem_logf_ctx(AEM_LOG_ERROR, "failed parsing boolean");
		return 1;
	}


	aem_logf_ctx(AEM_LOG_DEBUG, "done parsing boolean");

	return 0;
}

int session_load_parse_color(struct aem_stringslice *p, float (*color)[4])
{
	aem_logf_ctx(AEM_LOG_DEBUG, "parsing color");

	session_load_consume_whitespace(p);
	if (aem_stringslice_match(p, "#")) {
		for (size_t i = 0; i < 4; i++) {
			session_load_consume_whitespace(p);
			int c = aem_stringslice_match_hexbyte(p);
			if (c == -1) {
				aem_logf_ctx(AEM_LOG_DEBUG, "failed parsing color: expected hex number");
				return 1;
			}
			(*color)[i] = c / 255.0;
			aem_logf_ctx(AEM_LOG_DEBUG, "color[%zd] = %d", i, c);
		}
	} else {
		for (size_t i = 0; i < 4; i++) {
			session_load_consume_whitespace(p);
			char *pe;
			(*color)[i] = strtof(p->start, &pe);
			if (p->start == pe) {
				aem_logf_ctx(AEM_LOG_ERROR, "failed parsing color: expected float");
				return 1;
			}
			p->start = pe;
			aem_logf_ctx(AEM_LOG_DEBUG, "color[%zd] = %g", i, (*color)[i]);
		}
	}

	aem_logf_ctx(AEM_LOG_DEBUG, "done parsing color");

	return 0;
}

int session_load_parse_param(struct aem_stringslice *p)
{
	aem_logf_ctx(AEM_LOG_DEBUG, "parsing param");

	session_load_consume_whitespace(p);
	char *pe;
	size_t count = strtol(p->start, &pe, 10);
	if (p->start == pe) {
		aem_logf_ctx(AEM_LOG_ERROR, "failed parsing param: expected integer");
		return 1;
	}
	p->start = pe;
	aem_logf_ctx(AEM_LOG_DEBUG, "count %zd", count);

	struct graphics_graph_parameter *param = graphics_graph_parameter_new(count);

	for (size_t i = 0; i < count; i++) {
		session_load_consume_whitespace(p);
		char *pe;
		param->values[i] = strtof(p->start, &pe);
		if (p->start == pe) {
			aem_logf_ctx(AEM_LOG_ERROR, "failed parsing param: expected float");
			return 1;
		}
		p->start = pe;
		aem_logf_ctx(AEM_LOG_DEBUG, "values[%zd] = %g", i, param->values[i]);
	}

	session_load_consume_whitespace(p);
	struct aem_stringslice name = aem_stringslice_trim(aem_stringslice_match_line(p));
	AEM_LOG_MULTI(out, AEM_LOG_DEBUG) {
		aem_stringbuf_puts(out, "name ");
		aem_stringbuf_putss(out, name);
	}

	aem_stringbuf_reset(&param->name);
	aem_stringbuf_putss(&param->name, name);

	aem_logf_ctx(AEM_LOG_DEBUG, "done parsing param");

	return 0;
}

int session_load_parse_view(struct aem_stringslice *p, struct graphics_graph *graph)
{
	aem_logf_ctx(AEM_LOG_DEBUG, "parsing view");

	session_load_consume_whitespace(p);
	struct aem_stringslice name = aem_stringslice_match_word(p);

	AEM_LOG_MULTI(out, AEM_LOG_DEBUG) {
		aem_stringbuf_puts(out, "name ");
		aem_stringbuf_putss(out, name);
	}

	session_load_consume_whitespace(p);
	SESSION_LOAD_EXPECT(p, view, "=");

	session_load_consume_whitespace(p);
	struct aem_stringslice param_name = aem_stringslice_trim(aem_stringslice_match_line(p));

	AEM_LOG_MULTI(out, AEM_LOG_DEBUG) {
		aem_stringbuf_puts(out, "param name ");
		aem_stringbuf_putss(out, param_name);
	}

	struct graphics_graph_parameter *param = graphics_graph_parameter_lookup(param_name);
	if (!param) {
		AEM_LOG_MULTI(out, AEM_LOG_DEBUG) {
			aem_stringbuf_puts(out, "failed parsing view: no such param: \"");
			aem_stringbuf_putss(out, param_name);
			aem_stringbuf_puts(out, "\"");
		}
		return 1;
	}
	struct graphics_graph_parameter_view *view = graphics_graph_parameter_view_new(name, param);
	AEM_LL2_INSERT_BEFORE(&graph->params, view, param_view);

	aem_logf_ctx(AEM_LOG_DEBUG, "done parsing view");

	return 0;
}

int session_load_parse_shader(struct aem_stringslice *p, struct graphics_graph *graph)
{
	aem_logf_ctx(AEM_LOG_DEBUG, "parsing shader");

	session_load_consume_whitespace(p);
	struct aem_stringslice type = aem_stringslice_trim(aem_stringslice_match_line(p));

	AEM_LOG_MULTI(out, AEM_LOG_DEBUG) {
		aem_stringbuf_puts(out, "type ");
		aem_stringbuf_putss(out, type);
	}

	session_load_consume_whitespace(p);
	SESSION_LOAD_EXPECT(p, shader, "{");

	aem_stringslice_match_ws(p);

	struct aem_stringslice shader;
	shader.start = p->start;

	size_t depth = 0;
	while (aem_stringslice_ok(*p)) {
		session_load_consume_whitespace(p);

		char c = *p->start;

		if (c == '{') {
			depth++;
		} else if (c == '}') {
			if (depth == 0)
				break;
			depth--;
		}
		// TODO: does GLSL have string literals or anything that can contain curly brackets?

		p->start++;
	}

	shader.end = p->start;
	while (aem_stringslice_ok(shader) && isspace(shader.end[-1]))
		shader.end--;

	aem_stringbuf_reset(&graph->eqn);

	// unindent shader code
	while (aem_stringslice_ok(shader)) {
		struct aem_stringslice line = aem_stringslice_trim(aem_stringslice_match_line(&shader));

		aem_stringslice_match(&line, "\t\t\t");

		aem_stringbuf_putss(&graph->eqn, line);
		aem_stringbuf_putc(&graph->eqn, '\n');
	}

	aem_logf_ctx(AEM_LOG_DEBUG, "done parsing shader");
	session_load_consume_whitespace(p);
	struct aem_stringslice line = aem_stringslice_trim(aem_stringslice_match_line(p));
	AEM_LOG_MULTI(out, AEM_LOG_DEBUG) {
		aem_stringbuf_puts(out, "next line: \"");
		aem_stringbuf_putss(out, line);
		aem_stringbuf_puts(out, "\"");
	}

	return 0;
}

int session_load_parse_window_view(struct aem_stringslice *p, struct graphics_window *win)
{
	aem_logf_ctx(AEM_LOG_DEBUG, "parsing window view");

	session_load_consume_whitespace(p);
	{
		char *pe;
		win->axes.xmid = strtod(p->start, &pe);
		if (p->start == pe) {
			aem_logf_ctx(AEM_LOG_ERROR, "failed parsing window view: expected double");
			return 1;
		}
		p->start = pe;
		aem_logf_ctx(AEM_LOG_DEBUG, "xmid %g", win->axes.xmid);
	}
	session_load_consume_whitespace(p);
	SESSION_LOAD_EXPECT(p, shader, ",");
	session_load_consume_whitespace(p);
	{
		char *pe;
		win->axes.ymid = strtod(p->start, &pe);
		if (p->start == pe) {
			aem_logf_ctx(AEM_LOG_ERROR, "failed parsing window view: expected double");
			return 1;
		}
		p->start = pe;
		aem_logf_ctx(AEM_LOG_DEBUG, "ymid %g", win->axes.ymid);
	}
	session_load_consume_whitespace(p);
	SESSION_LOAD_EXPECT(p, shader, ";");
	session_load_consume_whitespace(p);
	{
		char *pe;
		win->axes.dp = strtod(p->start, &pe);
		if (p->start == pe) {
			aem_logf_ctx(AEM_LOG_ERROR, "failed parsing window view: expected double");
			return 1;
		}
		p->start = pe;
		aem_logf_ctx(AEM_LOG_DEBUG, "dp %g", win->axes.dp);
	}

	aem_logf_ctx(AEM_LOG_DEBUG, "done parsing window view");

	return 0;
}

int session_load_parse_graph(struct aem_stringslice *p, struct graphics_window *win)
{
	aem_logf_ctx(AEM_LOG_DEBUG, "parsing graph");

	session_load_consume_whitespace(p);
	struct aem_stringslice type = aem_stringslice_match_word(p);

	AEM_LOG_MULTI(out, AEM_LOG_DEBUG) {
		aem_stringbuf_puts(out, "type ");
		aem_stringbuf_putss(out, type);
	}

	session_load_consume_whitespace(p);
	struct aem_stringslice name = aem_stringslice_trim(aem_stringslice_match_line(p));

	AEM_LOG_MULTI(out, AEM_LOG_DEBUG) {
		aem_stringbuf_puts(out, "name ");
		aem_stringbuf_putss(out, name);
	}

	session_load_consume_whitespace(p);
	SESSION_LOAD_EXPECT(p, graph, "{");

	struct graphics_graph *graph = graphics_graph_new(win);
	AEM_LL2_INSERT_BEFORE(&win->graph_list, graph, graph);
	AEM_LL2_VERIFY(&win->graph_list, graph_prev, graph_next, assert);

	graph->dock = 1;

	aem_stringbuf_reset(&graph->name);
	aem_stringbuf_putss(&graph->name, name);

	while (aem_stringslice_ok(*p)) {
		session_load_consume_whitespace(p);

		if (aem_stringslice_match(p, "param")) {
			int rc = session_load_parse_view(p, graph);
			if (rc)
				goto fail;
		} else if (aem_stringslice_match(p, "color")) {
			int rc = session_load_parse_color(p, &graph->color);
			if (rc)
				goto fail;
		} else if (aem_stringslice_match(p, "hsv")) {
			int rc = session_load_parse_boolean(p, &graph->hsv);
			if (rc)
				goto fail;
		} else if (aem_stringslice_match(p, "enable")) {
			int rc = session_load_parse_boolean(p, &graph->enable);
			if (rc)
				goto fail;
		} else if (aem_stringslice_match(p, "dock")) {
			int rc = session_load_parse_boolean(p, &graph->dock);
			if (rc)
				goto fail;
		} else if (aem_stringslice_match(p, "shader")) {
			int rc = session_load_parse_shader(p, graph);
			if (rc)
				goto fail;
		} else if (aem_stringslice_match(p, "}")) {
			break;
		} else {
			aem_logf_ctx(AEM_LOG_ERROR, "failed parsing graph");
			goto fail;
		}
	}

	graphics_graph_setup(graph);

	aem_logf_ctx(AEM_LOG_DEBUG, "done parsing graph");

	return 0;

fail:
	graph->eqn_shader.status = GRAPHICS_SHADER_PROGRAM_FAIL;

	return 1;
}

int session_load_parse_window(struct aem_stringslice *p)
{
	aem_logf_ctx(AEM_LOG_DEBUG, "parsing window");

	session_load_consume_whitespace(p);
	struct aem_stringslice title = aem_stringslice_trim(aem_stringslice_match_line(p));

	AEM_LOG_MULTI(out, AEM_LOG_DEBUG) {
		aem_stringbuf_puts(out, "title ");
		aem_stringbuf_putss(out, title);
	}

	session_load_consume_whitespace(p);
	SESSION_LOAD_EXPECT(p, window, "{");

	struct graphics_window *graph_window_new(void); // from main.c
	struct graphics_window *win = graph_window_new();

	aem_stringbuf_reset(&win->title);
	aem_stringbuf_putss(&win->title, title);

	while (aem_stringslice_ok(*p)) {
		session_load_consume_whitespace(p);
		if (aem_stringslice_match(p, "graph")) {
			int rc = session_load_parse_graph(p, win);
			if (rc)
				return rc;
		} else if (aem_stringslice_match(p, "view")) {
			int rc = session_load_parse_window_view(p, win);
			if (rc)
				return rc;
		} else if (aem_stringslice_match(p, "color")) {
			float color[4];
			int rc = session_load_parse_color(p, &color);
			if (rc)
				return rc;
			win->background = nk_hsva_fv(color);
		} else if (aem_stringslice_match(p, "hsv")) {
			int rc = session_load_parse_boolean(p, &win->hsv);
			if (rc)
				return rc;
		} else if (aem_stringslice_match(p, "grid")) {
			int rc = session_load_parse_boolean(p, &win->grid_en);
			if (rc)
				return rc;
		} else if (aem_stringslice_match(p, "pfx")) {
			// TODO: error checking
			session_load_consume_whitespace(p);
			struct aem_stringslice eqn_pfx = aem_stringslice_trim(aem_stringslice_match_line(p));
			aem_stringbuf_putss(&win->eqn_pfx, eqn_pfx);
			aem_stringbuf_putc(&win->eqn_pfx, '\n');
		} else if (aem_stringslice_match(p, "}")) {
			break;
		} else {
			aem_logf_ctx(AEM_LOG_ERROR, "failed parsing window");
			return 1;
		}
	}

	graphics_axes_recalculate(&win->axes);

	aem_logf_ctx(AEM_LOG_DEBUG, "done parsing window");

	return 0;
}


int session_load_parse(struct aem_stringslice *p)
{
	aem_logf_ctx(AEM_LOG_DEBUG, "parsing file");
	while (aem_stringslice_ok(*p)) {
		session_load_consume_whitespace(p);
		if (aem_stringslice_match(p, "param")) {
			int rc = session_load_parse_param(p);
			if (rc)
				goto fail;
		} else if (aem_stringslice_match(p, "window")) {
			int rc = session_load_parse_window(p);
			if (rc)
				goto fail;
		} else if (aem_stringslice_ok(*p)) {
			aem_logf_ctx(AEM_LOG_ERROR, "failed parsing");
			goto fail;
		}
	}

	aem_logf_ctx(AEM_LOG_DEBUG, "done parsing file");

	return 0;

fail:
	AEM_LOG_MULTI(out, AEM_LOG_DEBUG) {
		aem_stringbuf_puts(out, "failed parsing file at \"");
		struct aem_stringslice line = aem_stringslice_match_line(p);
		aem_stringbuf_putss(out, line);
		aem_stringbuf_puts(out, "\"");
	}

	return 1;
}

int session_load_file(FILE *fp)
{
	struct aem_stringbuf str;
	aem_stringbuf_init_prealloc(&str, 4096);

	aem_stringbuf_file_read_all(&str, fp);
	struct aem_stringslice slice = aem_stringslice_new_str(&str);

	int rc = session_load_parse(&slice);

	aem_stringbuf_dtor(&str);

	return rc;
}

int session_load_path(const char *path)
{
	aem_stringbuf_reset(&session_path);
	aem_stringbuf_puts(&session_path, path);

	FILE *fp = fopen(path, "r");
	if (!fp) {
		aem_logf_ctx(AEM_LOG_ERROR, "failed to load %s", path);
		return 1;
	}

	int rc = session_load_file(fp);
	fclose(fp);

	return rc;
}

int session_load_curr(void)
{
	if (session_path.n == 0)
		return 1;

	return session_load_path(aem_stringbuf_get(&session_path));
}
