/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * diapagelayout.[ch] -- a page settings widget for dia.
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

/*#define PAGELAYOUT_TEST*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "diapagelayout.h"
#include "diaunitspinner.h"

#include "intl.h"

#include "pixmaps/portrait.xpm"
#include "pixmaps/landscape.xpm"

#include "paper.h"

enum {
  CHANGED,
  LAST_SIGNAL
};

static guint pl_signals[LAST_SIGNAL] = { 0 };
static GtkObjectClass *parent_class;

static void dia_page_layout_class_init(DiaPageLayoutClass *class);
static void dia_page_layout_init(DiaPageLayout *self);
static void dia_page_layout_destroy(GtkObject *object);

GtkType
dia_page_layout_get_type(void)
{
  static GtkType pl_type = 0;

  if (!pl_type) {
    GtkTypeInfo pl_info = {
      "DiaPageLayout",
      sizeof(DiaPageLayout),
      sizeof(DiaPageLayoutClass),
      (GtkClassInitFunc) dia_page_layout_class_init,
      (GtkObjectInitFunc) dia_page_layout_init,
      (GtkArgSetFunc) NULL,
      (GtkArgGetFunc) NULL
    };
    pl_type = gtk_type_unique(gtk_table_get_type(), &pl_info);
  }
  return pl_type;
}

static void
dia_page_layout_class_init(DiaPageLayoutClass *class)
{
  GtkObjectClass *object_class;
  
  object_class = (GtkObjectClass*) class;
  parent_class = gtk_type_class(gtk_table_get_type());

  pl_signals[CHANGED] =
    gtk_signal_new("changed",
		   GTK_RUN_FIRST,
		   object_class->type,
		   GTK_SIGNAL_OFFSET(DiaPageLayoutClass, changed),
		   gtk_signal_default_marshaller,
		   GTK_TYPE_NONE, 0);
  gtk_object_class_add_signals(object_class, pl_signals, LAST_SIGNAL);

  object_class->destroy = dia_page_layout_destroy;
}

static void darea_size_allocate(DiaPageLayout *self, GtkAllocation *alloc);
static gint darea_expose_event(DiaPageLayout *self, GdkEventExpose *ev);
static void paper_size_change(GtkMenuItem *item, DiaPageLayout *self);
static void orient_changed(DiaPageLayout *self);
static void margin_changed(DiaPageLayout *self);
static void scalemode_changed(DiaPageLayout *self);
static void scale_changed(DiaPageLayout *self);

static void
dia_page_layout_init(DiaPageLayout *self)
{
  GtkWidget *frame, *box, *table, *menu, *menuitem, *wid;
  GdkPixmap *pix;
  GdkBitmap *mask;
  GList *paper_names;
  gint i;

  gtk_table_resize(GTK_TABLE(self), 3, 2);
  gtk_table_set_row_spacings(GTK_TABLE(self), 5);
  gtk_table_set_col_spacings(GTK_TABLE(self), 5);

  /* paper size */
  frame = gtk_frame_new(_("Paper Size"));
  gtk_table_attach(GTK_TABLE(self), frame, 0,1, 0,1,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(frame);

  box = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(box), 5);
  gtk_container_add(GTK_CONTAINER(frame), box);
  gtk_widget_show(box);

  self->paper_size = gtk_option_menu_new();
  gtk_box_pack_start(GTK_BOX(box), self->paper_size, TRUE, FALSE, 0);

  menu = gtk_menu_new();

  paper_names = get_paper_name_list();  
  for (i = 0; paper_names != NULL; 
       i++, paper_names = g_list_next(paper_names)) {
    menuitem = gtk_menu_item_new_with_label(paper_names->data);
    gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(i));
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
		       GTK_SIGNAL_FUNC(paper_size_change), self);
    gtk_container_add(GTK_CONTAINER(menu), menuitem);
    gtk_widget_show(menuitem);
  }
  gtk_option_menu_set_menu(GTK_OPTION_MENU(self->paper_size), menu);
  gtk_widget_show(self->paper_size);

  self->paper_label = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(box), self->paper_label, TRUE, TRUE, 0);
  gtk_widget_show(self->paper_label);

  /* orientation */
  frame = gtk_frame_new(_("Orientation"));
  gtk_table_attach(GTK_TABLE(self), frame, 1,2, 0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show(frame);

  box = gtk_hbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(box), 5);
  gtk_container_add(GTK_CONTAINER(frame), box);
  gtk_widget_show(box);

  self->orient_portrait = gtk_radio_button_new(NULL);
  pix = gdk_pixmap_colormap_create_from_xpm_d(NULL,
		gtk_widget_get_colormap(GTK_WIDGET(self)), &mask, NULL,
		portrait_xpm);
  wid = gtk_pixmap_new(pix, mask);
  gdk_pixmap_unref(pix);
  gdk_bitmap_unref(mask);
  gtk_container_add(GTK_CONTAINER(self->orient_portrait), wid);
  gtk_widget_show(wid);

  gtk_box_pack_start(GTK_BOX(box), self->orient_portrait, TRUE, TRUE, 0);
  gtk_widget_show(self->orient_portrait);

  self->orient_landscape = gtk_radio_button_new(
	gtk_radio_button_group(GTK_RADIO_BUTTON(self->orient_portrait)));
  pix = gdk_pixmap_colormap_create_from_xpm_d(NULL,
		gtk_widget_get_colormap(GTK_WIDGET(self)), &mask, NULL,
		landscape_xpm);
  wid = gtk_pixmap_new(pix, mask);
  gdk_pixmap_unref(pix);
  gdk_bitmap_unref(mask);
  gtk_container_add(GTK_CONTAINER(self->orient_landscape), wid);
  gtk_widget_show(wid);

  gtk_box_pack_start(GTK_BOX(box), self->orient_landscape, TRUE, TRUE, 0);
  gtk_widget_show(self->orient_landscape);

  /* margins */
  frame = gtk_frame_new(_("Margins"));
  gtk_table_attach(GTK_TABLE(self), frame, 0,1, 1,2,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(frame);

  table = gtk_table_new(4, 2, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_container_add(GTK_CONTAINER(frame), table);
  gtk_widget_show(table);

  wid = gtk_label_new(_("Top:"));
  gtk_misc_set_alignment(GTK_MISC(wid), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), wid, 0,1, 0,1,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(wid);

  self->tmargin = dia_unit_spinner_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 0,100, 0.1,10,10)),
	2, DIA_UNIT_CENTIMETER);
  gtk_table_attach(GTK_TABLE(table), self->tmargin, 1,2, 0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->tmargin);

  wid = gtk_label_new(_("Bottom:"));
  gtk_misc_set_alignment(GTK_MISC(wid), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), wid, 0,1, 1,2,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(wid);

  self->bmargin = dia_unit_spinner_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 0,100, 0.1,10,10)),
	2, DIA_UNIT_CENTIMETER);
  gtk_table_attach(GTK_TABLE(table), self->bmargin, 1,2, 1,2,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->bmargin);

  wid = gtk_label_new(_("Left:"));
  gtk_misc_set_alignment(GTK_MISC(wid), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), wid, 0,1, 2,3,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(wid);

  self->lmargin = dia_unit_spinner_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 0,100, 0.1,10,10)),
	2, DIA_UNIT_CENTIMETER);
  gtk_table_attach(GTK_TABLE(table), self->lmargin, 1,2, 2,3,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->lmargin);

  wid = gtk_label_new(_("Right:"));
  gtk_misc_set_alignment(GTK_MISC(wid), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), wid, 0,1, 3,4,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(wid);

  self->rmargin = dia_unit_spinner_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 0,100, 0.1,10,10)),
	2, DIA_UNIT_CENTIMETER);
  gtk_table_attach(GTK_TABLE(table), self->rmargin, 1,2, 3,4,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->rmargin);

  /* Scaling */
  frame = gtk_frame_new(_("Scaling"));
  gtk_table_attach(GTK_TABLE(self), frame, 0,1, 2,3,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(frame);

  table = gtk_table_new(2, 4, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_container_add(GTK_CONTAINER(frame), table);
  gtk_widget_show(table);

  self->scale = gtk_radio_button_new_with_label(NULL, _("Scale:"));
  gtk_table_attach(GTK_TABLE(table), self->scale, 0,1, 0,1,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->scale);

  self->scaling = gtk_spin_button_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(100,1,10000, 1,10,10)), 1, 1);
  gtk_table_attach(GTK_TABLE(table), self->scaling, 1,4, 0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->scaling);

  self->fitto = gtk_radio_button_new_with_label(
			GTK_RADIO_BUTTON(self->scale)->group, _("Fit to:"));
  gtk_table_attach(GTK_TABLE(table), self->fitto, 0,1, 1,2,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->fitto);

  self->fitw = gtk_spin_button_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 1, 1000, 1, 10, 10)), 1, 0);
  gtk_widget_set_sensitive(self->fitw, FALSE);
  gtk_table_attach(GTK_TABLE(table), self->fitw, 1,2, 1,2,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->fitw);

  wid = gtk_label_new(_("by"));
  gtk_misc_set_padding(GTK_MISC(wid), 5, 0);
  gtk_table_attach(GTK_TABLE(table), wid, 2,3, 1,2,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(wid);

  self->fith = gtk_spin_button_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 1, 1000, 1, 10, 10)), 1, 0);
  gtk_widget_set_sensitive(self->fith, FALSE);
  gtk_table_attach(GTK_TABLE(table), self->fith, 3,4, 1,2,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->fith);

  /* the drawing area */
  self->darea = gtk_drawing_area_new();
  gtk_table_attach(GTK_TABLE(self), self->darea, 1,2, 1,3,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->darea);

  /* connect the signal handlers */
  gtk_signal_connect_object(GTK_OBJECT(self->orient_portrait), "toggled",
			    GTK_SIGNAL_FUNC(orient_changed), GTK_OBJECT(self));

  gtk_signal_connect_object(GTK_OBJECT(self->tmargin), "changed",
			    GTK_SIGNAL_FUNC(margin_changed), GTK_OBJECT(self));
  gtk_signal_connect_object(GTK_OBJECT(self->bmargin), "changed",
			    GTK_SIGNAL_FUNC(margin_changed), GTK_OBJECT(self));
  gtk_signal_connect_object(GTK_OBJECT(self->lmargin), "changed",
			    GTK_SIGNAL_FUNC(margin_changed), GTK_OBJECT(self));
  gtk_signal_connect_object(GTK_OBJECT(self->rmargin), "changed",
			    GTK_SIGNAL_FUNC(margin_changed), GTK_OBJECT(self));

  gtk_signal_connect_object(GTK_OBJECT(self->fitto), "toggled",
			    GTK_SIGNAL_FUNC(scalemode_changed),
			    GTK_OBJECT(self));
  gtk_signal_connect_object(GTK_OBJECT(self->scaling), "changed",
			    GTK_SIGNAL_FUNC(scale_changed), GTK_OBJECT(self));
  gtk_signal_connect_object(GTK_OBJECT(self->fitw), "changed",
			    GTK_SIGNAL_FUNC(scale_changed), GTK_OBJECT(self));
  gtk_signal_connect_object(GTK_OBJECT(self->fith), "changed",
			    GTK_SIGNAL_FUNC(scale_changed), GTK_OBJECT(self));

  gtk_signal_connect_object(GTK_OBJECT(self->darea), "size_allocate",
			    GTK_SIGNAL_FUNC(darea_size_allocate),
			    GTK_OBJECT(self));
  gtk_signal_connect_object(GTK_OBJECT(self->darea), "expose_event",
			    GTK_SIGNAL_FUNC(darea_expose_event),
			    GTK_OBJECT(self));

  gdk_color_white(gtk_widget_get_colormap(GTK_WIDGET(self)), &self->white);
  gdk_color_black(gtk_widget_get_colormap(GTK_WIDGET(self)), &self->black);
  self->blue.red = 0;
  self->blue.green = 0;
  self->blue.blue = 0x7fff;
  gdk_color_alloc(gtk_widget_get_colormap(GTK_WIDGET(self)), &self->blue);

  self->gc = NULL;
  self->block_changed = FALSE;
}

