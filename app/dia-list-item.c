/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#include "config.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "dia-list-item.h"


G_DEFINE_TYPE (DiaListItem, dia_list_item, GTK_TYPE_BIN)


enum {
  SCROLL_VERTICAL,
  SCROLL_HORIZONTAL,
  LAST_SIGNAL
};
static guint list_item_signals[LAST_SIGNAL] = {0};


static void
dia_list_item_realize (GtkWidget *widget)
{
  GdkWindowAttr attributes;
  gint attributes_mask;
  GtkAllocation alloc;
  GdkWindow *window;
  GtkStyle *style;

  /*GTK_WIDGET_CLASS (parent_class)->realize (widget);*/

  g_return_if_fail (DIA_IS_LIST_ITEM (widget));

  gtk_widget_set_realized (widget, TRUE);

  gtk_widget_get_allocation (widget, &alloc);

  attributes.x = alloc.x;
  attributes.y = alloc.y;
  attributes.width = alloc.width;
  attributes.height = alloc.height;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes.event_mask = (gtk_widget_get_events (widget) |
                           GDK_EXPOSURE_MASK |
                           GDK_BUTTON_PRESS_MASK |
                           GDK_BUTTON_RELEASE_MASK |
                           GDK_KEY_PRESS_MASK |
                           GDK_KEY_RELEASE_MASK);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
  gtk_widget_set_window (widget, window);
  gdk_window_set_user_data (window, widget);

  gtk_widget_style_attach (widget);

  style = gtk_widget_get_style (widget);
  gdk_window_set_background (window, &style->base[GTK_STATE_NORMAL]);
}


static void
dia_list_item_size_request (GtkWidget      *widget,
                                GtkRequisition *requisition)
{
  GtkBin *bin;
  GtkWidget *child;
  GtkRequisition child_requisition;
  GtkStyle *style;
  gint focus_width;
  gint focus_pad;
  int border_width;

  g_return_if_fail (DIA_IS_LIST_ITEM (widget));
  g_return_if_fail (requisition != NULL);

  bin = GTK_BIN (widget);
  gtk_widget_style_get (widget,
                        "focus-line-width", &focus_width,
                        "focus-padding", &focus_pad,
                        NULL);

  style = gtk_widget_get_style (widget);
  border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));

  requisition->width = 2 * (border_width + style->xthickness + focus_width + focus_pad - 1);
  requisition->height = 2 * (border_width + focus_width + focus_pad - 1);

  child = gtk_bin_get_child (bin);

  if (child && gtk_widget_get_visible (child)) {
    gtk_widget_size_request (child, &child_requisition);

    requisition->width += child_requisition.width;
    requisition->height += child_requisition.height;
  }
}


static void
dia_list_item_size_allocate (GtkWidget     *widget,
                                 GtkAllocation *allocation)
{
  GtkBin *bin;
  GtkStyle *style;
  int border_width;
  GtkAllocation child_allocation;

  g_return_if_fail (DIA_IS_LIST_ITEM (widget));
  g_return_if_fail (allocation != NULL);


  gtk_widget_set_allocation (widget, allocation);

  style = gtk_widget_get_style (widget);
  border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));

  if (gtk_widget_get_realized (widget)) {
    gdk_window_move_resize (gtk_widget_get_window (widget),
                            allocation->x, allocation->y,
                            allocation->width, allocation->height);
  }

  bin = GTK_BIN (widget);

  if (gtk_bin_get_child (bin)) {
    child_allocation.x = (border_width + style->xthickness);
    child_allocation.y = border_width;
    child_allocation.width = allocation->width - child_allocation.x * 2;
    child_allocation.height = allocation->height - child_allocation.y * 2;

    gtk_widget_size_allocate (gtk_bin_get_child (bin), &child_allocation);
  }
}


static void
dia_list_item_style_set (GtkWidget      *widget,
                             GtkStyle       *previous_style)
{
  GtkStyle *style;

  g_return_if_fail (widget != NULL);

  if (previous_style && gtk_widget_get_realized (widget)) {
    style = gtk_widget_get_style (widget);
    gdk_window_set_background (gtk_widget_get_window (widget),
                               &style->base[gtk_widget_get_state (widget)]);
  }
}


