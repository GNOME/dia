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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <xpm-pixbuf.h>

#include "dia-page-layout.h"

#include "pixmaps/portrait.xpm"
#include "pixmaps/landscape.xpm"

#include "preferences.h"
#include "paper.h"
#include "prefs.h"

#include "diamarshal.h"
#include "diaoptionmenu.h"
#include "dia-unit-spinner.h"


struct _DiaPageLayout {
  GtkGrid parent;

  /*<private>*/
  GtkWidget *paper_size, *paper_label;
  GtkWidget *orient_portrait, *orient_landscape;
  GtkWidget *tmargin, *bmargin, *lmargin, *rmargin;
  GtkWidget *scale, *fitto;
  GtkWidget *scaling, *fitw, *fith;

  GtkWidget *darea;

  int papernum; /* index into page_metrics array */

  /* position of paper preview */
  int x, y, width, height;

  gboolean block_changed;
};

G_DEFINE_TYPE (DiaPageLayout, dia_page_layout, GTK_TYPE_GRID)


enum {
  CHANGED,
  LAST_SIGNAL
};
static guint pl_signals[LAST_SIGNAL] = { 0 };


static void
dia_page_layout_class_init (DiaPageLayoutClass *klass)
{
  pl_signals[CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  dia_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}


static void darea_size_allocate(DiaPageLayout *self, GtkAllocation *alloc);
static gboolean darea_draw (GtkWidget *self, cairo_t *ctx);
static void paper_size_change(GtkWidget *widget, DiaPageLayout *self);
static void orient_changed(DiaPageLayout *self);
static void margin_changed(DiaPageLayout *self);
static void scalemode_changed(DiaPageLayout *self);
static void scale_changed(DiaPageLayout *self);


static void
dia_page_layout_init (DiaPageLayout *self)
{
  GtkWidget *frame, *box, *grid, *wid;
  GList *paper_names;
  int i;

  gtk_grid_set_row_spacing (GTK_GRID (self), 5);
  gtk_grid_set_column_spacing (GTK_GRID (self), 5);

  /* paper size */
  frame = gtk_frame_new (_("Paper Size"));
  gtk_grid_attach (GTK_GRID (self), frame, 0, 0, 1, 1);
  gtk_widget_show (frame);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_set_border_width (GTK_CONTAINER (box), 5);
  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_widget_show (box);

  self->paper_size = dia_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (box), self->paper_size, TRUE, FALSE, 0);

  g_signal_connect (self->paper_size,
                    "changed",
                    G_CALLBACK (paper_size_change),
                    self);

  paper_names = get_paper_name_list ();
  for (i = 0; paper_names != NULL;
       i++, paper_names = g_list_next(paper_names)) {

    dia_option_menu_add_item (DIA_OPTION_MENU (self->paper_size),
                              paper_names->data,
                              i);
  }
  gtk_widget_show (self->paper_size);

  self->paper_label = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (box), self->paper_label, TRUE, TRUE, 0);
  gtk_widget_show (self->paper_label);

  /* orientation */
  frame = gtk_frame_new (_("Orientation"));
  gtk_grid_attach (GTK_GRID(self), frame, 1, 0, 1, 1);
  gtk_widget_set_hexpand (GTK_WIDGET (self), TRUE);
  gtk_widget_show (frame);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_container_set_border_width (GTK_CONTAINER (box), 5);
  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_widget_show (box);

  self->orient_portrait = gtk_radio_button_new (NULL);

  wid = gtk_image_new_from_pixbuf (xpm_pixbuf_load (portrait_xpm));
  gtk_container_add (GTK_CONTAINER (self->orient_portrait), wid);
  gtk_widget_show (wid);

  gtk_box_pack_start (GTK_BOX(box), self->orient_portrait, TRUE, TRUE, 0);
  gtk_widget_show (self->orient_portrait);

  self->orient_landscape = gtk_radio_button_new (
        gtk_radio_button_get_group (GTK_RADIO_BUTTON (self->orient_portrait)));
  wid = gtk_image_new_from_pixbuf (xpm_pixbuf_load (landscape_xpm));
  gtk_container_add (GTK_CONTAINER (self->orient_landscape), wid);
  gtk_widget_show (wid);

  gtk_box_pack_start (GTK_BOX (box), self->orient_landscape, TRUE, TRUE, 0);
  gtk_widget_show (self->orient_landscape);

  /* margins */
  frame = gtk_frame_new (_("Margins"));
  gtk_grid_attach (GTK_GRID (self), frame, 0, 1, 1, 1);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_container_set_border_width (GTK_CONTAINER (grid), 5);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 5);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 5);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  wid = gtk_label_new (_("Top:"));
  gtk_misc_set_alignment (GTK_MISC (wid), 1.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), wid, 0, 0, 1, 1);
  gtk_widget_set_vexpand(wid, TRUE);
  gtk_widget_show (wid);

  self->tmargin = dia_unit_spinner_new (
                GTK_ADJUSTMENT (gtk_adjustment_new (1, 0, 100, 0.1, 10, 0)),
                prefs_get_length_unit ());
  gtk_grid_attach (GTK_GRID (grid), self->tmargin, 1, 0, 1, 1);
  gtk_widget_set_vexpand (self->tmargin, TRUE);
  gtk_widget_set_hexpand (self->tmargin, TRUE);
  gtk_widget_show (self->tmargin);

  wid = gtk_label_new (_("Bottom:"));
  gtk_misc_set_alignment (GTK_MISC (wid), 1.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), wid, 0, 1, 1, 1);
  gtk_widget_set_vexpand(wid, TRUE);
  gtk_widget_show (wid);

  self->bmargin = dia_unit_spinner_new (
                GTK_ADJUSTMENT (gtk_adjustment_new (1, 0, 100, 0.1, 10, 0)),
                prefs_get_length_unit ());
  gtk_grid_attach (GTK_GRID (grid), self->bmargin, 1, 1, 1, 1);
  gtk_widget_set_vexpand (self->bmargin, TRUE);
  gtk_widget_set_hexpand (self->bmargin, TRUE);
  gtk_widget_show (self->bmargin);

  wid = gtk_label_new (_("Left:"));
  gtk_misc_set_alignment (GTK_MISC (wid), 1.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), wid, 0, 2, 1, 1);
  gtk_widget_set_vexpand(wid, TRUE);
  gtk_widget_show (wid);

  self->lmargin = dia_unit_spinner_new(
                GTK_ADJUSTMENT (gtk_adjustment_new (1, 0, 100, 0.1, 10, 0)),
                prefs_get_length_unit ());
  gtk_grid_attach (GTK_GRID (grid), self->lmargin, 1, 2, 1, 1);
  gtk_widget_set_vexpand (self->lmargin, TRUE);
  gtk_widget_set_hexpand (self->lmargin, TRUE);
  gtk_widget_show (self->lmargin);

  wid = gtk_label_new (_("Right:"));
  gtk_misc_set_alignment (GTK_MISC (wid), 1.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), wid, 0, 3, 1, 1);
  gtk_widget_set_vexpand(wid, TRUE);
  gtk_widget_show (wid);

  self->rmargin = dia_unit_spinner_new(
                GTK_ADJUSTMENT (gtk_adjustment_new (1, 0, 100, 0.1, 10, 0)),
                prefs_get_length_unit ());
  gtk_grid_attach (GTK_GRID (grid), self->rmargin, 1, 3, 1, 1);
  gtk_widget_set_vexpand (self->rmargin, TRUE);
  gtk_widget_set_hexpand (self->rmargin, TRUE);
  gtk_widget_show (self->rmargin);

  /* Scaling */
  frame = gtk_frame_new (_("Scaling"));
  gtk_grid_attach (GTK_GRID (self), frame, 0, 2, 1, 1);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_container_set_border_width (GTK_CONTAINER (grid), 5);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 5);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  self->scale = gtk_radio_button_new_with_label (NULL, _("Scale:"));
  gtk_grid_attach (GTK_GRID (grid), self->scale, 0, 0, 1, 1);
  gtk_widget_set_vexpand (self->scale, TRUE);
  gtk_widget_show (self->scale);

  self->scaling = gtk_spin_button_new(
          GTK_ADJUSTMENT (gtk_adjustment_new (100, 1, 10000, 1, 10, 0)), 1, 0);
  gtk_grid_attach (GTK_GRID (grid), self->scaling, 1, 0, 3, 1);
  gtk_widget_set_vexpand (self->scaling, TRUE);
  gtk_widget_set_hexpand (self->scaling, TRUE);
  gtk_widget_show (self->scaling);

  self->fitto = gtk_radio_button_new_with_label (
            gtk_radio_button_get_group (GTK_RADIO_BUTTON (self->scale)),
            _("Fit to:"));
  gtk_grid_attach (GTK_GRID (grid), self->fitto, 0, 1, 1, 1);
  gtk_widget_set_vexpand (self->fitto, TRUE);
  gtk_widget_show (self->fitto);

  self->fitw = gtk_spin_button_new (
            GTK_ADJUSTMENT (gtk_adjustment_new (1, 1, 1000, 1, 10, 0)), 1, 0);
  gtk_widget_set_sensitive (self->fitw, FALSE);
  gtk_grid_attach (GTK_GRID (grid), self->fitw, 1, 1, 1, 1);
  gtk_widget_set_vexpand (self->fitw, TRUE);
  gtk_widget_set_hexpand (self->fitw, TRUE);
  gtk_widget_show (self->fitw);

  wid = gtk_label_new (_("by"));
  gtk_misc_set_padding (GTK_MISC (wid), 5, 0);
  gtk_grid_attach (GTK_GRID (grid), wid, 2, 1, 1, 1);
  gtk_widget_set_vexpand (wid, TRUE);
  gtk_widget_show (wid);

  self->fith = gtk_spin_button_new(
            GTK_ADJUSTMENT (gtk_adjustment_new (1, 1, 1000, 1, 10, 0)), 1, 0);
  gtk_widget_set_sensitive (self->fith, FALSE);
  gtk_grid_attach (GTK_GRID (grid), self->fith, 3, 1, 1, 1);
  gtk_widget_set_vexpand (self->fith, TRUE);
  gtk_widget_set_hexpand (self->fith, TRUE);
  gtk_widget_show (self->fith);

  /* the drawing area */
  self->darea = gtk_drawing_area_new ();
  gtk_grid_attach (GTK_GRID (self), self->darea, 1, 1, 1, 2);
  gtk_widget_set_vexpand (self->darea, TRUE);
  gtk_widget_set_hexpand (self->darea, TRUE);
  gtk_widget_show (self->darea);

  /* connect the signal handlers */
  g_signal_connect_swapped (G_OBJECT (self->orient_portrait),
                            "toggled",
                            G_CALLBACK (orient_changed),
                            G_OBJECT (self));

  g_signal_connect_swapped (G_OBJECT (self->tmargin),
                            "changed",
                            G_CALLBACK (margin_changed),
                            G_OBJECT (self));
  g_signal_connect_swapped (G_OBJECT (self->bmargin),
                            "changed",
                            G_CALLBACK (margin_changed),
                            G_OBJECT (self));
  g_signal_connect_swapped (G_OBJECT(self->lmargin),
                            "changed",
                            G_CALLBACK (margin_changed),
                            G_OBJECT (self));
  g_signal_connect_swapped (G_OBJECT (self->rmargin),
                            "changed",
                            G_CALLBACK (margin_changed),
                            G_OBJECT (self));

  g_signal_connect_swapped (G_OBJECT (self->fitto),
                            "toggled",
                            G_CALLBACK (scalemode_changed),
                            G_OBJECT (self));
  g_signal_connect_swapped (G_OBJECT (self->scaling),
                            "changed",
                            G_CALLBACK (scale_changed),
                            G_OBJECT (self));
  g_signal_connect_swapped (G_OBJECT (self->fitw),
                            "changed",
                            G_CALLBACK (scale_changed),
                            G_OBJECT (self));
  g_signal_connect_swapped (G_OBJECT (self->fith),
                            "changed",
                            G_CALLBACK (scale_changed),
                            G_OBJECT (self));

  g_signal_connect_swapped (G_OBJECT (self->darea),
                            "size_allocate",
                            G_CALLBACK (darea_size_allocate),
                            G_OBJECT (self));
  g_signal_connect_swapped (G_OBJECT (self->darea),
                            "draw", G_CALLBACK (darea_draw),
                            G_OBJECT (self));

  self->block_changed = FALSE;
}