GtkWidget *
dia_page_layout_new(void)
{
  DiaPageLayout *self = gtk_type_new(dia_page_layout_get_type());

  dia_page_layout_set_paper(self, "");
  return GTK_WIDGET(self);
}

const gchar *
dia_page_layout_get_paper(DiaPageLayout *self)
{
  return get_paper_name(self->papernum);
}

void
dia_page_layout_set_paper(DiaPageLayout *self, const gchar *paper)
{
  gint i;

  i = find_paper(paper);
  if (i == -1)
    i = get_default_paper();
  gtk_option_menu_set_history(GTK_OPTION_MENU(self->paper_size), i);
  gtk_menu_item_activate(
	GTK_MENU_ITEM(GTK_OPTION_MENU(self->paper_size)->menu_item));
}

void
dia_page_layout_get_margins(DiaPageLayout *self,
			    gfloat *tmargin, gfloat *bmargin,
			    gfloat *lmargin, gfloat *rmargin)
{
  if (tmargin)
    *tmargin = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->tmargin));
  if (bmargin)
    *bmargin = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->bmargin));
  if (lmargin)
    *lmargin = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->lmargin));
  if (rmargin)
    *rmargin = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->rmargin));
}

void
dia_page_layout_set_margins(DiaPageLayout *self,
			    gfloat tmargin, gfloat bmargin,
			    gfloat lmargin, gfloat rmargin)
{
  self->block_changed = TRUE;
  dia_unit_spinner_set_value(DIA_UNIT_SPINNER(self->tmargin), tmargin);
  dia_unit_spinner_set_value(DIA_UNIT_SPINNER(self->bmargin), bmargin);
  dia_unit_spinner_set_value(DIA_UNIT_SPINNER(self->lmargin), lmargin);
  dia_unit_spinner_set_value(DIA_UNIT_SPINNER(self->rmargin), rmargin);
  self->block_changed = FALSE;

  gtk_signal_emit(GTK_OBJECT(self), pl_signals[CHANGED]);
}

