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
#include "widgets.h"

#include "intl.h"

#include "pixmaps/portrait.xpm"
#include "pixmaps/landscape.xpm"

#include "preferences.h"
#include "paper.h"
#include "prefs.h"

#include "diamarshal.h"
#include "diaoptionmenu.h"

/* private class : noone wants to inherit and noone needs to mess with details */
#define DIA_PAGE_LAYOUT_CLASS(klass) G_TYPE_CHECK_CLASS_CAST(klass, dia_page_layout_get_type(), DiaPageLayoutClass)
#define DIA_IS_PAGE_LAYOUT(obj) G_TYPE_CHECK_INSTANCE_TYPE(obj, dia_page_layout_get_type())

typedef struct _DiaPageLayoutClass DiaPageLayoutClass;

struct _DiaPageLayout {
  GtkTable parent;

  /*<private>*/
  GtkWidget *paper_size, *paper_label;
  GtkWidget *orient_portrait, *orient_landscape;
  GtkWidget *tmargin, *bmargin, *lmargin, *rmargin;
  GtkWidget *scale, *fitto;
  GtkWidget *scaling, *fitw, *fith;

  GtkWidget *darea;

  GdkGC *gc;
  GdkColor white, black, blue;
  gint papernum; /* index into page_metrics array */

  /* position of paper preview */
  gint x, y, width, height;

  gboolean block_changed;
};

struct _DiaPageLayoutClass {
  GtkTableClass parent_class;

  void (*changed)(DiaPageLayout *pl);
};


enum {
  CHANGED,
  LAST_SIGNAL
};

static guint pl_signals[LAST_SIGNAL] = { 0 };
static GtkObjectClass *parent_class;

static void dia_page_layout_class_init(DiaPageLayoutClass *class);
static void dia_page_layout_init(DiaPageLayout *self);
static void dia_page_layout_destroy(GtkObject *object);

GType
dia_page_layout_get_type(void)
{
  static GType pl_type = 0;

  if (!pl_type) {
    static const GTypeInfo pl_info = {
      sizeof(DiaPageLayoutClass),
      NULL,		/* base_init */
      NULL,		/* base_finalize */
      (GClassInitFunc)dia_page_layout_class_init,
      NULL,		/* class_finalize */
      NULL,		/* class_data */
      sizeof(DiaPageLayout),
      0,		/* n_preallocs */
      (GInstanceInitFunc) dia_page_layout_init,
    };
    pl_type = g_type_register_static (gtk_table_get_type (), "DiaPageLayout", &pl_info, 0);
  }
  return pl_type;
}

static void
dia_page_layout_class_init(DiaPageLayoutClass *class)
{
  GtkObjectClass *object_class;
  
  object_class = (GtkObjectClass*) class;
  parent_class = g_type_class_peek_parent (class);

  pl_signals[CHANGED] =
    g_signal_new("changed",
		 G_TYPE_FROM_CLASS (object_class),
		 G_SIGNAL_RUN_FIRST,
		 G_STRUCT_OFFSET(DiaPageLayoutClass, changed),
		 NULL, NULL,
		 dia_marshal_VOID__VOID,
		 G_TYPE_NONE, 0);
#if 0 /* FIXME ?*/
  gtk_object_class_add_signals(object_class, pl_signals, LAST_SIGNAL);
#endif

  object_class->destroy = dia_page_layout_destroy;
}

static void darea_size_allocate(DiaPageLayout *self, GtkAllocation *alloc);
static gint darea_expose_event(DiaPageLayout *self, GdkEventExpose *ev);
static void paper_size_change(GtkWidget *widget, DiaPageLayout *self);
static void orient_changed(DiaPageLayout *self);
static void margin_changed(DiaPageLayout *self);
static void scalemode_changed(DiaPageLayout *self);
static void scale_changed(DiaPageLayout *self);

