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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#include "dia-list.h"
#include "dia-layer-widget.h"


typedef struct _DiaListPrivate DiaListPrivate;
struct _DiaListPrivate
{
  GtkContainer container;

  GList *children;
  DiaLayerWidget *selected;

  GtkWidget *last_focus_child;
};


G_DEFINE_TYPE_WITH_PRIVATE (DiaList, dia_list, GTK_TYPE_CONTAINER)


static void
dia_list_dispose (GObject *object)
{
  dia_list_clear_items (DIA_LIST (object), 0, -1);

  G_OBJECT_CLASS (dia_list_parent_class)->dispose (object);
}


static void
dia_list_size_request (GtkWidget      *widget,
                       GtkRequisition *requisition)
{
  DiaList *list = DIA_LIST (widget);
  DiaListPrivate *priv = dia_list_get_instance_private (list);
  GtkWidget *child;
  GList *children;
  int border_width;

  requisition->width = 0;
  requisition->height = 0;

  children = priv->children;
  while (children) {
    child = children->data;
    children = children->next;

    if (gtk_widget_get_visible (child)) {
      GtkRequisition child_requisition;

      gtk_widget_size_request (child, &child_requisition);

      requisition->width = MAX (requisition->width,
              child_requisition.width);
      requisition->height += child_requisition.height;
    }
  }

  border_width = gtk_container_get_border_width (GTK_CONTAINER (list));

  requisition->width += border_width * 2;
  requisition->height += border_width * 2;

  requisition->width = MAX (requisition->width, 1);
  requisition->height = MAX (requisition->height, 1);
}


static void
dia_list_size_allocate (GtkWidget     *widget,
                        GtkAllocation *allocation)
{
  DiaList *list = DIA_LIST (widget);
  DiaListPrivate *priv = dia_list_get_instance_private (list);
  GtkWidget *child;
  GtkAllocation child_allocation;
  GList *children;
  GdkWindow *window;

  gtk_widget_set_allocation (widget, allocation);

  window = gtk_widget_get_window (widget);

  if (gtk_widget_get_realized (widget)) {
    gdk_window_move_resize (window,
                            allocation->x, allocation->y,
                            allocation->width, allocation->height);
  }

  if (priv->children) {
    int border_width = gtk_container_get_border_width (GTK_CONTAINER (list));
    child_allocation.x = border_width;
    child_allocation.y = border_width;
    child_allocation.width = MAX (1,
                                  allocation->width - child_allocation.x * 2);

    children = priv->children;

    while (children) {
      child = children->data;
      children = children->next;

      if (gtk_widget_get_visible (child)) {
          GtkRequisition child_requisition;

          gtk_widget_get_child_requisition (child, &child_requisition);

          child_allocation.height = child_requisition.height;

          gtk_widget_size_allocate (child, &child_allocation);

          child_allocation.y += child_allocation.height;
        }
    }
  }
}


static void
dia_list_realize (GtkWidget *widget)
{
  GtkAllocation alloc;
  GdkWindowAttr attributes;
  gint attributes_mask;
  GdkWindow *window;
  GtkStyle *style;
  gtk_widget_set_realized (widget, TRUE);

  gtk_widget_get_allocation (widget, &alloc);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = alloc.x;
  attributes.y = alloc.y;
  attributes.width = alloc.width;
  attributes.height = alloc.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes.event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK;

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;


  window = gdk_window_new (gtk_widget_get_parent_window (widget),
                           &attributes, attributes_mask);
  gtk_widget_set_window (widget, window);
  gdk_window_set_user_data (window, widget);

  gtk_widget_style_attach (widget);

  style = gtk_widget_get_style (widget);

  gdk_window_set_background (window,
                             &style->base[GTK_STATE_NORMAL]);
}


static void
dia_list_unmap (GtkWidget *widget)
{
  GdkWindow *window;

  if (!gtk_widget_get_mapped (widget)) {
    return;
  }

  gtk_widget_set_mapped (widget, FALSE);

  window = gtk_widget_get_window (widget);

  gdk_window_hide (window);
}