DiaPageOrientation
dia_page_layout_get_orientation(DiaPageLayout *self)
{
  if (GTK_TOGGLE_BUTTON(self->orient_portrait)->active)
    return DIA_PAGE_ORIENT_PORTRAIT;
  else
    return DIA_PAGE_ORIENT_LANDSCAPE;
}

void
dia_page_layout_set_orientation(DiaPageLayout *self,
				DiaPageOrientation orient)
{
  switch (orient) {
  case DIA_PAGE_ORIENT_PORTRAIT:
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->orient_portrait),
				 TRUE);
    break;
  case DIA_PAGE_ORIENT_LANDSCAPE:
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->orient_landscape),
				 TRUE);
    break;
  }
}

gboolean
dia_page_layout_get_fitto(DiaPageLayout *self)
{
  return GTK_TOGGLE_BUTTON(self->fitto)->active;
}

void
dia_page_layout_set_fitto(DiaPageLayout *self, gboolean fitto)
{
  if (fitto)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->fitto), TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->scale), TRUE);
}

gfloat
dia_page_layout_get_scaling(DiaPageLayout *self)
{
  return GTK_SPIN_BUTTON(self->scaling)->adjustment->value / 100.0;
}

void
dia_page_layout_set_scaling(DiaPageLayout *self, gfloat scaling)
{
  GTK_SPIN_BUTTON(self->scaling)->adjustment->value = scaling * 100.0;
  gtk_adjustment_value_changed(GTK_SPIN_BUTTON(self->scaling)->adjustment);
}