static void
dia_page_layout_init(DiaPageLayout *self)
{
  GtkWidget *frame, *box, *table, *wid;
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

  self->paper_size = dia_option_menu_new();
  gtk_box_pack_start(GTK_BOX(box), self->paper_size, TRUE, FALSE, 0);

  g_signal_connect (self->paper_size, "changed", 
		    G_CALLBACK(paper_size_change), self);

  paper_names = get_paper_name_list();  
  for (i = 0; paper_names != NULL; 
       i++, paper_names = g_list_next(paper_names)) {

    dia_option_menu_add_item (self->paper_size, paper_names->data, i);
  }
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
  
  wid = gtk_image_new_from_pixbuf (gdk_pixbuf_new_from_xpm_data (portrait_xpm));
  gtk_container_add(GTK_CONTAINER(self->orient_portrait), wid);
  gtk_widget_show(wid);

  gtk_box_pack_start(GTK_BOX(box), self->orient_portrait, TRUE, TRUE, 0);
  gtk_widget_show(self->orient_portrait);

  self->orient_landscape = gtk_radio_button_new(
	gtk_radio_button_get_group(GTK_RADIO_BUTTON(self->orient_portrait)));
  wid = gtk_image_new_from_pixbuf (gdk_pixbuf_new_from_xpm_data (landscape_xpm));
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
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 0,100, 0.1,10,0)),
	prefs_get_length_unit());
  gtk_table_attach(GTK_TABLE(table), self->tmargin, 1,2, 0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->tmargin);

  wid = gtk_label_new(_("Bottom:"));
  gtk_misc_set_alignment(GTK_MISC(wid), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), wid, 0,1, 1,2,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(wid);

  self->bmargin = dia_unit_spinner_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 0,100, 0.1,10,0)),
	prefs_get_length_unit());
  gtk_table_attach(GTK_TABLE(table), self->bmargin, 1,2, 1,2,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->bmargin);

  wid = gtk_label_new(_("Left:"));
  gtk_misc_set_alignment(GTK_MISC(wid), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), wid, 0,1, 2,3,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(wid);

  self->lmargin = dia_unit_spinner_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 0,100, 0.1,10,0)),
	prefs_get_length_unit());
  gtk_table_attach(GTK_TABLE(table), self->lmargin, 1,2, 2,3,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->lmargin);

  wid = gtk_label_new(_("Right:"));
  gtk_misc_set_alignment(GTK_MISC(wid), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), wid, 0,1, 3,4,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(wid);

  self->rmargin = dia_unit_spinner_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 0,100, 0.1,10,0)),
	prefs_get_length_unit());
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
	GTK_ADJUSTMENT(gtk_adjustment_new(100,1,10000, 1,10,0)), 1, 0);
  gtk_table_attach(GTK_TABLE(table), self->scaling, 1,4, 0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->scaling);

  self->fitto = gtk_radio_button_new_with_label(
			gtk_radio_button_get_group(GTK_RADIO_BUTTON(self->scale)), _("Fit to:"));
  gtk_table_attach(GTK_TABLE(table), self->fitto, 0,1, 1,2,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->fitto);

  self->fitw = gtk_spin_button_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 1, 1000, 1, 10, 0)), 1, 0);
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
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 1, 1000, 1, 10, 0)), 1, 0);
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
  g_signal_connect_swapped(G_OBJECT(self->orient_portrait), "toggled",
			  G_CALLBACK(orient_changed), G_OBJECT(self));

  g_signal_connect_swapped(G_OBJECT(self->tmargin), "changed",
			   G_CALLBACK(margin_changed), G_OBJECT(self));
  g_signal_connect_swapped(G_OBJECT(self->bmargin), "changed",
			   G_CALLBACK(margin_changed), G_OBJECT(self));
  g_signal_connect_swapped(G_OBJECT(self->lmargin), "changed",
			   G_CALLBACK(margin_changed), G_OBJECT(self));
  g_signal_connect_swapped(G_OBJECT(self->rmargin), "changed",
			   G_CALLBACK(margin_changed), G_OBJECT(self));

  g_signal_connect_swapped(G_OBJECT(self->fitto), "toggled",
			   G_CALLBACK(scalemode_changed),
			    G_OBJECT(self));
  g_signal_connect_swapped(G_OBJECT(self->scaling), "changed",
			   G_CALLBACK(scale_changed), G_OBJECT(self));
  g_signal_connect_swapped(G_OBJECT(self->fitw), "changed",
			   G_CALLBACK(scale_changed), G_OBJECT(self));
  g_signal_connect_swapped(G_OBJECT(self->fith), "changed",
			   G_CALLBACK(scale_changed), G_OBJECT(self));

  g_signal_connect_swapped(G_OBJECT(self->darea), "size_allocate",
			   G_CALLBACK(darea_size_allocate),
			    G_OBJECT(self));
  g_signal_connect_swapped(G_OBJECT(self->darea), "expose_event",
			   G_CALLBACK(darea_expose_event),
			    G_OBJECT(self));

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
  DiaPageLayout *self = g_object_new(dia_page_layout_get_type(), NULL);

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
    i = find_paper(prefs.new_diagram.papertype);
  dia_option_menu_set_active (self->paper_size, i);
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

  g_signal_emit(G_OBJECT(self), pl_signals[CHANGED], 0);
}

DiaPageOrientation
dia_page_layout_get_orientation(DiaPageLayout *self)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(self->orient_portrait)))
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
  return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(self->fitto));
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
  GtkAdjustment *adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON(self->scaling));
  
  return gtk_adjustment_get_value (adj) / 100.0;
}

void
dia_page_layout_set_scaling(DiaPageLayout *self, gfloat scaling)
{
  GtkAdjustment *adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON(self->scaling));

  gtk_adjustment_set_value (adj, scaling * 100.0);
}

