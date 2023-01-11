/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * pagesetup.[ch] -- code for the page setup dialog
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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include <math.h>

#include "pagesetup.h"
#include "dia-page-layout.h"
#include "display.h"

typedef struct _PageSetup PageSetup;
struct _PageSetup {
  Diagram *dia;
  GtkWidget *window;
  GtkWidget *paper;
  gboolean changed;
};

static void pagesetup_changed  (GtkWidget *wid, PageSetup *ps);
static void pagesetup_apply    (GtkWidget *wid, PageSetup *ps);


static int
pagesetup_respond (GtkWidget *widget,
                   int        response_id,
                   gpointer   data)
{
  PageSetup *ps = (PageSetup *)data;

  if (   response_id == GTK_RESPONSE_APPLY
      || response_id == GTK_RESPONSE_OK) {
    if (ps->changed) {
      pagesetup_apply (widget, ps);
    }
  }

  if (response_id != GTK_RESPONSE_APPLY) {
    g_clear_object (&ps->dia);
    gtk_widget_destroy(ps->window);
  }

  return 0;
}


void
create_page_setup_dlg(Diagram *dia)
{
  PageSetup *ps;
  GtkWidget *vbox;

  ps = g_new (PageSetup, 1);
  ps->dia = g_object_ref (dia);
  ps->window = gtk_dialog_new_with_buttons (_("Page Setup"),
                                            GTK_WINDOW (ddisplay_active()->shell),
                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                            _("_Close"), GTK_RESPONSE_CLOSE,
                                            _("_Apply"), GTK_RESPONSE_APPLY,
                                            _("_OK"), GTK_RESPONSE_OK,
                                            NULL);
  gtk_dialog_set_default_response (GTK_DIALOG(ps->window), GTK_RESPONSE_OK);
  vbox = gtk_dialog_get_content_area (GTK_DIALOG(ps->window));
  gtk_dialog_set_response_sensitive (GTK_DIALOG(ps->window), GTK_RESPONSE_APPLY, FALSE);
  ps->changed = FALSE;

  g_signal_connect(G_OBJECT (ps->window), "response",
                   G_CALLBACK (pagesetup_respond), ps);

  /* destroy ps when the dialog is closed */
  g_datalist_set_data_full (&G_OBJECT (ps->window)->qdata, "pagesetup", ps, g_free);

  ps->paper = dia_page_layout_new();
  dia_page_layout_set_paper(DIA_PAGE_LAYOUT(ps->paper), dia->data->paper.name);
  dia_page_layout_set_margins(DIA_PAGE_LAYOUT(ps->paper),
			      dia->data->paper.tmargin,
			      dia->data->paper.bmargin,
			      dia->data->paper.lmargin,
			      dia->data->paper.rmargin);
  dia_page_layout_set_orientation(DIA_PAGE_LAYOUT(ps->paper),
	dia->data->paper.is_portrait ? DIA_PAGE_ORIENT_PORTRAIT :
				  DIA_PAGE_ORIENT_LANDSCAPE);
  dia_page_layout_set_scaling(DIA_PAGE_LAYOUT(ps->paper),
			      dia->data->paper.scaling);
  dia_page_layout_set_fitto(DIA_PAGE_LAYOUT(ps->paper),
			    dia->data->paper.fitto);
  dia_page_layout_set_fit_dims(DIA_PAGE_LAYOUT(ps->paper),
			       dia->data->paper.fitwidth,
			       dia->data->paper.fitheight);

  gtk_container_set_border_width(GTK_CONTAINER(ps->paper), 5);
  gtk_box_pack_start(GTK_BOX(vbox), ps->paper, TRUE, TRUE, 0);
  gtk_widget_show(ps->paper);

  g_signal_connect(G_OBJECT(ps->paper), "changed",
		   G_CALLBACK(pagesetup_changed), ps);

  gtk_widget_show(ps->window);
}