void
dia_page_layout_get_fit_dims(DiaPageLayout *self, gint *w, gint *h)
{
  if (w)
    *w = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(self->fitw));
  if (h)
    *h = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(self->fith));
}

void
dia_page_layout_set_fit_dims(DiaPageLayout *self, gint w, gint h)
{
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(self->fitw), w);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(self->fith), h);
}

void
dia_page_layout_get_effective_area(DiaPageLayout *self, gfloat *width,
				   gfloat *height)
{
  gfloat h, w, scaling;

  if (GTK_TOGGLE_BUTTON(self->orient_portrait)->active) {
    w = get_paper_pswidth(self->papernum);
    h = get_paper_psheight(self->papernum);
  } else {
    h = get_paper_pswidth(self->papernum);
    w = get_paper_psheight(self->papernum);
  }
  h -= dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->tmargin));
  h -= dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->bmargin));
  w -= dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->lmargin));
  w -= dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->rmargin));
  scaling = GTK_SPIN_BUTTON(self->scaling)->adjustment->value / 100.0;
  h /= scaling;
  w /= scaling;

  if (width)  *width = w;
  if (height) *height = h;
}

void
dia_page_layout_get_paper_size(const gchar *paper,
			       gfloat *width, gfloat *height)
{
  gint i;

  i = find_paper(paper);
  if (i == -1)
    i = get_default_paper();
  if (width)
    *width = get_paper_pswidth(i);
  if (height)
    *height = get_paper_psheight(i);
}

