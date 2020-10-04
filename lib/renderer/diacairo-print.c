/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diacairo-print.c -- Cairo/gtk+ based printing for dia
 * Copyright (C) 2008, Hans Breuer, <Hans@Breuer.Org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "diacairo.h"
#include "diacairo-print.h"

#include "message.h"

typedef struct _PrintData
{
  DiagramData *data;
  DiaRenderer *renderer;
} PrintData;

G_GNUC_UNUSED static void
count_objs(DiaObject *obj, DiaRenderer *renderer, int active_layer, guint *nobjs)
{
  (*nobjs)++;
}

/* Dia has it's own thing */
static void
_dia_to_gtk_page_setup (const DiagramData *data, GtkPageSetup *setup)
{
  GtkPaperSize *paper_size;
  const double points_per_cm = 28.346457;
  const PaperInfo *paper = &(data->paper);
  int index = find_paper (paper->name);
  if (index < 0)
    index = get_default_paper ();
  paper_size = gtk_paper_size_new_from_ppd (
		 paper->name, paper->name,
                 get_paper_pswidth (index) * points_per_cm,
		 get_paper_psheight (index) * points_per_cm);

  gtk_page_setup_set_orientation (setup, data->paper.is_portrait ?
    GTK_PAGE_ORIENTATION_PORTRAIT : GTK_PAGE_ORIENTATION_LANDSCAPE);
  gtk_page_setup_set_paper_size (setup, paper_size);

  gtk_page_setup_set_left_margin (setup, data->paper.lmargin * 10, GTK_UNIT_MM);
  gtk_page_setup_set_top_margin (setup, data->paper.tmargin * 10, GTK_UNIT_MM);
  gtk_page_setup_set_right_margin (setup, data->paper.rmargin * 10, GTK_UNIT_MM);
  gtk_page_setup_set_bottom_margin (setup, data->paper.bmargin * 10, GTK_UNIT_MM);

}

static void
begin_print (GtkPrintOperation *operation,
             GtkPrintContext   *context,
             PrintData         *print_data)
{
  DiaCairoRenderer *cairo_renderer;
  g_return_if_fail (print_data->renderer != NULL);
  cairo_renderer = DIA_CAIRO_RENDERER (print_data->renderer);
  g_return_if_fail (cairo_renderer->cr == NULL);

  /* the renderer wants it's own reference */
#if 0 /* no alpha with printers */
  cairo_renderer->with_alpha = TRUE;
#endif
  cairo_renderer->cr = cairo_reference (gtk_print_context_get_cairo_context (context));
  cairo_renderer->dia = print_data->data;
#if 0 /* needs some text size scaling ... */
  cairo_renderer->layout = gtk_print_context_create_pango_layout (context);
#endif

  /* scaling - as usual I don't get it, or do I? */
  cairo_renderer->scale = (
      gtk_page_setup_get_paper_width (gtk_print_context_get_page_setup (context), GTK_UNIT_MM)
      - gtk_page_setup_get_left_margin( gtk_print_context_get_page_setup (context), GTK_UNIT_MM)
      - gtk_page_setup_get_right_margin( gtk_print_context_get_page_setup (context), GTK_UNIT_MM)
      ) / print_data->data->paper.width;
  cairo_renderer->skip_show_page = TRUE;
}

