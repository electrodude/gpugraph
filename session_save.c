#include "aem/linked_list.h"

#include "graphics.h"
#include "controls.h"

#include "session_save.h"

void session_save_serialize(struct aem_stringbuf *str)
{
	AEM_LL_FOR_ALL(param, &graphics_graph_parameters, prev, next) {
		aem_stringbuf_puts(str, "param ");
		aem_stringbuf_putnum(str, 10, param->count);

		for (size_t i = 0; i < param->count; i++) {
			aem_stringbuf_putc(str, ' ');
			aem_stringbuf_printf(str, "%g", param->values[i]);
		}

		aem_stringbuf_putc(str, ' ');
		aem_stringbuf_append(str, &param->name);

		aem_stringbuf_putc(str, '\n');
	}

	AEM_LL_FOR_ALL(win, &graphics_window_list, prev, next) {
		aem_stringbuf_puts(str, "\nwindow ");
		aem_stringbuf_append(str, &win->title);
		aem_stringbuf_puts(str, "\n{\n");

		aem_stringbuf_printf(str, "\tview %g, %g; %g\n", win->axes.xmid, win->axes.ymid, win->axes.dp);

		if (!win->grid_en) {
			aem_stringbuf_puts(str, "\n\tgrid ");
			aem_stringbuf_putc(str, '0' + win->grid_en);
			aem_stringbuf_putc(str, '\n');
		}

		if (win->eqn_pfx.n) {
			struct aem_stringslice eqn_pfx = aem_stringslice_new_str(&win->eqn_pfx);
			for (struct aem_stringslice pfx_line = aem_stringslice_match_line(&eqn_pfx); aem_stringslice_ok(&pfx_line); pfx_line = aem_stringslice_match_line(&eqn_pfx)) {
				aem_stringbuf_puts(str, "\n\tpfx ");
				aem_stringbuf_append_stringslice(str, pfx_line);
			}
			aem_stringbuf_putc(str, '\n');
		}

		AEM_LL_FOR_ALL(graph, &win->graph_list, prev, next) {
			aem_stringbuf_puts(str, "\n\tgraph ");
			aem_stringbuf_puts(str, "frag");
			aem_stringbuf_putc(str, ' ');
			aem_stringbuf_append(str, &graph->name);
			aem_stringbuf_puts(str, "\n\t{\n");

			AEM_LL_FOR_ALL(view, &graph->params, prev, next) {
				aem_stringbuf_puts(str, "\t\tparam ");

				aem_stringbuf_append(str, &view->name);
				aem_stringbuf_puts(str, " = ");
				aem_stringbuf_append(str, &view->param->name);
				aem_stringbuf_puts(str, "\n");
			}

			if (graph->params.next != &graph->params) { // if any params
				aem_stringbuf_putc(str, '\n');
			}

			aem_stringbuf_puts(str, "\t\tcolor #");
			aem_stringbuf_puthex(str, graph->color[0]*255.0); // TODO: clamp these
			aem_stringbuf_puthex(str, graph->color[1]*255.0);
			aem_stringbuf_puthex(str, graph->color[2]*255.0);
			aem_stringbuf_puthex(str, graph->color[3]*255.0);
			aem_stringbuf_puts(str, "\n");

			if (graph->hsv) {
				aem_stringbuf_puts(str, "\t\thsv ");
				aem_stringbuf_putc(str, '0' + graph->hsv);
				aem_stringbuf_puts(str, "\n");
			}

			aem_stringbuf_puts(str, "\n");

			if (!graph->enable) {
				aem_stringbuf_puts(str, "\t\tenable ");
				aem_stringbuf_putc(str, '0' + graph->enable);
				aem_stringbuf_puts(str, "\n\n");
			}

			aem_stringbuf_puts(str, "\t\tdock ");
			aem_stringbuf_putc(str, '0' + graph->dock);
			aem_stringbuf_puts(str, "\n\n");

			aem_stringbuf_puts(str, "\t\tshader ");
			aem_stringbuf_puts(str, "frag");
			aem_stringbuf_puts(str, "\n\t\t{\n");

			// indent shader code
			struct aem_stringslice eqn = aem_stringslice_new_str(&graph->eqn);
			while (aem_stringslice_ok(&eqn)) {
				struct aem_stringslice line = aem_stringslice_match_line(&eqn);
				aem_stringslice_match(&eqn, "\r");
				aem_stringslice_match(&eqn, "\n");

				aem_stringbuf_puts(str, "\t\t\t");
				aem_stringbuf_append_stringslice(str, line);
				aem_stringbuf_putc(str, '\n');
			}

			aem_stringbuf_puts(str, "\t\t}\n");

			aem_stringbuf_puts(str, "\t}\n");
		}

		aem_stringbuf_puts(str, "}\n");
	}
}

void session_save_file(FILE *fp)
{
	struct aem_stringbuf str;
	aem_stringbuf_init_prealloc(&str, 4096);
	session_save_serialize(&str);

	aem_stringbuf_file_write(&str, fp);

	aem_stringbuf_dtor(&str);
}

struct aem_stringbuf session_path = {0};

int session_save_path(const char *path)
{
	aem_stringbuf_reset(&session_path);
	aem_stringbuf_puts(&session_path, path);

	FILE *fp = fopen(path, "w+");
	if (!fp) {
		fprintf(stderr, "failed to save %s\n", path);
		return 1;
	}

	session_save_file(fp);
	fclose(fp);

	return 0;
}

int session_save_curr(void)
{
	if (session_path.n == 0)
		return 1;

	return session_save_path(aem_stringbuf_get(&session_path));
}