GtkWidget *
dia_page_layout_new (void)
{
  DiaPageLayout *self = g_object_new (DIA_TYPE_PAGE_LAYOUT, NULL);

  dia_page_layout_set_paper (self, "");

  return GTK_WIDGET (self);
}


const char *
dia_page_layout_get_paper (DiaPageLayout *self)
{
  return dia_paper_get_name (self->papernum);
}


void
dia_page_layout_set_paper (DiaPageLayout *self, const char *paper)
{
  int i;

  i = find_paper (paper);
  if (i == -1) {
    i = find_paper (prefs.new_diagram.papertype);
  }

  dia_option_menu_set_active (DIA_OPTION_MENU (self->paper_size), i);
}


void
dia_page_layout_get_margins (DiaPageLayout *self,
                             double        *tmargin,
                             double        *bmargin,
                             double        *lmargin,
                             double        *rmargin)
{
  if (tmargin) {
    *tmargin = dia_unit_spinner_get_value (DIA_UNIT_SPINNER (self->tmargin));
  }
  if (bmargin) {
    *bmargin = dia_unit_spinner_get_value (DIA_UNIT_SPINNER (self->bmargin));
  }
  if (lmargin) {
    *lmargin = dia_unit_spinner_get_value (DIA_UNIT_SPINNER (self->lmargin));
  }
  if (rmargin) {
    *rmargin = dia_unit_spinner_get_value (DIA_UNIT_SPINNER (self->rmargin));
  }
}


