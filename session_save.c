#include "linked_list.h"

#include "graphics.h"
#include "controls.h"

#include "session_save.h"

void session_save_serialize(struct stringbuf *str)
{
	LL_FOR_ALL(param, &graphics_graph_parameters, prev, next)
	{
		stringbuf_puts(str, "param ");
		stringbuf_putnum(str, 10, param->count);

		for (size_t i = 0; i < param->count; i++)
		{
			stringbuf_putc(str, ' ');
			stringbuf_printf(str, "%g", param->values[i]);
		}

		stringbuf_putc(str, ' ');
		stringbuf_append(str, &param->name);

		stringbuf_putc(str, '\n');
	}

	LL_FOR_ALL(win, &graphics_window_list, prev, next)
	{
		stringbuf_puts(str, "\nwindow ");
		stringbuf_append(str, &win->title);
		stringbuf_puts(str, "\n{\n");

		stringbuf_printf(str, "\tview %g, %g; %g\n", win->axes.xmid, win->axes.ymid, win->axes.dp);

		LL_FOR_ALL(graph, &win->graph_list, prev, next)
		{
			stringbuf_puts(str, "\n\tgraph ");
			stringbuf_puts(str, "frag");
			stringbuf_putc(str, ' ');
			stringbuf_append(str, &graph->name);
			stringbuf_puts(str, "\n\t{\n");

			LL_FOR_ALL(view, &graph->params, prev, next)
			{
				stringbuf_puts(str, "\t\tparam ");

				stringbuf_append(str, &view->name);
				stringbuf_puts(str, " = ");
				stringbuf_append(str, &view->param->name);
				stringbuf_puts(str, "\n");
			}

			if (graph->params.next != &graph->params) // if any params
			{
				stringbuf_putc(str, '\n');
			}

			stringbuf_puts(str, "\t\tcolor #");
			stringbuf_puthex(str, graph->color[0]*255.0); // TODO: clamp these
			stringbuf_puthex(str, graph->color[1]*255.0);
			stringbuf_puthex(str, graph->color[2]*255.0);
			stringbuf_puthex(str, graph->color[3]*255.0);
			stringbuf_puts(str, "\n");

			if (graph->hsv)
			{
				stringbuf_puts(str, "\t\thsv ");
				stringbuf_putc(str, '0' + graph->hsv);
				stringbuf_puts(str, "\n");
			}

			stringbuf_puts(str, "\n");

			if (!graph->enable)
			{
				stringbuf_puts(str, "\t\tenable ");
				stringbuf_putc(str, '0' + graph->enable);
				stringbuf_puts(str, "\n\n");
			}

			stringbuf_puts(str, "\t\tdock ");
			stringbuf_putc(str, '0' + graph->dock);
			stringbuf_puts(str, "\n\n");

			stringbuf_puts(str, "\t\tshader ");
			stringbuf_puts(str, "frag");
			stringbuf_puts(str, "\n\t\t{\n");

			// indent shader code
			struct stringslice eqn = stringslice_new_str(graph->eqn);
			while (stringslice_ok(&eqn))
			{
				struct stringslice line = stringslice_match_line(&eqn);
				stringslice_match(&eqn, "\r");
				stringslice_match(&eqn, "\n");

				stringbuf_puts(str, "\t\t\t");
				stringbuf_append_stringslice(str, line);
				stringbuf_putc(str, '\n');
			}

			stringbuf_puts(str, "\t\t}\n");

			stringbuf_puts(str, "\t}\n");
		}

		stringbuf_puts(str, "}\n");
	}
}

void session_save_file(FILE *fp)
{
	struct stringbuf str;
	stringbuf_new_prealloc_at(&str, 4096);
	session_save_serialize(&str);

	stringbuf_file_write(&str, fp);

	stringbuf_dtor(&str);
}

struct stringbuf session_path = {0};

int session_save_path(const char *path)
{
	stringbuf_reset(&session_path);
	stringbuf_puts(&session_path, path);

	FILE *fp = fopen(path, "w+");
	if (fp == NULL)
	{
		fprintf(stderr, "failed to save %s\n", path);
		return 1;
	}

	session_save_file(fp);
	fclose(fp);

	return 0;
}

int session_save_curr(void)
{
	if (session_path.n == 0) return 1;

	return session_save_path(stringbuf_get(&session_path));
}
