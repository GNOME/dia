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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef GNOME
#include <gnome.h>
#else
#include <gtk/gtk.h>
#endif

#include <math.h>

#include "pagesetup.h"
#include "intl.h"
#include "diapagelayout.h"

typedef struct _PageSetup PageSetup;
struct _PageSetup {
  Diagram *dia;
  GtkWidget *window;
  GtkWidget *paper;
  GtkWidget *ok, *apply, *close;
};

static void pagesetup_changed  (GtkWidget *wid, PageSetup *ps);
static void pagesetup_ok       (GtkWidget *wid, PageSetup *ps);
static void pagesetup_apply    (GtkWidget *wid, PageSetup *ps);
static void pagesetup_close    (GtkWidget *wid, PageSetup *ps);

void
create_page_setup_dlg(Diagram *dia)
{
  PageSetup *ps;
  GtkWidget *vbox;

  ps = g_new(PageSetup, 1);
  ps->dia = dia;
#ifdef GNOME
  ps->window = gnome_dialog_new(_("Page Setup"), GNOME_STOCK_BUTTON_OK,
				GNOME_STOCK_BUTTON_APPLY,
				GNOME_STOCK_BUTTON_CLOSE, NULL);
  vbox = GNOME_DIALOG(ps->window)->vbox;
  gnome_dialog_set_sensitive(GNOME_DIALOG(ps->window), 1, FALSE);
  gnome_dialog_set_default(GNOME_DIALOG(ps->window), 2);
#else
  ps->window = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(ps->window), _("Page Setup"));
  vbox = GTK_DIALOG(ps->window)->vbox;

  /* setup buttons */
  ps->ok = gtk_button_new_with_label(_("OK"));
  GTK_WIDGET_SET_FLAGS(ps->ok, GTK_CAN_DEFAULT);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(ps->window)->action_area),
		    ps->ok);
  gtk_widget_show(ps->ok);
  ps->apply = gtk_button_new_with_label(_("Apply"));
  GTK_WIDGET_SET_FLAGS(ps->apply, GTK_CAN_DEFAULT);
  gtk_widget_set_sensitive(ps->apply, FALSE);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(ps->window)->action_area),
		    ps->apply);
  gtk_widget_show(ps->apply);
  ps->close = gtk_button_new_with_label(_("Close"));
  GTK_WIDGET_SET_FLAGS(ps->close, GTK_CAN_DEFAULT);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(ps->window)->action_area),
		    ps->close);
  gtk_widget_grab_default(ps->close);
  gtk_widget_show(ps->close);
#endif

  /* destroy ps when the dialog is closed */
  gtk_object_set_data_full(GTK_OBJECT(ps->window), "pagesetup", ps,
			   (GtkDestroyNotify)g_free);

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

  gtk_signal_connect(GTK_OBJECT(ps->paper), "changed",
		     GTK_SIGNAL_FUNC(pagesetup_changed), ps);
#ifdef GNOME
  gnome_dialog_button_connect(GNOME_DIALOG(ps->window), 0,
			      GTK_SIGNAL_FUNC(pagesetup_ok), ps);
  gnome_dialog_button_connect(GNOME_DIALOG(ps->window), 1,
			      GTK_SIGNAL_FUNC(pagesetup_apply), ps);
  gnome_dialog_button_connect(GNOME_DIALOG(ps->window), 2,
			      GTK_SIGNAL_FUNC(pagesetup_close), ps);
#else
  gtk_signal_connect(GTK_OBJECT(ps->ok), "clicked",
		     GTK_SIGNAL_FUNC(pagesetup_ok), ps);
  gtk_signal_connect(GTK_OBJECT(ps->apply), "clicked",
		     GTK_SIGNAL_FUNC(pagesetup_apply), ps);
  gtk_signal_connect(GTK_OBJECT(ps->close), "clicked",
		     GTK_SIGNAL_FUNC(pagesetup_close), ps);
#endif

  gtk_widget_show(ps->window);
}

static void
pagesetup_changed(GtkWidget *wid, PageSetup *ps)
{
  gfloat dwidth, dheight;
  gfloat pwidth, pheight;
  gfloat xscale, yscale;
  gint fitw = 0, fith = 0;
  gfloat cur_scaling;

  /* set sensitivity on apply button */
#ifdef GNOME
  gnome_dialog_set_sensitive(GNOME_DIALOG(ps->window), 1, TRUE);
#else
  gtk_widget_set_sensitive(ps->apply, TRUE);
#endif

  dwidth  = ps->dia->data->extents.right - ps->dia->data->extents.left;
  dheight = ps->dia->data->extents.bottom - ps->dia->data->extents.top;

  if (dwidth <= 0.0 || dheight <= 0.0)
    return;

  DIA_PAGE_LAYOUT(ps->paper)->block_changed = TRUE;

  cur_scaling = dia_page_layout_get_scaling(DIA_PAGE_LAYOUT(ps->paper));
  dia_page_layout_get_effective_area(DIA_PAGE_LAYOUT(ps->paper),
				     &pwidth, &pheight);
  pwidth *= cur_scaling;
  pheight *= cur_scaling;

  if (dia_page_layout_get_fitto(DIA_PAGE_LAYOUT(ps->paper))) {
    dia_page_layout_get_fit_dims(DIA_PAGE_LAYOUT(ps->paper), &fitw, &fith);
    xscale = fitw * pwidth / dwidth;
    yscale = fith * pheight / dheight;
    dia_page_layout_set_scaling(DIA_PAGE_LAYOUT(ps->paper),
				MIN(xscale, yscale));
  } else {
    fitw = ceil(dwidth * cur_scaling / pwidth);
    fith = ceil(dheight * cur_scaling / pheight);
    dia_page_layout_set_fit_dims(DIA_PAGE_LAYOUT(ps->paper), fitw, fith);
  }

  DIA_PAGE_LAYOUT(ps->paper)->block_changed = FALSE;
}

static void
pagesetup_ok(GtkWidget *wid, PageSetup *ps)
{
  pagesetup_apply(wid, ps);
  pagesetup_close(wid, ps);
}

static void
pagesetup_apply(GtkWidget *wid, PageSetup *ps)
{
  g_free(ps->dia->data->paper.name);
  ps->dia->data->paper.name =
    g_strdup(dia_page_layout_get_paper(DIA_PAGE_LAYOUT(ps->paper)));
  dia_page_layout_get_margins(DIA_PAGE_LAYOUT(ps->paper),
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
#ifdef GNOME
  gnome_dialog_set_sensitive(GNOME_DIALOG(ps->window), 1, FALSE);
#else
  gtk_widget_set_sensitive(ps->apply, FALSE);
#endif
  /* update diagram -- this is needed to reposition page boundaries */
  diagram_add_update_all(ps->dia);
  diagram_flush(ps->dia);
}

static void
pagesetup_close(GtkWidget *wid, PageSetup *ps)
{
  gtk_widget_destroy(ps->window);
}