void
dia_page_layout_get_default_margins(const gchar *paper,
				    gfloat *tmargin, gfloat *bmargin,
				    gfloat *lmargin, gfloat *rmargin)
{
  gint i;

  i = find_paper(paper);
  if (i == -1)
    i = get_default_paper();
  if (tmargin)
    *tmargin = get_paper_tmargin(i);
  if (bmargin)
    *bmargin = get_paper_bmargin(i);
  if (lmargin)
    *lmargin = get_paper_lmargin(i);
  if (rmargin)
    *rmargin = get_paper_rmargin(i);
}

static void
size_page(DiaPageLayout *self, GtkAllocation *a)
{
  self->width = a->width - 3;
  self->height = a->height - 3;

  /* change to correct metrics */
  if (GTK_TOGGLE_BUTTON(self->orient_portrait)->active) {
    if (self->width * get_paper_psheight(self->papernum) >
	self->height * get_paper_pswidth(self->papernum))
      self->width = self->height * get_paper_pswidth(self->papernum) /
	get_paper_psheight(self->papernum);
    else
      self->height = self->width * get_paper_psheight(self->papernum) /
	get_paper_pswidth(self->papernum);
  } else {
    if (self->width * get_paper_pswidth(self->papernum) >
	self->height * get_paper_psheight(self->papernum))
      self->width = self->height * get_paper_psheight(self->papernum) /
	get_paper_pswidth(self->papernum);
    else
      self->height = self->width * get_paper_pswidth(self->papernum) /
	get_paper_psheight(self->papernum);
  }

  self->x = (a->width - self->width - 3) / 2;
  self->y = (a->height - self->height - 3) / 2;
}

static void
darea_size_allocate(DiaPageLayout *self, GtkAllocation *allocation)
{
  size_page(self, allocation);
}

