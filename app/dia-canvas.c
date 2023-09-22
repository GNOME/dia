/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
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
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "dia-canvas.h"
#include "disp_callbacks.h"
#include "toolbox.h"
#include "diainteractiverenderer.h"
#include "interface.h"
#include "object.h"

struct _DiaCanvas {
  GtkDrawingArea parent;

  DDisplay *display;
};

G_DEFINE_TYPE (DiaCanvas, dia_canvas, GTK_TYPE_DRAWING_AREA)


enum {
  PROP_0,
  PROP_DISPLAY,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
dia_canvas_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  DiaCanvas *self = DIA_CANVAS (object);

  switch (property_id) {
    case PROP_DISPLAY:
      self->display = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_canvas_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  DiaCanvas *self = DIA_CANVAS (object);

  switch (property_id) {
    case PROP_DISPLAY:
      g_value_set_pointer (value, self->display);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


/*!
 * Called when the widget's window "size, position or stacking"
 * changes. Needs GDK_STRUCTURE_MASK set.
 */
static gboolean
dia_canvas_configure_event (GtkWidget *widget, GdkEventConfigure *cevent)
{
  DiaCanvas *self = DIA_CANVAS (widget);
  gboolean new_size = FALSE;
  int width, height;
  DiaRenderer *renderer;

  g_return_val_if_fail (self->display, FALSE);
  g_return_val_if_fail (widget == self->display->canvas, FALSE);

  renderer = self->display->renderer;

  if (renderer) {
    width = dia_interactive_renderer_get_width_pixels (DIA_INTERACTIVE_RENDERER (renderer));
    height = dia_interactive_renderer_get_height_pixels (DIA_INTERACTIVE_RENDERER (renderer));
  } else {
    /* We can continue even without a renderer here because
     * ddisplay_resize_canvas () does the setup for us.
     */
    width = height = 0;
  }

  /* Only do this when size is really changing */
  if (width != cevent->width || height != cevent->height) {
    g_debug ("%s: Canvas size change...", G_STRLOC);
    ddisplay_resize_canvas (self->display, cevent->width, cevent->height);
    ddisplay_update_scrollbars (self->display);
    /* on resize stop further propagation - does not help */
    new_size = TRUE;
  }

  /* If the UI is not integrated, resizing should set the resized
   * window as active.  With integrated UI, there is only one window.
   */
  if (is_integrated_ui () == 0) {
    display_set_active (self->display);
  }

  /* continue propagation with FALSE */
  return new_size;
}


static gboolean
dia_canvas_draw (GtkWidget *widget, cairo_t *ctx)
{
  DiaCanvas *self = DIA_CANVAS (widget);
  GSList *l;
  DiaRectangle *r, totrect;
  GtkAllocation alloc;
  DiaRenderer *renderer;

  g_return_val_if_fail (self->display, FALSE);
  g_return_val_if_fail (self->display->renderer != NULL, FALSE);

  renderer = self->display->renderer;

  /* Only update if update_areas exist */
  l = self->display->update_areas;
  if (l != NULL) {
    totrect = *(DiaRectangle *) l->data;

    dia_interactive_renderer_clip_region_clear (DIA_INTERACTIVE_RENDERER (renderer));

    while ( l!= NULL) {
      r = (DiaRectangle *) l->data;

      rectangle_union (&totrect, r);
      dia_interactive_renderer_clip_region_add_rect (DIA_INTERACTIVE_RENDERER (renderer), r);

      l = g_slist_next (l);
    }
    /* Free update_areas list: */
    g_slist_free_full (self->display->update_areas, g_free);
    self->display->update_areas = NULL;

    totrect.left -= 0.1;
    totrect.right += 0.1;
    totrect.top -= 0.1;
    totrect.bottom += 0.1;

    ddisplay_render_pixmap (self->display, &totrect);
  }

  gtk_widget_get_allocation (widget, &alloc);

  dia_interactive_renderer_paint (DIA_INTERACTIVE_RENDERER (renderer),
                                  ctx,
                                  alloc.width,
                                  alloc.height);

  return FALSE;
}

static gboolean
dia_canvas_event (GtkWidget *widget, GdkEvent *event)
{
  DiaCanvas *self = DIA_CANVAS (widget);

  return ddisplay_canvas_events (widget, event, self->display);
}


static gboolean
dia_canvas_drag_drop (GtkWidget      *widget,
                      GdkDragContext *context,
                      int             x,
                      int             y,
                      guint           time)
{
  if (gtk_drag_get_source_widget (context) != NULL) {
    /* we only accept drops from the same instance of the application,
     * as the drag data is a pointer in our address space */
    return TRUE;
  }

  gtk_drag_finish (context, FALSE, FALSE, time);

  return FALSE;
}


static void
dia_canvas_drag_data_received (GtkWidget        *widget,
                               GdkDragContext   *context,
                               int               x,
                               int               y,
                               GtkSelectionData *data,
                               guint             info,
                               guint             time)
{
  DiaCanvas *self = DIA_CANVAS (widget);

  if (gtk_selection_data_get_format (data) == 8 &&
      gtk_selection_data_get_length (data) == sizeof (ToolButtonData *) &&
      gtk_drag_get_source_widget(context) != NULL) {
    ToolButtonData *tooldata = *(ToolButtonData **) gtk_selection_data_get_data (data);

    ddisplay_drop_object (self->display,
                          x,
                          y,
                          object_get_type ((char *) tooldata->extra_data),
                          tooldata->user_data);

    gtk_drag_finish (context, TRUE, FALSE, time);
  } else {
    dia_dnd_file_drag_data_received (widget,
                                     context,
                                     x,
                                     y,
                                     data,
                                     info,
                                     time,
                                     self->display);
  }

  /* ensure the right window has the focus for text editing */
  gtk_window_present (GTK_WINDOW (self->display->shell));
}


static void
dia_canvas_class_init (DiaCanvasClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_css_name (widget_class, "diacanvas");

  object_class->set_property = dia_canvas_set_property;
  object_class->get_property = dia_canvas_get_property;

  widget_class->configure_event = dia_canvas_configure_event;
  widget_class->draw = dia_canvas_draw;
  widget_class->event = dia_canvas_event;
  widget_class->drag_drop = dia_canvas_drag_drop;
  widget_class->drag_data_received = dia_canvas_drag_data_received;

  pspecs[PROP_DISPLAY] =
    g_param_spec_pointer ("display",
                          "Display",
                          "The DDisplay",
                          G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}


static void
dia_canvas_init (DiaCanvas *self)
{
  gtk_widget_set_events (GTK_WIDGET (self),
                         GDK_EXPOSURE_MASK |
                         GDK_SCROLL_MASK | GDK_SMOOTH_SCROLL_MASK |
                         GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK |
                         GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                         GDK_STRUCTURE_MASK | GDK_ENTER_NOTIFY_MASK |
                         GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);

  gtk_widget_set_can_focus (GTK_WIDGET (self), TRUE);

}


GtkWidget *
dia_canvas_new (DDisplay *ddisp)
{
  GtkWidget *self = g_object_new (DIA_TYPE_CANVAS,
                                  "display", ddisp,
                                  NULL);

  g_object_set_data (G_OBJECT (self), "user_data", (gpointer) ddisp);

  return self;
}