static gboolean
dia_list_button_press (GtkWidget      *widget,
                       GdkEventButton *event)
{
  GtkWidget *item;

  if (event->button != 1) {
    return FALSE;
  }

  item = gtk_get_event_widget ((GdkEvent*) event);

  while (item && !DIA_IS_LAYER_WIDGET (item)) {
    item = gtk_widget_get_parent (item);
  }

  if (item && (gtk_widget_get_parent (item) == widget)) {
    if (!gtk_widget_has_focus (item)) {
      gtk_widget_grab_focus (item);
    }

    return TRUE;
  }

  return FALSE;
}


static void
dia_list_style_set (GtkWidget *widget,
                    GtkStyle  *previous_style)
{
  GtkStyle *style;

  if (previous_style && gtk_widget_get_realized (widget)) {
    style = gtk_widget_get_style (widget);
    gdk_window_set_background (gtk_widget_get_window (widget),
                               &style->base[gtk_widget_get_state (widget)]);
  }
}


static void
dia_list_add (GtkContainer *container,
              GtkWidget    *widget)
{
  GList *item_list;

  g_return_if_fail (DIA_IS_LAYER_WIDGET (widget));

  item_list = g_list_alloc ();
  item_list->data = widget;

  dia_list_append_items (DIA_LIST (container), item_list);
}


static void
dia_list_remove (GtkContainer *container,
                 GtkWidget    *widget)
{
  GList *item_list;

  g_return_if_fail (container == GTK_CONTAINER (gtk_widget_get_parent (widget)));

  item_list = g_list_alloc ();
  item_list->data = widget;

  dia_list_remove_items (DIA_LIST (container), item_list);

  g_list_free (item_list);
}


static void
dia_list_forall (GtkContainer  *container,
                 gboolean       include_internals,
                 GtkCallback    callback,
                 gpointer       callback_data)
{
  DiaList *list = DIA_LIST (container);
  DiaListPrivate *priv = dia_list_get_instance_private (list);
  GtkWidget *child;
  GList *children;

  children = priv->children;

  while (children) {
    child = children->data;
    children = children->next;

    (* callback) (child, callback_data);
  }
}


static GType
dia_list_child_type (GtkContainer *container)
{
  return DIA_TYPE_LAYER_WIDGET;
}


static void
dia_list_set_focus_child (GtkContainer *container,
                          GtkWidget    *child)
{
  DiaList *list;
  GtkWidget *focus_child;
  DiaListPrivate *priv;

  g_return_if_fail (DIA_IS_LIST (container));

  if (child) {
    g_return_if_fail (GTK_IS_WIDGET (child));
  }

  list = DIA_LIST (container);
  priv = dia_list_get_instance_private (list);

  focus_child = gtk_container_get_focus_child (container);

  if (child != focus_child) {
    if (focus_child) {
      priv->last_focus_child = focus_child;
      g_clear_object (&focus_child);
    }
    GTK_CONTAINER_CLASS (dia_list_parent_class)->set_focus_child (container, child);
    if (child) {
      g_object_ref (child);
    }
  }

  /* check for v adjustment */
  if (child) {
    GtkAdjustment *adjustment;

    adjustment = gtk_container_get_focus_vadjustment (container);
    if (adjustment) {
      GtkAllocation alloc;

      gtk_widget_get_allocation (child, &alloc);

      gtk_adjustment_clamp_page (adjustment,
                                 alloc.y,
                                 (alloc.y +
                                 alloc.height));
    }

    dia_list_select_child (list, DIA_LAYER_WIDGET (child));
  }
}


static gboolean
dia_list_focus (GtkWidget        *widget,
                GtkDirectionType  direction)
{
  gint return_val = FALSE;
  GtkContainer *container;
  GtkWidget *focus_child;
  DiaListPrivate *priv;

  g_return_val_if_fail (DIA_IS_LIST (widget), FALSE);

  priv = dia_list_get_instance_private (DIA_LIST (widget));

  container = GTK_CONTAINER (widget);
  focus_child = gtk_container_get_focus_child (container);

  if (focus_child == NULL || !gtk_widget_has_focus (focus_child)) {
    if (priv->last_focus_child) {
      gtk_container_set_focus_child (container, priv->last_focus_child);
    }

    if (GTK_WIDGET_CLASS (dia_list_parent_class)->focus) {
      return_val = GTK_WIDGET_CLASS (dia_list_parent_class)->focus (widget,
                                                                        direction);
    }
  }

  if (!return_val) {
    focus_child = gtk_container_get_focus_child (GTK_CONTAINER (container));

    if (focus_child) {
      priv->last_focus_child = focus_child;
    }
  }

  return return_val;
}


