/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * paginate_gnomeprint.[ch] -- pagination code for the gnome-print backend
 * Copyright (C) 1999 James Henstridge
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

#include "diagram.h"
#include "diagramdata.h"
#include "libgnome/libgnome.h"
#include "paginate_gnomeprint.h"

#include <gnome.h>
#include <libgnomeprint/gnome-printer-dialog.h>

static void
count_objs(Object *obj, Renderer *renderer, int active_layer, guint *nobjs)
{
  (*nobjs)++;
}


static guint
print_page(DiagramData *data, RendererGPrint *rend, Rectangle *bounds,
	   gint xpos, gint ypos)
{
  guint nobjs = 0;
  gdouble tmargin = data->paper.tmargin, bmargin = data->paper.bmargin;
  gdouble lmargin = data->paper.lmargin, rmargin = data->paper.rmargin;
  gdouble scale = data->paper.scaling;
  char buf[256];

  /* count the number of objects in this region */
  data_render(data, (Renderer *)rend, bounds,
	      (ObjectRenderer) count_objs, &nobjs);

  if (nobjs == 0)
    return nobjs;

  /* give a name to this page */
  g_snprintf(buf, sizeof(buf), "%d,%d", xpos, ypos);
  gnome_print_beginpage(rend->ctx, buf);

  /* save print context */
  gnome_print_gsave(rend->ctx);

  /* transform coordinate system */
  if (data->paper.is_portrait) {
    gnome_print_scale(rend->ctx, 28.346457 * scale, -28.346457 * scale);
    gnome_print_translate(rend->ctx, lmargin/scale - bounds->left,
			  -bmargin/scale - bounds->bottom);
  } else {
    gnome_print_rotate(rend->ctx, 90);
    gnome_print_scale(rend->ctx, 28.346457 * scale, -28.346457 * scale);
    gnome_print_translate(rend->ctx, lmargin/scale - bounds->left,
			  tmargin/scale - bounds->top);
  }

  /* set up clip mask */
  gnome_print_newpath(rend->ctx);
  gnome_print_moveto(rend->ctx, bounds->left, bounds->top);
  gnome_print_lineto(rend->ctx, bounds->right, bounds->top);
  gnome_print_lineto(rend->ctx, bounds->right, bounds->bottom);
  gnome_print_lineto(rend->ctx, bounds->left, bounds->bottom);
  gnome_print_lineto(rend->ctx, bounds->left, bounds->top);
  gnome_print_clip(rend->ctx);

  /* render the region */
  data_render(data, (Renderer *)rend, bounds, NULL, NULL);

  /* restore print context */
  gnome_print_grestore(rend->ctx);

  /* print the page */
  gnome_print_showpage(rend->ctx);

  return nobjs;
}

void
paginate_gnomeprint(Diagram *dia, GnomePrintContext *ctx)
{
  RendererGPrint *rend;
  Rectangle *extents;
  gdouble width, height;
  gdouble x, y, initx, inity;
  gint xpos, ypos;
  guint nobjs = 0;

  rend = new_gnomeprint_renderer(dia, ctx);

  /* the usable area of the page */
  width = dia->data->paper.width;
  height = dia->data->paper.height;

  /* get extents, and make them multiples of width / height */
  extents = &dia->data->extents;
  initx = extents->left;
  inity = extents->top;
  /* make page boundaries align with origin */
  if (!dia->data->paper.fitto) {
    initx = floor(initx / width)  * width;
    inity = floor(inity / height) * height;
  }

  /* iterate through all the pages in the diagram */
  for (y = inity, ypos = 0; y < extents->bottom; y += height, ypos++)
    for (x = inity, xpos = 0; x < extents->right; x += width, xpos++) {
      Rectangle page_bounds;

      page_bounds.left = x;
      page_bounds.right = x + width;
      page_bounds.top = y;
      page_bounds.bottom = y + height;

      nobjs += print_page(dia->data, rend, &page_bounds, xpos, ypos);
    }

  free(rend);

  gnome_print_context_close(ctx);

  gtk_object_unref(GTK_OBJECT(ctx));
}
		    
void
diagram_print_gnome(Diagram *dia)
{
  GtkWidget *dialog, *printersel;
  int btn;
  GnomePrinter *printer;
  GnomePrintContext *ctx;

  dialog = gnome_dialog_new(_("Print Diagram"), GNOME_STOCK_BUTTON_OK,
			    GNOME_STOCK_BUTTON_CANCEL, NULL);

  printersel = gnome_printer_widget_new();
  gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dialog)->vbox), printersel,
		     TRUE, TRUE, 0);
  gtk_widget_show(printersel);

  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  btn = gnome_dialog_run(GNOME_DIALOG(dialog));
  if (btn < 0)
    return;

  /* cancel was pressed */
  if (btn == 1) {
    gtk_widget_destroy(dialog);
    return;
  }

  /* get the printer name */
  printer = gnome_printer_widget_get_printer(GNOME_PRINTER_WIDGET(printersel));

  ctx = gnome_print_context_new_with_paper_size(printer,dia->data->paper.name);

  if (ctx != NULL)
    paginate_gnomeprint(dia, ctx);
  else
    message_error(_("An error occured while creating the print context"));

  gtk_object_unref(GTK_OBJECT(printer));
  gtk_widget_destroy(dialog);
}
