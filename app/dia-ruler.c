/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * ruler.c - Dia specific reimplementation (C) 2012 Hans Breuer
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

#include <config.h>

/* GtkRuler is deprecated by gtk-2-24, gone with gtk-3-0 because it is
 * deemed to be too specialized for maintenance in Gtk. Maybe Dia is
 * too specialized to be ported?
 *
 * Or maybe not - at least the ruler part can be reimplemented quite
 * easily with already existing application specific logic.
 */

#include "grid.h"
#include "display.h"

#include "dia-ruler.h"


struct _DiaRuler {
  GtkDrawingArea parent;

  DDisplay *ddisp;

  GtkOrientation orientation;
  gdouble lower;
  gdouble upper;
  gdouble position;
  gdouble max_size;
};


G_DEFINE_FINAL_TYPE (DiaRuler, dia_ruler, GTK_TYPE_DRAWING_AREA)


static gboolean
dia_ruler_draw (GtkWidget *widget,
                cairo_t   *cr)
{
  DiaRuler *ruler = DIA_RULER (widget);

  if (gtk_widget_is_drawable (widget)) {
    GtkStyleContext *style = gtk_widget_get_style_context (widget);
    GdkRGBA fg;
    PangoLayout *layout;
    int x, y, dx, dy, width, height;
    real pos;
    GtkAllocation alloc;

    layout = gtk_widget_create_pango_layout (widget, "012456789");

    gtk_widget_get_allocation (widget, &alloc);
    width = alloc.width;
    height = alloc.height;

    dx = (ruler->orientation == GTK_ORIENTATION_VERTICAL) ? width/3 : 0;
    dy = (ruler->orientation == GTK_ORIENTATION_HORIZONTAL) ? height/3 : 0;

    gtk_style_context_get_color (style,
                                 gtk_widget_get_state_flags (widget),
                                 &fg);
    gdk_cairo_set_source_rgba (cr, &fg);
    cairo_set_line_width (cr, 1);

    pos = ruler->lower;
    for (x = 0, y = 0; x < width && y < height;) {
      gboolean is_major;
      int n;

      if (!grid_step (ruler->ddisp, ruler->orientation, &pos,
                      (ruler->orientation == GTK_ORIENTATION_HORIZONTAL) ? &x : &y,
          &is_major)) {
        break;
      }

      /* ticks */
      n = (is_major ? 1 : 2);
      /* + .5 for pixel aligned lines */
      cairo_move_to (cr, x + 3*dx + .5, y + 3*dy + .5);
      cairo_line_to (cr, x + n*dx + .5, y + n*dy + .5);
      cairo_stroke (cr);

        /* label */
      if (is_major) {
        char *markup = g_strdup_printf ("<small>%g</small>", pos);

        pango_layout_set_markup (layout, markup, -1);
        pango_layout_context_changed (layout);
        cairo_save (cr);
        if (ruler->orientation == GTK_ORIENTATION_VERTICAL) {
          cairo_translate (cr, x, y);
          cairo_rotate (cr, -G_PI/2.0);
          cairo_move_to (cr, 2, 0);
        } else {
          cairo_move_to (cr, x+2, 0);
        }
        pango_cairo_show_layout (cr, layout);
        cairo_restore (cr);

        g_clear_pointer (&markup, g_free);
      }
    }

    g_clear_object (&layout);

    /* arrow */
    if (ruler->position > ruler->lower && ruler->position < ruler->upper) {
      real r_pos = ruler->position;
      if (ruler->orientation == GTK_ORIENTATION_VERTICAL) {
        ddisplay_transform_coords (ruler->ddisp, 0, r_pos, &x, &y);
        cairo_move_to (cr, 3*dx, y);
        cairo_line_to (cr, 2*dx, y - dx);
        cairo_line_to (cr, 2*dx, y + dx);
        cairo_fill (cr);
      } else {
        ddisplay_transform_coords (ruler->ddisp, r_pos, 0, &x, &y);
        cairo_move_to (cr, x, 3*dy);
        cairo_line_to (cr, x + dy, 2*dy);
        cairo_line_to (cr, x - dy, 2*dy);
        cairo_fill (cr);
      }
    }
  }
  return FALSE;
}


static gboolean
dia_ruler_motion_notify (GtkWidget      *widget,
                         GdkEventMotion *event)
{
  DiaRuler *ruler = DIA_RULER (widget);
  int x;
  int y;
  real tmp;
  int width, height;
  int x0, y0;
  GtkAllocation alloc;

  gdk_event_request_motions (event);
  x = event->x;
  y = event->y;

  gtk_widget_get_allocation (widget, &alloc);
  width = alloc.width;
  height = alloc.height;

  if (ruler->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      /* get old position in display coordinates */
      ddisplay_transform_coords (ruler->ddisp, ruler->position, 0, &x0, &y0);
      /* update ruler position with original coords */
      ddisplay_untransform_coords (ruler->ddisp, x, y, &ruler->position, &tmp);
      /* reuse x,y to restrict invalidation */
      y0 = y = 0;
      x0 = MAX(x0 - height/2, 0);
      x = MAX(x - height/2, 0);
      width = height;
    }
  else
    {
      ddisplay_transform_coords (ruler->ddisp, 0, ruler->position, &x0, &y0);
      ddisplay_untransform_coords (ruler->ddisp, x, y, &tmp, &ruler->position);
      x0 = x = 0;
      y0 = MAX(y0 - width/2, 0);
      y = MAX(y - width/2, 0);
      height = width;
    }
  if (gtk_widget_is_drawable (GTK_WIDGET (ruler)))
    {
#if 0
      /* this is a bit too expensive - on a slow enough computer the indicators lags */
      gtk_widget_queue_draw (GTK_WIDGET (ruler));
#else
      /* old arrow position */
      gtk_widget_queue_draw_area (GTK_WIDGET (ruler), x0, y0, width, height);
      /* new arrow position */
      gtk_widget_queue_draw_area (GTK_WIDGET (ruler), x, y, width, height);
#endif
    }
  return FALSE;
}


static void
dia_ruler_class_init (DiaRulerClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_css_name (widget_class, "diaruler");

  widget_class->draw = dia_ruler_draw;
}


static void
dia_ruler_init (DiaRuler *rule)
{
}


GtkWidget *
dia_ruler_new (GtkOrientation orientation, GtkWidget *shell, DDisplay *ddisp)
{
  GtkWidget *rule = g_object_new (DIA_TYPE_RULER, NULL);
  /* calculate from style settings  */
  PangoLayout *layout = gtk_widget_create_pango_layout (shell, "X");
  int height;

  pango_layout_get_pixel_size (layout, NULL, &height);
  gtk_widget_set_size_request (rule, height, height);
  g_clear_object (&layout);

  DIA_RULER (rule)->orientation = orientation;
  DIA_RULER (rule)->ddisp = ddisp;

  gtk_widget_set_events (rule, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);

  g_signal_connect_swapped (G_OBJECT (shell), "motion_notify_event",
                            G_CALLBACK (dia_ruler_motion_notify),
                            G_OBJECT (rule));
  return rule;
}


void
dia_ruler_set_range (DiaRuler *self,
                     double    lower,
                     double    upper,
                     double    position,
                     double    max_size)
{
  g_return_if_fail (DIA_IS_RULER (self));

  self->lower = lower;
  self->upper = upper;
  self->position = position;
  self->max_size = max_size;

  if (gtk_widget_is_drawable (GTK_WIDGET (self))) {
    gtk_widget_queue_draw (GTK_WIDGET (self));
    /* XXX: draw arrow at mouse position */
  }
}