void
dia_page_layout_set_changed (DiaPageLayout *self, gboolean changed)
{
  self->block_changed = changed;
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
  GtkAdjustment *adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON(self->scaling));

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(self->orient_portrait))) {
    w = get_paper_pswidth(self->papernum);
    h = get_paper_psheight(self->papernum);
  } else {
    h = get_paper_pswidth(self->papernum);
    w = get_paper_psheight(self->papernum);
  }
  h -= dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->tmargin));
  g_return_if_fail (h > 0.0);
  h -= dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->bmargin));
  g_return_if_fail (h > 0.0);
  w -= dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->lmargin));
  g_return_if_fail (w > 0.0);
  w -= dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->rmargin));
  g_return_if_fail (w > 0.0);
  scaling = gtk_adjustment_get_value (adj) / 100.0;
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
    i = find_paper(prefs.new_diagram.papertype);
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
    i = find_paper(prefs.new_diagram.papertype);
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
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(self->orient_portrait))) {
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
  GdkWindow *window = gtk_widget_get_window(self->darea);
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
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(self->orient_portrait))) {
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

/*!
 * \brief given a (page) size calculate the maximum margin size from it 
 *
 * The function calculation assumes that more than half the size is not useful
 * for the margin. For safety it allows a little less.
 */
static float
max_margin (float size)
{
  return size / 2.0 - 0.5;
}

static void
paper_size_change(GtkWidget *widget, DiaPageLayout *self)
{
  gchar buf[512];

  self->papernum = dia_option_menu_get_active (widget);
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

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(self->orient_portrait))) {
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->tmargin),
      max_margin(get_paper_psheight(self->papernum)));
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->bmargin),
      max_margin(get_paper_psheight(self->papernum)));
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->lmargin),
      max_margin(get_paper_pswidth(self->papernum)));
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->rmargin),
      max_margin(get_paper_pswidth(self->papernum)));
  } else {
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->tmargin),
      max_margin(get_paper_pswidth(self->papernum)));
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->bmargin),
      max_margin(get_paper_pswidth(self->papernum)));
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->lmargin),
      max_margin(get_paper_psheight(self->papernum)));
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->rmargin),
      max_margin(get_paper_psheight(self->papernum)));
  }
  self->block_changed = FALSE;

  g_snprintf(buf, sizeof(buf), _("%0.3gcm x %0.3gcm"),
	     get_paper_pswidth(self->papernum),
	     get_paper_psheight(self->papernum));
  gtk_label_set_text(GTK_LABEL(self->paper_label), buf);

  g_signal_emit(G_OBJECT(self), pl_signals[CHANGED], 0);
}

static void
orient_changed(DiaPageLayout *self)
{
  size_page(self, &self->darea->allocation);
  gtk_widget_queue_draw(self->darea);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(self->orient_portrait))) {
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->tmargin),
      max_margin(get_paper_psheight(self->papernum)));
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->bmargin),
      max_margin(get_paper_psheight(self->papernum)));
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->lmargin),
      max_margin(get_paper_pswidth(self->papernum)));
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->rmargin),
      max_margin(get_paper_pswidth(self->papernum)));
  } else {
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->tmargin),
      max_margin(get_paper_pswidth(self->papernum)));
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->bmargin),
      max_margin(get_paper_pswidth(self->papernum)));
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->lmargin),
      max_margin(get_paper_psheight(self->papernum)));
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->rmargin),
      max_margin(get_paper_psheight(self->papernum)));
  }

  if (!self->block_changed)
    g_signal_emit(G_OBJECT(self), pl_signals[CHANGED], 0);
}

static void
margin_changed(DiaPageLayout *self)
{
  gtk_widget_queue_draw(self->darea);
  if (!self->block_changed)
    g_signal_emit(G_OBJECT(self), pl_signals[CHANGED], 0);
}

static void
scalemode_changed(DiaPageLayout *self)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(self->fitto))) {
    gtk_widget_set_sensitive(self->scaling, FALSE);
    gtk_widget_set_sensitive(self->fitw, TRUE);
    gtk_widget_set_sensitive(self->fith, TRUE);
  } else {
    gtk_widget_set_sensitive(self->scaling, TRUE);
    gtk_widget_set_sensitive(self->fitw, FALSE);
    gtk_widget_set_sensitive(self->fith, FALSE);
  }
  if (!self->block_changed)
    g_signal_emit(G_OBJECT(self), pl_signals[CHANGED], 0);
}

static void
scale_changed(DiaPageLayout *self)
{
  if (!self->block_changed)
    g_signal_emit(G_OBJECT(self), pl_signals[CHANGED], 0);
}

static void
dia_page_layout_destroy(GtkObject *object)
{
  DiaPageLayout *self = DIA_PAGE_LAYOUT(object);

  if (self->gc) {
    g_object_unref(self->gc);
    self->gc = NULL;
  }

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
  g_signal_connect(G_OBJECT(win), "destroy",
		   G_CALLBACK(gtk_main_quit), NULL);

  pl = dia_page_layout_new();
  gtk_container_set_border_width(GTK_CONTAINER(pl), 5);
  gtk_container_add(GTK_CONTAINER(win), pl);
  gtk_widget_show(pl);

  g_signal_connect(G_OBJECT(pl), "changed",
		   G_CALLBACK(changed_signal), NULL);
  g_signal_connect(G_OBJECT(pl), "fittopage",
		   G_CALLBACK(fittopage_signal), NULL);

  gtk_widget_show(win);
  gtk_main();
}

#endif