static void
dia_list_class_init (DiaListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->dispose = dia_list_dispose;

  widget_class->unmap = dia_list_unmap;
  widget_class->style_set = dia_list_style_set;
  widget_class->realize = dia_list_realize;
  widget_class->button_press_event = dia_list_button_press;
  widget_class->size_request = dia_list_size_request;
  widget_class->size_allocate = dia_list_size_allocate;
  widget_class->focus = dia_list_focus;

  container_class->add = dia_list_add;
  container_class->remove = dia_list_remove;
  container_class->forall = dia_list_forall;
  container_class->child_type = dia_list_child_type;
  container_class->set_focus_child = dia_list_set_focus_child;
}


static void
dia_list_init (DiaList *list)
{
  DiaListPrivate *priv = dia_list_get_instance_private (list);

  priv->children = NULL;
  priv->selected = NULL;

  priv->last_focus_child = NULL;
}


GtkWidget *
dia_list_new (void)
{
  return g_object_new (DIA_TYPE_LIST, NULL);
}


static void
scroll_vertical (DiaLayerWidget *list_item,
                 GtkScrollType   scroll_type,
                 double          position,
                 DiaList        *list)
{
  GtkContainer *container;
  DiaListPrivate *priv;
  GList *work;
  GtkWidget *item;
  GtkAdjustment *adj;
  GtkWidget *focus_child;
  GtkAllocation alloc;
  gint new_value;

  g_return_if_fail (DIA_IS_LIST (list));

  priv = dia_list_get_instance_private (list);

  container = GTK_CONTAINER (list);
  focus_child = gtk_container_get_focus_child (container);

  if (focus_child) {
    work = g_list_find (priv->children, focus_child);
  } else {
    work = priv->children;
  }

  if (!work) {
    return;
  }

  switch (scroll_type) {
    case GTK_SCROLL_STEP_BACKWARD:
      work = work->prev;
      if (work) {
        gtk_widget_grab_focus (GTK_WIDGET (work->data));
      }
      break;
    case GTK_SCROLL_STEP_FORWARD:
      work = work->next;
      if (work) {
        gtk_widget_grab_focus (GTK_WIDGET (work->data));
      }
      break;
    case GTK_SCROLL_PAGE_BACKWARD:
      if (!work->prev) {
        return;
      }
      item = work->data;
      adj = gtk_container_get_focus_vadjustment (GTK_CONTAINER (list));

      if (adj) {
        gboolean correct = FALSE;
        double value = gtk_adjustment_get_value (adj);
        double page_size = gtk_adjustment_get_page_size (adj);
        double lower = gtk_adjustment_get_lower (adj);

        gtk_widget_get_allocation (item, &alloc);

        new_value = value;

        if (alloc.y <= value) {
          new_value = MAX (alloc.y + alloc.height - page_size, lower);
          correct = TRUE;
        }

        if (alloc.y > new_value) {
          for (; work; work = work->prev) {
            item = GTK_WIDGET (work->data);

            gtk_widget_get_allocation (item, &alloc);

            if (alloc.y <= new_value &&
                alloc.y + alloc.height > new_value) {
              break;
            }
          }
        } else {
          for (; work; work = work->next) {
            item = GTK_WIDGET (work->data);

            gtk_widget_get_allocation (item, &alloc);

            if (alloc.y <= new_value &&
                alloc.y + alloc.height > new_value) {
              break;
            }
          }
        }

        gtk_widget_get_allocation (item, &alloc);

        if (correct && work && work->next && alloc.y < new_value) {
          item = work->next->data;
        }
      } else {
        item = priv->children->data;
      }

      gtk_widget_grab_focus (item);
      break;
    case GTK_SCROLL_PAGE_FORWARD:
      if (!work->next) {
        return;
      }

      item = work->data;
      adj = gtk_container_get_focus_vadjustment (GTK_CONTAINER (list));

      if (adj) {
        gboolean correct = FALSE;
        double value = gtk_adjustment_get_value (adj);
        double upper = gtk_adjustment_get_upper (adj);
        double page_size = gtk_adjustment_get_page_size (adj);

        new_value = value;

        gtk_widget_get_allocation (item, &alloc);

        if (alloc.y + alloc.height >=
            value + page_size) {
          new_value = alloc.y;
          correct = TRUE;
        }

        new_value = MIN (new_value + page_size, upper);

        if (alloc.y > new_value) {
          for (; work; work = work->prev) {
            item = GTK_WIDGET (work->data);

            gtk_widget_get_allocation (item, &alloc);

            if (alloc.y <= new_value &&
                alloc.y + alloc.height > new_value) {
              break;
            }
          }
        } else {
          for (; work; work = work->next) {
            item = GTK_WIDGET (work->data);

            gtk_widget_get_allocation (item, &alloc);

            if (alloc.y <= new_value &&
                alloc.y + alloc.height > new_value) {
              break;
            }
          }
        }

        gtk_widget_get_allocation (item, &alloc);

        if (correct && work && work->prev &&
            alloc.y + alloc.height - 1 > new_value) {
          item = work->prev->data;
        }
      } else {
        item = g_list_last (work)->data;
      }

      gtk_widget_grab_focus (item);
      break;
    case GTK_SCROLL_JUMP:
      gtk_widget_get_allocation (GTK_WIDGET (list), &alloc);

      new_value = alloc.height * CLAMP (position, 0, 1);

      for (item = NULL, work = priv->children; work; work = work->next) {
        item = GTK_WIDGET (work->data);

        gtk_widget_get_allocation (item, &alloc);

        if (alloc.y <= new_value &&
            alloc.y + alloc.height > new_value) {
          break;
        }
      }

      gtk_widget_grab_focus (item);
      break;
    case GTK_SCROLL_STEP_UP:
    case GTK_SCROLL_STEP_DOWN:
    case GTK_SCROLL_STEP_LEFT:
    case GTK_SCROLL_STEP_RIGHT:
    case GTK_SCROLL_PAGE_UP:
    case GTK_SCROLL_PAGE_DOWN:
    case GTK_SCROLL_PAGE_LEFT:
    case GTK_SCROLL_PAGE_RIGHT:
    case GTK_SCROLL_START:
    case GTK_SCROLL_END:
    case GTK_SCROLL_NONE:
    default:
      break;
  }
}