static gint
darea_expose_event(DiaPageLayout *self, GdkEventExpose *event)
{
  GdkWindow *window= self->darea->window;
  gfloat val;
  gint num;

  if (!window)
    return FALSE;

  if (!self->gc)
    self->gc = gdk_gc_new(window);

  gdk_window_clear_area (window,
                         0, 0,
                         self->darea->allocation.width,
                         self->darea->allocation.height);

  /* draw the page image */
  gdk_gc_set_foreground(self->gc, &self->black);
  gdk_draw_rectangle(window, self->gc, TRUE, self->x+3, self->y+3,
		     self->width, self->height);
  gdk_gc_set_foreground(self->gc, &self->white);
  gdk_draw_rectangle(window, self->gc, TRUE, self->x, self->y,
		     self->width, self->height);
  gdk_gc_set_foreground(self->gc, &self->black);
  gdk_draw_rectangle(window, self->gc, FALSE, self->x, self->y,
		     self->width-1, self->height-1);

  gdk_gc_set_foreground(self->gc, &self->blue);

  /* draw margins */
  if (GTK_TOGGLE_BUTTON(self->orient_portrait)->active) {
    val = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->tmargin));
    num = self->y + val * self->height /get_paper_psheight(self->papernum);
    gdk_draw_line(window, self->gc, self->x+1, num, self->x+self->width-2,num);

    val = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->bmargin));
    num = self->y + self->height -
      val * self->height / get_paper_psheight(self->papernum);
    gdk_draw_line(window, self->gc, self->x+1, num, self->x+self->width-2,num);

    val = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->lmargin));
    num = self->x + val * self->width / get_paper_pswidth(self->papernum);
    gdk_draw_line(window, self->gc, num, self->y+1,num,self->y+self->height-2);

    val = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->rmargin));
    num = self->x + self->width -
      val * self->width / get_paper_pswidth(self->papernum);
    gdk_draw_line(window, self->gc, num, self->y+1,num,self->y+self->height-2);
  } else {
    val = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->tmargin));
    num = self->y + val * self->height /get_paper_pswidth(self->papernum);
    gdk_draw_line(window, self->gc, self->x+1, num, self->x+self->width-2,num);

    val = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->bmargin));
    num = self->y + self->height -
      val * self->height / get_paper_pswidth(self->papernum);
    gdk_draw_line(window, self->gc, self->x+1, num, self->x+self->width-2,num);

    val = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->lmargin));
    num = self->x + val * self->width / get_paper_psheight(self->papernum);
    gdk_draw_line(window, self->gc, num, self->y+1,num,self->y+self->height-2);

    val = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->rmargin));
    num = self->x + self->width -
      val * self->width / get_paper_psheight(self->papernum);
    gdk_draw_line(window, self->gc, num, self->y+1,num,self->y+self->height-2);
  }

  return FALSE;
}

static void
paper_size_change(GtkMenuItem *item, DiaPageLayout *self)
{
  gchar buf[512];

  self->papernum = GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(item)));
  size_page(self, &self->darea->allocation);
  gtk_widget_queue_draw(self->darea);

  self->block_changed = TRUE;
  dia_unit_spinner_set_value(DIA_UNIT_SPINNER(self->tmargin),
			     get_paper_tmargin(self->papernum));
  dia_unit_spinner_set_value(DIA_UNIT_SPINNER(self->bmargin),
			     get_paper_bmargin(self->papernum));
  dia_unit_spinner_set_value(DIA_UNIT_SPINNER(self->lmargin),
			     get_paper_lmargin(self->papernum));
  dia_unit_spinner_set_value(DIA_UNIT_SPINNER(self->rmargin),
			     get_paper_rmargin(self->papernum));

  if (GTK_TOGGLE_BUTTON(self->orient_portrait)->active) {
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->tmargin))->upper =
      get_paper_psheight(self->papernum);
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->bmargin))->upper =
      get_paper_psheight(self->papernum);
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->lmargin))->upper =
      get_paper_pswidth(self->papernum);
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->rmargin))->upper =
      get_paper_pswidth(self->papernum);
  } else {
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->tmargin))->upper =
      get_paper_pswidth(self->papernum);
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->bmargin))->upper =
      get_paper_pswidth(self->papernum);
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->lmargin))->upper =
      get_paper_psheight(self->papernum);
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->rmargin))->upper =
      get_paper_psheight(self->papernum);
  }
  self->block_changed = FALSE;

  g_snprintf(buf, sizeof(buf), _("%0.3gcm x %0.3gcm"),
	     get_paper_pswidth(self->papernum),
	     get_paper_psheight(self->papernum));
  gtk_label_set(GTK_LABEL(self->paper_label), buf);

  gtk_signal_emit(GTK_OBJECT(self), pl_signals[CHANGED]);
}