static gint
dia_list_item_expose (GtkWidget      *widget,
                          GdkEventExpose *event)
{
  GtkAllocation alloc;
  GdkWindow *window;
  GtkStyle *style;

  g_return_val_if_fail (widget != NULL, FALSE);

  if (gtk_widget_is_drawable (widget)) {
    window = gtk_widget_get_window (widget);
    style = gtk_widget_get_style (widget);

    if (gtk_widget_get_state (GTK_WIDGET (widget)) == GTK_STATE_NORMAL) {
      gdk_window_set_back_pixmap (window, NULL, TRUE);
      gdk_window_clear_area (window, event->area.x, event->area.y,
                             event->area.width, event->area.height);
    } else {
      gtk_paint_flat_box (style, window,
                          gtk_widget_get_state (GTK_WIDGET (widget)), GTK_SHADOW_ETCHED_OUT,
                          &event->area, widget, "listitem",
                          0, 0, -1, -1);
    }

    GTK_WIDGET_CLASS (dia_list_item_parent_class)->expose_event (widget, event);

    gtk_widget_get_allocation (widget, &alloc);

    if (gtk_widget_has_focus (widget)) {
      gtk_paint_focus (style, window, gtk_widget_get_state (widget),
                       NULL, widget, NULL,
                       0, 0, alloc.width, alloc.height);
    }
  }

  return FALSE;
}


static void
dia_list_item_class_init (DiaListItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkBindingSet *binding_set;

  widget_class->realize = dia_list_item_realize;
  widget_class->size_request = dia_list_item_size_request;
  widget_class->size_allocate = dia_list_item_size_allocate;
  widget_class->style_set = dia_list_item_style_set;
  widget_class->expose_event = dia_list_item_expose;

  klass->scroll_vertical = NULL;

  list_item_signals[SCROLL_VERTICAL] =
    g_signal_new ("scroll-vertical",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (DiaListItemClass, scroll_vertical),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 2, GTK_TYPE_SCROLL_TYPE, G_TYPE_DOUBLE);

  binding_set = gtk_binding_set_by_class (klass);
  gtk_binding_entry_add_signal (binding_set, GDK_Up, 0,
                                "scroll-vertical", 2,
                                G_TYPE_ENUM, GTK_SCROLL_STEP_BACKWARD,
                                G_TYPE_DOUBLE, 0.0);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Up, 0,
                                "scroll-vertical", 2,
                                G_TYPE_ENUM, GTK_SCROLL_STEP_BACKWARD,
                                G_TYPE_DOUBLE, 0.0);
  gtk_binding_entry_add_signal (binding_set, GDK_Down, 0,
                                "scroll-vertical", 2,
                                G_TYPE_ENUM, GTK_SCROLL_STEP_FORWARD,
                                G_TYPE_DOUBLE, 0.0);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Down, 0,
                                "scroll-vertical", 2,
                                G_TYPE_ENUM, GTK_SCROLL_STEP_FORWARD,
                                G_TYPE_DOUBLE, 0.0);
  gtk_binding_entry_add_signal (binding_set, GDK_Page_Up, 0,
                                "scroll-vertical", 2,
                                G_TYPE_ENUM, GTK_SCROLL_PAGE_BACKWARD,
                                G_TYPE_DOUBLE, 0.0);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Page_Up, 0,
                                "scroll-vertical", 2,
                                G_TYPE_ENUM, GTK_SCROLL_PAGE_BACKWARD,
                                G_TYPE_DOUBLE, 0.0);
  gtk_binding_entry_add_signal (binding_set, GDK_Page_Down, 0,
                                "scroll-vertical", 2,
                                G_TYPE_ENUM, GTK_SCROLL_PAGE_FORWARD,
                                G_TYPE_DOUBLE, 0.0);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Page_Down, 0,
                                "scroll-vertical", 2,
                                G_TYPE_ENUM, GTK_SCROLL_PAGE_FORWARD,
                                G_TYPE_DOUBLE, 0.0);
  gtk_binding_entry_add_signal (binding_set, GDK_Home, GDK_CONTROL_MASK,
                                "scroll-vertical", 2,
                                G_TYPE_ENUM, GTK_SCROLL_JUMP,
                                G_TYPE_DOUBLE, 0.0);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Home, GDK_CONTROL_MASK,
                                "scroll-vertical", 2,
                                G_TYPE_ENUM, GTK_SCROLL_JUMP,
                                G_TYPE_DOUBLE, 0.0);
  gtk_binding_entry_add_signal (binding_set, GDK_End, GDK_CONTROL_MASK,
                                "scroll-vertical", 2,
                                G_TYPE_ENUM, GTK_SCROLL_JUMP,
                                G_TYPE_DOUBLE, 1.0);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_End, GDK_CONTROL_MASK,
                                "scroll-vertical", 2,
                                G_TYPE_ENUM, GTK_SCROLL_JUMP,
                                G_TYPE_DOUBLE, 1.0);
}


static void
dia_list_item_init (DiaListItem *list_item)
{
  gtk_widget_set_has_window (GTK_WIDGET (list_item), TRUE);
  gtk_widget_set_can_focus (GTK_WIDGET (list_item), TRUE);
}