void
dia_list_insert_items (DiaList *list,
                       GList   *items,
                       int      position)
{
  GtkWidget *widget;
  DiaListPrivate *priv;
  GList *tmp_list;
  GList *last;
  int nchildren;

  g_return_if_fail (DIA_IS_LIST (list));

  if (!items) {
    return;
  }

  tmp_list = items;
  while (tmp_list) {
    widget = tmp_list->data;
    tmp_list = tmp_list->next;

    gtk_widget_set_parent (widget, GTK_WIDGET (list));

    g_signal_connect (widget,
                      "scroll-vertical",
                      G_CALLBACK (scroll_vertical),
                      list);
  }

  priv = dia_list_get_instance_private (list);

  nchildren = g_list_length (priv->children);
  if ((position < 0) || (position > nchildren))
    position = nchildren;

  if (position == nchildren) {
    if (priv->children) {
      tmp_list = g_list_last (priv->children);
      tmp_list->next = items;
      items->prev = tmp_list;
    } else {
      priv->children = items;
    }
  } else {
    tmp_list = g_list_nth (priv->children, position);
    last = g_list_last (items);

    if (tmp_list->prev) {
      tmp_list->prev->next = items;
    }
    last->next = tmp_list;
    items->prev = tmp_list->prev;
    tmp_list->prev = last;

    if (tmp_list == priv->children) {
      priv->children = items;
    }
  }

  if (priv->children && !priv->selected) {
    widget = priv->children->data;
    dia_list_select_child (list, DIA_LAYER_WIDGET (widget));
  }
}