void
dia_page_layout_set_margins (DiaPageLayout *self,
                             double         tmargin,
                             double         bmargin,
                             double         lmargin,
                             double         rmargin)
{
  self->block_changed = TRUE;
  dia_unit_spinner_set_value (DIA_UNIT_SPINNER (self->tmargin), tmargin);
  dia_unit_spinner_set_value (DIA_UNIT_SPINNER (self->bmargin), bmargin);
  dia_unit_spinner_set_value (DIA_UNIT_SPINNER (self->lmargin), lmargin);
  dia_unit_spinner_set_value (DIA_UNIT_SPINNER (self->rmargin), rmargin);
  self->block_changed = FALSE;

  g_signal_emit (G_OBJECT (self), pl_signals[CHANGED], 0);
}


DiaPageOrientation
dia_page_layout_get_orientation (DiaPageLayout *self)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->orient_portrait))) {
    return DIA_PAGE_ORIENT_PORTRAIT;
  } else {
    return DIA_PAGE_ORIENT_LANDSCAPE;
  }
}


void
dia_page_layout_set_orientation (DiaPageLayout      *self,
                                 DiaPageOrientation  orient)
{
  switch (orient) {
    case DIA_PAGE_ORIENT_PORTRAIT:
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->orient_portrait),
                                    TRUE);
      break;
    case DIA_PAGE_ORIENT_LANDSCAPE:
    default:
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->orient_landscape),
                                    TRUE);
      break;
  }
}