static void
orient_changed(DiaPageLayout *self)
{
  size_page(self, &self->darea->allocation);
  gtk_widget_queue_draw(self->darea);

  if (GTK_TOGGLE_BUTTON(self->orient_portrait)->active) {
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->tmargin))->upper =
      get_paper_psheight(self->papernum);
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->bmargin))->upper =
      get_paper_psheight(self->papernum);
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->lmargin))->upper =
      get_paper_pswidth(self->papernum);
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->rmargin))->upper =
      get_paper_pswidth(self->papernum);
  } else {
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->tmargin))->upper =
      get_paper_pswidth(self->papernum);
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->bmargin))->upper =
      get_paper_pswidth(self->papernum);
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->lmargin))->upper =
      get_paper_psheight(self->papernum);
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->rmargin))->upper =
      get_paper_psheight(self->papernum);
  }

  if (!self->block_changed)
    gtk_signal_emit(GTK_OBJECT(self), pl_signals[CHANGED]);
}

static void
margin_changed(DiaPageLayout *self)
{
  gtk_widget_queue_draw(self->darea);
  if (!self->block_changed)
    gtk_signal_emit(GTK_OBJECT(self), pl_signals[CHANGED]);
}

static void
scalemode_changed(DiaPageLayout *self)
{
  if (GTK_TOGGLE_BUTTON(self->fitto)->active) {
    gtk_widget_set_sensitive(self->scaling, FALSE);
    gtk_widget_set_sensitive(self->fitw, TRUE);
    gtk_widget_set_sensitive(self->fith, TRUE);
  } else {
    gtk_widget_set_sensitive(self->scaling, TRUE);
    gtk_widget_set_sensitive(self->fitw, FALSE);
    gtk_widget_set_sensitive(self->fith, FALSE);
  }
  if (!self->block_changed)
    gtk_signal_emit(GTK_OBJECT(self), pl_signals[CHANGED]);
}

static void
scale_changed(DiaPageLayout *self)
{
  if (!self->block_changed)
    gtk_signal_emit(GTK_OBJECT(self), pl_signals[CHANGED]);
}

static void
dia_page_layout_destroy(GtkObject *object)
{
  DiaPageLayout *self = DIA_PAGE_LAYOUT(object);

  if (self->gc)
    gdk_gc_unref(self->gc);

  if (parent_class->destroy)
    (* parent_class->destroy)(object);
}

#ifdef PAGELAYOUT_TEST

void
changed_signal(DiaPageLayout *self)
{
  g_message("changed");
}
void
fittopage_signal(DiaPageLayout *self)
{
  g_message("fit to page");
}

void
main(int argc, char **argv)
{
  GtkWidget *win, *pl;

  gtk_init(&argc, &argv);

  win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(win), _("Page Setup"));
  gtk_signal_connect(GTK_OBJECT(win), "destroy",
		     GTK_SIGNAL_FUNC(gtk_main_quit), NULL);

  pl = dia_page_layout_new();
  gtk_container_set_border_width(GTK_CONTAINER(pl), 5);
  gtk_container_add(GTK_CONTAINER(win), pl);
  gtk_widget_show(pl);

  gtk_signal_connect(GTK_OBJECT(pl), "changed",
		     GTK_SIGNAL_FUNC(changed_signal), NULL);
  gtk_signal_connect(GTK_OBJECT(pl), "fittopage",
		     GTK_SIGNAL_FUNC(fittopage_signal), NULL);

  gtk_widget_show(win);
  gtk_main();
}

#endif