void
dia_list_append_items (DiaList *list,
                       GList   *items)
{
  g_return_if_fail (DIA_IS_LIST (list));

  dia_list_insert_items (list, items, -1);
}


void
dia_list_prepend_items (DiaList *list,
                        GList   *items)
{
  g_return_if_fail (DIA_IS_LIST (list));

  dia_list_insert_items (list, items, 0);
}


void
dia_list_remove_items (DiaList *list,
                       GList   *items)
{
  GtkWidget *widget;
  DiaListPrivate *priv;
  GtkWidget *new_focus_child;
  GtkWidget *old_focus_child;
  GtkWidget *focus_child;
  GtkContainer *container;
  GList *tmp_list;
  GList *work;
  gboolean grab_focus = FALSE;

  g_return_if_fail (DIA_IS_LIST (list));

  priv = dia_list_get_instance_private (list);

  if (!items) {
    return;
  }

  container = GTK_CONTAINER (list);
  focus_child = gtk_container_get_focus_child (container);

  tmp_list = items;
  while (tmp_list) {
    widget = tmp_list->data;
    tmp_list = tmp_list->next;

    if (gtk_widget_get_state (widget) == GTK_STATE_SELECTED) {
      dia_list_unselect_child (list, DIA_LAYER_WIDGET (widget));
    }
  }

  if (focus_child) {
    old_focus_child = new_focus_child = focus_child;
    if (gtk_widget_has_focus (focus_child)) {
      grab_focus = TRUE;
    }
  } else {
    old_focus_child = new_focus_child = priv->last_focus_child;
  }

  tmp_list = items;
  while (tmp_list) {
    widget = tmp_list->data;
    tmp_list = tmp_list->next;

    g_object_ref (widget);

    if (widget == new_focus_child) {
      work = g_list_find (priv->children, widget);

      if (work) {
        if (work->next) {
          new_focus_child = work->next->data;
        } else if (priv->children != work && work->prev) {
          new_focus_child = work->prev->data;
        } else {
          new_focus_child = NULL;
        }
      }
    }

    g_signal_handlers_disconnect_by_data (widget, list);
    priv->children = g_list_remove (priv->children, widget);
    gtk_widget_unparent (widget);

    if (widget == priv->last_focus_child) {
      priv->last_focus_child = NULL;
    }

    g_clear_object (&widget);
  }

  focus_child = gtk_container_get_focus_child (container);

  if (new_focus_child && new_focus_child != old_focus_child) {
    if (grab_focus) {
      gtk_widget_grab_focus (new_focus_child);
    } else if (focus_child) {
      gtk_container_set_focus_child (container, new_focus_child);
    }

    if (!priv->selected) {
      priv->last_focus_child = new_focus_child;
      dia_list_select_child (list, DIA_LAYER_WIDGET (new_focus_child));
    }
  }

  if (gtk_widget_get_visible (GTK_WIDGET (list))) {
    gtk_widget_queue_resize (GTK_WIDGET (list));
  }
}