gboolean
dia_page_layout_get_fitto (DiaPageLayout *self)
{
  return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->fitto));
}


void
dia_page_layout_set_fitto (DiaPageLayout *self, gboolean fitto)
{
  if (fitto) {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->fitto), TRUE);
  } else {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->scale), TRUE);
  }
}


double
dia_page_layout_get_scaling (DiaPageLayout *self)
{
  GtkAdjustment *adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON(self->scaling));

  return gtk_adjustment_get_value (adj) / 100.0;
}


void
dia_page_layout_set_scaling (DiaPageLayout *self, double scaling)
{
  GtkAdjustment *adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (self->scaling));

  gtk_adjustment_set_value (adj, scaling * 100.0);
}


void
dia_page_layout_set_changed (DiaPageLayout *self, gboolean changed)
{
  self->block_changed = changed;
}


void
dia_page_layout_get_fit_dims (DiaPageLayout *self, int *w, int *h)
{
  if (w) {
    *w = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (self->fitw));
  }

  if (h) {
    *h = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (self->fith));
  }
}


void
dia_page_layout_set_fit_dims (DiaPageLayout *self, int w, int h)
{
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (self->fitw), w);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (self->fith), h);
}


void
dia_page_layout_get_effective_area (DiaPageLayout *self,
                                    double        *width,
                                    double        *height)
{
  double h, w, scaling;
  GtkAdjustment *adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (self->scaling));

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->orient_portrait))) {
    w = get_paper_pswidth (self->papernum);
    h = get_paper_psheight (self->papernum);
  } else {
    h = get_paper_pswidth (self->papernum);
    w = get_paper_psheight (self->papernum);
  }
  h -= dia_unit_spinner_get_value (DIA_UNIT_SPINNER (self->tmargin));
  g_return_if_fail (h > 0.0);
  h -= dia_unit_spinner_get_value (DIA_UNIT_SPINNER (self->bmargin));
  g_return_if_fail (h > 0.0);
  w -= dia_unit_spinner_get_value (DIA_UNIT_SPINNER (self->lmargin));
  g_return_if_fail (w > 0.0);
  w -= dia_unit_spinner_get_value (DIA_UNIT_SPINNER (self->rmargin));
  g_return_if_fail (w > 0.0);
  scaling = gtk_adjustment_get_value (adj) / 100.0;
  h /= scaling;
  w /= scaling;

  if (width) {
    *width = w;
  }

  if (height) {
    *height = h;
  }
}