static void
pagesetup_changed (GtkWidget *wid, PageSetup *ps)
{
  double dwidth, dheight;
  double pwidth, pheight;
  double xscale, yscale;
  int fitw = 0, fith = 0;
  double cur_scaling;

  /* set sensitivity on apply button */
  gtk_dialog_set_response_sensitive (GTK_DIALOG (ps->window),
                                     GTK_RESPONSE_APPLY,
                                     TRUE);
  ps->changed = TRUE;

  dwidth  = ps->dia->data->extents.right - ps->dia->data->extents.left;
  dheight = ps->dia->data->extents.bottom - ps->dia->data->extents.top;

  if (dwidth <= 0.0 || dheight <= 0.0)
    return;

  dia_page_layout_set_changed (DIA_PAGE_LAYOUT (ps->paper), TRUE);

  cur_scaling = dia_page_layout_get_scaling (DIA_PAGE_LAYOUT (ps->paper));
  dia_page_layout_get_effective_area (DIA_PAGE_LAYOUT (ps->paper),
                                      &pwidth, &pheight);
  g_return_if_fail (pwidth > 0.0 && pheight > 0.0);
  pwidth *= cur_scaling;
  pheight *= cur_scaling;

  if (dia_page_layout_get_fitto (DIA_PAGE_LAYOUT (ps->paper))) {
    dia_page_layout_get_fit_dims (DIA_PAGE_LAYOUT (ps->paper), &fitw, &fith);
    xscale = fitw * pwidth / dwidth;
    yscale = fith * pheight / dheight;
    dia_page_layout_set_scaling (DIA_PAGE_LAYOUT (ps->paper),
                                 MIN (xscale, yscale));
  } else {
    fitw = ceil (dwidth * cur_scaling / pwidth);
    fith = ceil (dheight * cur_scaling / pheight);
    dia_page_layout_set_fit_dims (DIA_PAGE_LAYOUT (ps->paper), fitw, fith);
  }

  dia_page_layout_set_changed (DIA_PAGE_LAYOUT(ps->paper), FALSE);
}

/* This affects the actual setup.  It should only be called when
   something has in fact changed.
*/
static void
pagesetup_apply(GtkWidget *wid, PageSetup *ps)
{
  dia_mem_swap_change_new (ps->dia, &ps->dia->data->paper, sizeof(ps->dia->data->paper));
  undo_set_transactionpoint(ps->dia->undo);

  g_clear_pointer (&ps->dia->data->paper.name, g_free);
  ps->dia->data->paper.name =
    g_strdup (dia_page_layout_get_paper (DIA_PAGE_LAYOUT (ps->paper)));

  dia_page_layout_get_margins (DIA_PAGE_LAYOUT (ps->paper),
                               &ps->dia->data->paper.tmargin,
                               &ps->dia->data->paper.bmargin,
                               &ps->dia->data->paper.lmargin,
                               &ps->dia->data->paper.rmargin);

  ps->dia->data->paper.is_portrait =
    dia_page_layout_get_orientation(DIA_PAGE_LAYOUT(ps->paper)) ==
    DIA_PAGE_ORIENT_PORTRAIT;
  ps->dia->data->paper.scaling =
    dia_page_layout_get_scaling(DIA_PAGE_LAYOUT(ps->paper));

  ps->dia->data->paper.fitto = dia_page_layout_get_fitto(
					DIA_PAGE_LAYOUT(ps->paper));
  dia_page_layout_get_fit_dims(DIA_PAGE_LAYOUT(ps->paper),
			       &ps->dia->data->paper.fitwidth,
			       &ps->dia->data->paper.fitheight);

  dia_page_layout_get_effective_area(DIA_PAGE_LAYOUT(ps->paper),
				     &ps->dia->data->paper.width,
				     &ps->dia->data->paper.height);

  /* set sensitivity on apply button */
  gtk_dialog_set_response_sensitive(GTK_DIALOG(ps->window), GTK_RESPONSE_APPLY, FALSE);
  ps->changed = FALSE;


  /* update diagram -- this is needed to reposition page boundaries */
  diagram_set_modified(ps->dia, TRUE);
  diagram_add_update_all(ps->dia);
  diagram_flush(ps->dia);
}