static void
draw_page (GtkPrintOperation *operation,
           GtkPrintContext   *context,
           int                page_nr,
           PrintData         *print_data)
{
  DiaRectangle bounds;
  DiagramData *data = print_data->data;
  int x, y;
  /* the effective sizes - dia already applied is_portrait */
  double dp_width = data->paper.width;
  double dp_height = data->paper.height;

  DiaCairoRenderer *cairo_renderer;
  g_return_if_fail (print_data->renderer != NULL);
  cairo_renderer = DIA_CAIRO_RENDERER (print_data->renderer);

  if (data->paper.fitto) {
    x = page_nr % data->paper.fitwidth;
    y = page_nr / data->paper.fitwidth;

    bounds.left = dp_width * x + data->extents.left;
    bounds.top = dp_height * y + data->extents.top;
    bounds.right = bounds.left + dp_width;
    bounds.bottom = bounds.top + dp_height;
  } else {
    double dx, dy;
    int nx = ceil((data->extents.right - data->extents.left) / dp_width);
    x = page_nr % nx;
    y = page_nr / nx;

    /* Respect the original pagination as shown by the page guides.
     * Caclulate the offset between page origin 0,0 and data.extents.topleft.
     * For the usual first page this boils down to lefttop=(0,0) but beware
     * the origin being negative.
     */
    dx = fmod(data->extents.left, dp_width);
    if (dx < 0.0)
      dx += dp_width;
    dy = fmod(data->extents.top, dp_height);
    if (dy < 0.0)
      dy += dp_height;

    bounds.left = dp_width * x + data->extents.left - dx;
    bounds.top = dp_height * y + data->extents.top - dy;
    bounds.right = bounds.left + dp_width;
    bounds.bottom = bounds.top + dp_height;
  }

#if 0 /* calls begin/end of the given renderer */
  /* count the number of objects in this region */
  data_render(data, print_data->renderer, &bounds,
              (ObjectRenderer) count_objs, &nobjs);
  if (!nobjs)
    return; /* not printing empty pages */
#endif

  /* setup a clipping rect */
  {
    GtkPageSetup *setup = gtk_print_context_get_page_setup (context);
    double left = gtk_page_setup_get_left_margin (setup, GTK_UNIT_MM);
    double top = gtk_page_setup_get_top_margin (setup, GTK_UNIT_MM);
    double width = gtk_page_setup_get_paper_width (setup, GTK_UNIT_MM)
		   - left - gtk_page_setup_get_right_margin (setup, GTK_UNIT_MM);
    double height = gtk_page_setup_get_paper_height (setup, GTK_UNIT_MM)
		    - top - gtk_page_setup_get_bottom_margin (setup, GTK_UNIT_MM);
    cairo_save (cairo_renderer->cr);
    /* we are still in the gtk-print coordinate system */
#if 1
    /* ... but apparently already transalted to top,left */
    top = left = 0;
#endif
    cairo_rectangle (cairo_renderer->cr, left, top, width, height);

    cairo_clip (cairo_renderer->cr);
  }

  {
    DiaRectangle extents = data->extents;

    data->extents = bounds;
    /* render only the region, FIXME: better way than modifying DiagramData ?  */
    data_render(data, print_data->renderer, &bounds, NULL, NULL);
    data->extents = extents;
  }
  cairo_restore (cairo_renderer->cr);
}


static void
end_print (GtkPrintOperation *operation,
           GtkPrintContext   *context,
           PrintData         *print_data)
{
  g_clear_object (&print_data->data);
  g_clear_object (&print_data->renderer);

  g_clear_pointer (&print_data, g_free);
}


GtkPrintOperation *
create_print_operation (DiagramData *data, const char *name)
{
  PrintData *print_data;
  GtkPrintOperation *operation;
  GtkPageSetup * setup;
  int num_pages;

  /* gets deleted in end_print */
  print_data = g_new0 (PrintData, 1);
  print_data->data = g_object_ref (data);
  print_data->renderer = g_object_new (DIA_CAIRO_TYPE_RENDERER, NULL);

  operation = gtk_print_operation_new ();

  gtk_print_operation_set_job_name (operation, name);

  setup = gtk_print_operation_get_default_page_setup (operation);
  if (!setup)
    setup = gtk_page_setup_new ();
  _dia_to_gtk_page_setup (print_data->data, setup);
  gtk_print_operation_set_default_page_setup (operation, setup);
  g_clear_object (&setup);

  /* similar logic draw_page() but we need to set the total pages in advance */
  if (data->paper.fitto) {
    num_pages = data->paper.fitwidth * data->paper.fitheight;
  } else {
    int nx = ceil((data->extents.right - data->extents.left) / data->paper.width);
    int ny = ceil((data->extents.bottom - data->extents.top) / data->paper.height);
    num_pages = nx * ny;
  }
  gtk_print_operation_set_n_pages (operation, num_pages);

  gtk_print_operation_set_unit (operation, GTK_UNIT_MM);

  g_signal_connect (operation, "draw_page", G_CALLBACK (draw_page), print_data);
  g_signal_connect (operation, "begin_print", G_CALLBACK (begin_print), print_data);
  g_signal_connect (operation, "end_print", G_CALLBACK (end_print), print_data);

  return operation;
}


DiaObjectChange *
cairo_print_callback (DiagramData *data,
                      const char  *filename,
                      guint        flags, /* further additions */
                      void        *user_data)
{
  GtkPrintOperation *op = create_print_operation (data, filename ? filename : "diagram");
  GtkPrintOperationResult res;
  GError *error = NULL;

  res = gtk_print_operation_run (op,
                                 GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
                                 NULL,
                                 &error);

  if (GTK_PRINT_OPERATION_RESULT_ERROR == res) {
    message_error ("%s", error->message);
    g_clear_error (&error);
  }

  return NULL;
}