void
dia_page_layout_get_paper_size (const char *paper,
                                double     *width,
                                double     *height)
{
  int i;

  i = find_paper (paper);
  if (i == -1) {
    i = find_paper (prefs.new_diagram.papertype);
  }

  if (width) {
    *width = get_paper_pswidth (i);
  }

  if (height) {
    *height = get_paper_psheight (i);
  }
}


void
dia_page_layout_get_default_margins (const char *paper,
                                     double     *tmargin,
                                     double     *bmargin,
                                     double     *lmargin,
                                     double     *rmargin)
{
  int i;

  i = find_paper (paper);
  if (i == -1) {
    i = find_paper (prefs.new_diagram.papertype);
  }

  if (tmargin) {
    *tmargin = get_paper_tmargin (i);
  }

  if (bmargin) {
    *bmargin = get_paper_bmargin (i);
  }

  if (lmargin) {
    *lmargin = get_paper_lmargin (i);
  }

  if (rmargin) {
    *rmargin = get_paper_rmargin (i);
  }
}


static void
size_page (DiaPageLayout *self, GtkAllocation *a)
{
  self->width = a->width - 3;
  self->height = a->height - 3;

  /* change to correct metrics */
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->orient_portrait))) {
    if (self->width * get_paper_psheight(self->papernum) >
          self->height * get_paper_pswidth (self->papernum)) {
      self->width = self->height * get_paper_pswidth (self->papernum) /
                                      get_paper_psheight (self->papernum);
    } else {
      self->height = self->width * get_paper_psheight (self->papernum) /
                                      get_paper_pswidth (self->papernum);
    }
  } else {
    if (self->width * get_paper_pswidth (self->papernum) >
          self->height * get_paper_psheight (self->papernum)) {
      self->width = self->height * get_paper_psheight (self->papernum) /
                                      get_paper_pswidth (self->papernum);
    } else {
      self->height = self->width * get_paper_pswidth (self->papernum) /
                                      get_paper_psheight (self->papernum);
    }
  }

  self->x = (a->width - self->width - 3) / 2;
  self->y = (a->height - self->height - 3) / 2;
}


static void
darea_size_allocate (DiaPageLayout *self, GtkAllocation *allocation)
{
  size_page (self, allocation);
}