void
dia_list_clear_items (DiaList *list,
                      int         start,
                      int         end)
{
  GtkContainer *container;
  GtkWidget *widget;
  DiaListPrivate *priv;
  GtkWidget *new_focus_child = NULL;
  GtkWidget *focus_child;
  GList *start_list;
  GList *end_list;
  GList *tmp_list;
  guint nchildren;
  gboolean grab_focus = FALSE;

  g_return_if_fail (DIA_IS_LIST (list));

  priv = dia_list_get_instance_private (list);

  nchildren = g_list_length (priv->children);

  if (nchildren == 0) {
    return;
  }

  if ((end < 0) || (end > nchildren)) {
    end = nchildren;
  }

  if (start >= end) {
    return;
  }

  container = GTK_CONTAINER (list);

  start_list = g_list_nth (priv->children, start);
  end_list = g_list_nth (priv->children, end);

  if (start_list->prev) {
    start_list->prev->next = end_list;
  }
  if (end_list && end_list->prev) {
    end_list->prev->next = NULL;
  }
  if (end_list) {
    end_list->prev = start_list->prev;
  }
  if (start_list == priv->children) {
    priv->children = end_list;
  }

  focus_child = gtk_container_get_focus_child (container);

  if (focus_child) {
    if (g_list_find (start_list, focus_child)) {
      if (start_list->prev) {
        new_focus_child = start_list->prev->data;
      } else if (priv->children) {
        new_focus_child = priv->children->data;
      }

      if (gtk_widget_has_focus (focus_child)) {
        grab_focus = TRUE;
      }
    }
  }

  tmp_list = start_list;
  while (tmp_list) {
    widget = tmp_list->data;
    tmp_list = tmp_list->next;

    g_object_ref (widget);

    if (gtk_widget_get_state (widget) == GTK_STATE_SELECTED) {
      dia_list_unselect_child (list, DIA_LAYER_WIDGET (widget));
    }

    g_signal_handlers_disconnect_by_data (widget, list);
    gtk_widget_unparent (widget);

    if (widget == priv->last_focus_child) {
      priv->last_focus_child = NULL;
    }

    g_clear_object (&widget);
  }

  g_list_free (start_list);

  if (new_focus_child) {
    if (grab_focus) {
      gtk_widget_grab_focus (new_focus_child);
    } else if (focus_child) {
      gtk_container_set_focus_child (container, new_focus_child);
    }

    if (!priv->selected) {
      priv->last_focus_child = new_focus_child;
      dia_list_select_child (list, DIA_LAYER_WIDGET (new_focus_child));
    }
  }

  if (gtk_widget_get_visible (GTK_WIDGET (list))) {
    gtk_widget_queue_resize (GTK_WIDGET (list));
  }
}


void
dia_list_select_item (DiaList *list,
                      int      item)
{
  GList *tmp_list;
  DiaListPrivate *priv;

  g_return_if_fail (DIA_IS_LIST (list));

  priv = dia_list_get_instance_private (list);

  tmp_list = g_list_nth (priv->children, item);
  if (tmp_list) {
    dia_list_select_child (list, tmp_list->data);
  }
}


void
dia_list_select_child (DiaList        *self,
                       DiaLayerWidget *item)
{
  DiaListPrivate *priv;
  DiaLayerWidget *old = NULL;

  g_return_if_fail (DIA_IS_LIST (self));
  g_return_if_fail (DIA_IS_LAYER_WIDGET (item));

  priv = dia_list_get_instance_private (self);

  if (priv->selected) {
    old = g_object_ref (priv->selected);
  }

  if (g_set_object (&priv->selected, item)) {
    if (old) {
      gtk_widget_set_state (GTK_WIDGET (old), GTK_STATE_NORMAL);
    }

    gtk_widget_set_state (GTK_WIDGET (item), GTK_STATE_SELECTED);

    dia_layer_widget_select (item);
  }

  g_clear_object (&old);
}


void
dia_list_unselect_child (DiaList        *self,
                         DiaLayerWidget *item)
{
  DiaListPrivate *priv;

  g_return_if_fail (DIA_IS_LIST (self));
  g_return_if_fail (DIA_IS_LAYER_WIDGET (item));

  priv = dia_list_get_instance_private (self);

  g_return_if_fail (priv->selected == item);

  gtk_widget_set_state (GTK_WIDGET (priv->selected), GTK_STATE_NORMAL);

  g_clear_object (&priv->selected);

  dia_layer_widget_deselect (item);
}


int
dia_list_child_position (DiaList        *self,
                         DiaLayerWidget *item)
{
  DiaListPrivate *priv;
  GList *children;
  int pos;

  g_return_val_if_fail (DIA_IS_LIST (self), -1);
  g_return_val_if_fail (DIA_IS_LAYER_WIDGET (item), -1);

  priv = dia_list_get_instance_private (self);

  pos = 0;
  children = priv->children;

  while (children) {
    if (item == children->data) {
      return pos;
    }

    pos += 1;
    children = children->next;
  }

  return -1;
}


DiaLayerWidget *
dia_list_get_selected (DiaList *self)
{
  DiaListPrivate *priv;

  g_return_val_if_fail (DIA_IS_LIST (self), NULL);

  priv = dia_list_get_instance_private (self);

  return priv->selected;
}