static gboolean
darea_draw (GtkWidget *widget, cairo_t *ctx)
{
  DiaPageLayout *self = DIA_PAGE_LAYOUT (widget);
  GdkWindow *window = gtk_widget_get_window (self->darea);
  double val;
  int num;
  GtkAllocation alloc;

  if (!window)
    return FALSE;

  cairo_set_line_cap (ctx, CAIRO_LINE_CAP_SQUARE);
  cairo_set_line_width (ctx, 1);
  cairo_set_antialias (ctx, CAIRO_ANTIALIAS_NONE);

  gtk_widget_get_allocation (self->darea, &alloc);

  cairo_set_source_rgba (ctx, 0, 0, 0, 0);
  cairo_rectangle (ctx, 0, 0,
                   alloc.width,
                   alloc.height);
  cairo_fill (ctx);

  /* draw the page image */
  cairo_set_source_rgba (ctx, 0, 0, 0, 1.0);
  cairo_rectangle(ctx, self->x+3, self->y+3, self->width, self->height);
  cairo_fill (ctx);
  cairo_set_source_rgba (ctx, 1.0, 1.0, 1.0, 1.0);
  cairo_rectangle (ctx, self->x, self->y, self->width, self->height);
  cairo_fill (ctx);
  cairo_set_source_rgba (ctx, 0, 0, 0, 1.0);
  cairo_rectangle (ctx, self->x + 1, self->y, self->width, self->height);
  cairo_stroke (ctx);

  cairo_set_source_rgba (ctx, 0, 0, 0.8, 1.0);

  /* draw margins */
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->orient_portrait))) {
    /* Top */
    val = dia_unit_spinner_get_value (DIA_UNIT_SPINNER (self->tmargin));
    num = self->y + val * self->height / get_paper_psheight (self->papernum);
    cairo_move_to (ctx, self->x + 2, num);
    cairo_line_to (ctx, self->x + self->width, num);
    cairo_stroke (ctx);

    /* Bottom */
    val = dia_unit_spinner_get_value (DIA_UNIT_SPINNER (self->bmargin));
    num = self->y + self->height -
      val * self->height / get_paper_psheight (self->papernum);
    cairo_move_to (ctx, self->x + 2, num);
    cairo_line_to (ctx, self->x + self->width, num);
    cairo_stroke (ctx);

    /* Left */
    val = dia_unit_spinner_get_value (DIA_UNIT_SPINNER (self->lmargin));
    num = self->x + val * self->width / get_paper_pswidth (self->papernum);
    cairo_move_to (ctx, num + 1, self->y + 1);
    cairo_line_to (ctx, num + 1, self->y + self->height - 1);
    cairo_stroke (ctx);

    /* Right */
    val = dia_unit_spinner_get_value (DIA_UNIT_SPINNER (self->rmargin));
    num = self->x + self->width -
      val * self->width / get_paper_pswidth (self->papernum);
    cairo_move_to (ctx, num + 1, self->y + 1);
    cairo_line_to (ctx, num + 1, self->y + self->height - 1);
    cairo_stroke (ctx);
  } else {
    /* Top */
    val = dia_unit_spinner_get_value (DIA_UNIT_SPINNER (self->tmargin));
    num = self->y + val * self->height / get_paper_pswidth (self->papernum);
    cairo_move_to (ctx, self->x + 2, num);
    cairo_line_to (ctx, self->x + self->width, num);
    cairo_stroke (ctx);

    /* Bottom */
    val = dia_unit_spinner_get_value (DIA_UNIT_SPINNER (self->bmargin));
    num = self->y + self->height -
      val * self->height / get_paper_pswidth (self->papernum);
    cairo_move_to (ctx, self->x + 2, num);
    cairo_line_to (ctx, self->x + self->width, num);
    cairo_stroke (ctx);

    /* Left */
    val = dia_unit_spinner_get_value (DIA_UNIT_SPINNER (self->lmargin));
    num = self->x + val * self->width / get_paper_psheight (self->papernum);
    cairo_move_to (ctx, num + 1, self->y + 1);
    cairo_line_to (ctx, num + 1, self->y + self->height - 1);
    cairo_stroke (ctx);

    /* Right */
    val = dia_unit_spinner_get_value (DIA_UNIT_SPINNER (self->rmargin));
    num = self->x + self->width -
      val * self->width / get_paper_psheight (self->papernum);
    cairo_move_to (ctx, num + 1, self->y + 1);
    cairo_line_to (ctx, num + 1, self->y + self->height - 1);
    cairo_stroke (ctx);
  }

  return FALSE;
}


/**
 * max_margin:
 * @size: page size
 *
 * Given a (page) size calculate the maximum margin size from it
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
paper_size_change (GtkWidget *widget, DiaPageLayout *self)
{
  char buf[512];
  GtkAllocation alloc;

  gtk_widget_get_allocation (self->darea, &alloc);

  self->papernum = dia_option_menu_get_active (DIA_OPTION_MENU (widget));
  size_page (self, &alloc);
  gtk_widget_queue_draw (self->darea);

  self->block_changed = TRUE;
  dia_unit_spinner_set_value (DIA_UNIT_SPINNER (self->tmargin),
                              get_paper_tmargin (self->papernum));
  dia_unit_spinner_set_value (DIA_UNIT_SPINNER (self->bmargin),
                              get_paper_bmargin (self->papernum));
  dia_unit_spinner_set_value (DIA_UNIT_SPINNER (self->lmargin),
                              get_paper_lmargin (self->papernum));
  dia_unit_spinner_set_value (DIA_UNIT_SPINNER (self->rmargin),
                              get_paper_rmargin (self->papernum));

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->orient_portrait))) {
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER (self->tmargin),
                            max_margin (get_paper_psheight (self->papernum)));
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->bmargin),
                            max_margin (get_paper_psheight (self->papernum)));
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->lmargin),
                            max_margin (get_paper_pswidth (self->papernum)));
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->rmargin),
                            max_margin (get_paper_pswidth (self->papernum)));
  } else {
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER (self->tmargin),
                            max_margin (get_paper_pswidth (self->papernum)));
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER (self->bmargin),
                            max_margin (get_paper_pswidth (self->papernum)));
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER (self->lmargin),
                            max_margin (get_paper_psheight (self->papernum)));
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER (self->rmargin),
                            max_margin (get_paper_psheight (self->papernum)));
  }
  self->block_changed = FALSE;

  g_snprintf (buf, sizeof (buf), _("%0.3gcm x %0.3gcm"),
              get_paper_pswidth (self->papernum),
              get_paper_psheight (self->papernum));
  gtk_label_set_text (GTK_LABEL (self->paper_label), buf);

  g_signal_emit (G_OBJECT (self), pl_signals[CHANGED], 0);
}


static void
orient_changed (DiaPageLayout *self)
{
  GtkAllocation alloc;

  gtk_widget_get_allocation (self->darea, &alloc);

  size_page (self, &alloc);
  gtk_widget_queue_draw (self->darea);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(self->orient_portrait))) {
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->tmargin),
                            max_margin (get_paper_psheight (self->papernum)));
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->bmargin),
                            max_margin (get_paper_psheight (self->papernum)));
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->lmargin),
                            max_margin (get_paper_pswidth (self->papernum)));
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->rmargin),
                            max_margin (get_paper_pswidth (self->papernum)));
  } else {
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->tmargin),
                            max_margin (get_paper_pswidth (self->papernum)));
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->bmargin),
                            max_margin (get_paper_pswidth (self->papernum)));
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->lmargin),
                            max_margin (get_paper_psheight (self->papernum)));
    dia_unit_spinner_set_upper (DIA_UNIT_SPINNER(self->rmargin),
                            max_margin (get_paper_psheight (self->papernum)));
  }

  if (!self->block_changed) {
    g_signal_emit (G_OBJECT (self), pl_signals[CHANGED], 0);
  }
}


static void
margin_changed (DiaPageLayout *self)
{
  gtk_widget_queue_draw (self->darea);
  if (!self->block_changed) {
    g_signal_emit (G_OBJECT(self), pl_signals[CHANGED], 0);
  }
}


static void
scalemode_changed (DiaPageLayout *self)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->fitto))) {
    gtk_widget_set_sensitive (self->scaling, FALSE);
    gtk_widget_set_sensitive (self->fitw, TRUE);
    gtk_widget_set_sensitive (self->fith, TRUE);
  } else {
    gtk_widget_set_sensitive (self->scaling, TRUE);
    gtk_widget_set_sensitive (self->fitw, FALSE);
    gtk_widget_set_sensitive (self->fith, FALSE);
  }

  if (!self->block_changed) {
    g_signal_emit (G_OBJECT (self), pl_signals[CHANGED], 0);
  }
}


static void
scale_changed (DiaPageLayout *self)
{
  if (!self->block_changed) {
    g_signal_emit (G_OBJECT (self), pl_signals[CHANGED], 0);
  }
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
