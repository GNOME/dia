/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
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
 *
 * © 2020 Zander Brown <zbrown@gnome.org>
 * © 2023 Hubert Figuière <hub@figuiere.net>
 */

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

#include "dia-layer-list.h"
#include "dia-layer-widget.h"


typedef struct _DiaLayerListPrivate DiaLayerListPrivate;
struct _DiaLayerListPrivate {
  GtkContainer container;

  DiagramData *diagram;

  GList *children;
  DiaLayerWidget *selected;

  GtkWidget *last_focus_child;
};


G_DEFINE_TYPE_WITH_PRIVATE (DiaLayerList, dia_layer_list, GTK_TYPE_CONTAINER)


enum {
  PROP_0,
  PROP_DIAGRAM,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };



static void            dia_layer_list_insert_items   (DiaLayerList        *self,
                                                      GList               *items,
                                                      int                  position);
static void            dia_layer_list_remove_items   (DiaLayerList        *self,
                                                      GList               *items);
static void            dia_layer_list_select_item    (DiaLayerList        *self,
                                                      int                  item);
static void            dia_layer_list_select_child   (DiaLayerList        *self,
                                                      DiaLayerWidget      *item);
static void            dia_layer_list_unselect_child (DiaLayerList        *self,
                                                      DiaLayerWidget      *item);

static void
dia_layer_list_finalize (GObject *object)
{
  DiaLayerList *list = DIA_LAYER_LIST (object);
  DiaLayerListPrivate *priv = dia_layer_list_get_instance_private (list);

  g_clear_pointer (&priv->children, g_list_free);
  g_clear_object (&priv->selected);

  G_OBJECT_CLASS (dia_layer_list_parent_class)->finalize (object);
}


static void
dia_layer_list_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  DiaLayerList *self = DIA_LAYER_LIST (object);

  switch (property_id) {
    case PROP_DIAGRAM:
      dia_layer_list_set_diagram (self, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_layer_list_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  DiaLayerList *self = DIA_LAYER_LIST (object);

  switch (property_id) {
    case PROP_DIAGRAM:
      g_value_set_object (value, dia_layer_list_get_diagram (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_layer_list_size_request (GtkWidget      *widget,
                             GtkRequisition *requisition)
{
  DiaLayerList *list = DIA_LAYER_LIST (widget);
  DiaLayerListPrivate *priv = dia_layer_list_get_instance_private (list);
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

      gtk_widget_get_preferred_size (child, NULL, &child_requisition);

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
dia_layer_list_get_preferred_width (GtkWidget      *widget,
                                    gint           *minimal_width,
                                    gint           *natural_width)
{
  GtkRequisition requisition;

  dia_layer_list_size_request (widget, &requisition);

  *minimal_width = *natural_width = requisition.width;
}


static void
dia_layer_list_get_preferred_height (GtkWidget      *widget,
                                     gint           *minimal_height,
                                     gint           *natural_height)
{
  GtkRequisition requisition;

  dia_layer_list_size_request (widget, &requisition);

  *minimal_height = *natural_height = requisition.height;
}


static void
dia_layer_list_size_allocate (GtkWidget     *widget,
                              GtkAllocation *allocation)
{
  DiaLayerList *list = DIA_LAYER_LIST (widget);
  DiaLayerListPrivate *priv = dia_layer_list_get_instance_private (list);
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

          gtk_widget_get_preferred_size (child, NULL, &child_requisition);

          child_allocation.height = child_requisition.height;

          gtk_widget_size_allocate (child, &child_allocation);

          child_allocation.y += child_allocation.height;
        }
    }
  }
}


static void
dia_layer_list_realize (GtkWidget *widget)
{
  GtkAllocation alloc;
  GdkWindowAttr attributes;
  int attributes_mask;
  GdkWindow *window;
  GtkStyleContext *style;
  GdkRGBA fg;

  gtk_widget_set_realized (widget, TRUE);
  gtk_widget_get_allocation (widget, &alloc);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = alloc.x;
  attributes.y = alloc.y;
  attributes.width = alloc.width;
  attributes.height = alloc.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK;

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;


  window = gdk_window_new (gtk_widget_get_parent_window (widget),
                           &attributes, attributes_mask);
  gtk_widget_set_window (widget, window);
  gdk_window_set_user_data (window, widget);

  style = gtk_widget_get_style_context (widget);

  gtk_style_context_get_color (style, GTK_STATE_FLAG_NORMAL, &fg);
  gdk_window_set_background_rgba (window, &fg);
}


static void
dia_layer_list_unmap (GtkWidget *widget)
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
dia_layer_list_button_press (GtkWidget      *widget,
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
dia_layer_list_style_set (GtkWidget *widget,
                          GtkStyle  *previous_style)
{
  GtkStyleContext *style;

  if (previous_style && gtk_widget_get_realized (widget)) {
    GdkRGBA bg;
    style = gtk_widget_get_style_context (widget);
    gtk_style_context_get_background_color (style, gtk_widget_get_state_flags (widget), &bg);
    gdk_window_set_background_rgba (gtk_widget_get_window (widget), &bg);
  }
}


static void
dia_layer_list_add (GtkContainer *container,
                    GtkWidget    *widget)
{
  GList *item_list;

  g_return_if_fail (DIA_IS_LAYER_WIDGET (widget));

  item_list = g_list_alloc ();
  item_list->data = widget;

  dia_layer_list_insert_items (DIA_LAYER_LIST (container), item_list, -1);
}


static void
dia_layer_list_remove (GtkContainer *container,
                       GtkWidget    *widget)
{
  GList *item_list;

  g_return_if_fail (container == GTK_CONTAINER (gtk_widget_get_parent (widget)));

  item_list = g_list_alloc ();
  item_list->data = widget;

  dia_layer_list_remove_items (DIA_LAYER_LIST (container), item_list);

  g_list_free (item_list);
}


static void
dia_layer_list_forall (GtkContainer *container,
                       gboolean      include_internals,
                       GtkCallback   callback,
                       gpointer      callback_data)
{
  DiaLayerList *list = DIA_LAYER_LIST (container);
  DiaLayerListPrivate *priv = dia_layer_list_get_instance_private (list);
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
dia_layer_list_child_type (GtkContainer *container)
{
  return DIA_TYPE_LAYER_WIDGET;
}


static void
dia_layer_list_set_focus_child (GtkContainer *container,
                                GtkWidget    *child)
{
  DiaLayerList *list;
  GtkWidget *focus_child;
  DiaLayerListPrivate *priv;

  g_return_if_fail (DIA_IS_LAYER_LIST (container));

  if (child) {
    g_return_if_fail (GTK_IS_WIDGET (child));
  }

  list = DIA_LAYER_LIST (container);
  priv = dia_layer_list_get_instance_private (list);

  focus_child = gtk_container_get_focus_child (container);

  if (child != focus_child) {
    if (focus_child) {
      priv->last_focus_child = focus_child;
      g_clear_object (&focus_child);
    }
    GTK_CONTAINER_CLASS (dia_layer_list_parent_class)->set_focus_child (container, child);
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

    dia_layer_list_select_child (list, DIA_LAYER_WIDGET (child));
  }
}


static gboolean
dia_layer_list_focus (GtkWidget        *widget,
                      GtkDirectionType  direction)
{
  int return_val = FALSE;
  GtkContainer *container;
  GtkWidget *focus_child;
  DiaLayerListPrivate *priv;

  g_return_val_if_fail (DIA_IS_LAYER_LIST (widget), FALSE);

  priv = dia_layer_list_get_instance_private (DIA_LAYER_LIST (widget));

  container = GTK_CONTAINER (widget);
  focus_child = gtk_container_get_focus_child (container);

  if (focus_child == NULL || !gtk_widget_has_focus (focus_child)) {
    if (priv->last_focus_child) {
      gtk_container_set_focus_child (container, priv->last_focus_child);
    }

    if (GTK_WIDGET_CLASS (dia_layer_list_parent_class)->focus) {
      return_val = GTK_WIDGET_CLASS (dia_layer_list_parent_class)->focus (widget,
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

static int
dia_layer_list_draw (GtkWidget      *widget,
                     cairo_t        *ctx)
{
    if (gtk_widget_is_drawable (widget)) {
      GtkStyleContext *style = gtk_widget_get_style_context (widget);

      int height = gtk_widget_get_allocated_height (widget);
      int width = gtk_widget_get_allocated_width (widget);

      gtk_render_background (style, ctx, 0, 0, width, height);

      return GTK_WIDGET_CLASS (dia_layer_list_parent_class)->draw (widget, ctx);
    }

    return FALSE;
}

static void
dia_layer_list_class_init (DiaLayerListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->finalize = dia_layer_list_finalize;
  object_class->get_property = dia_layer_list_get_property;
  object_class->set_property = dia_layer_list_set_property;

  gtk_widget_class_set_css_name (widget_class, "list");

  widget_class->unmap = dia_layer_list_unmap;
  widget_class->style_set = dia_layer_list_style_set;
  widget_class->realize = dia_layer_list_realize;
  widget_class->button_press_event = dia_layer_list_button_press;
  widget_class->get_preferred_width = dia_layer_list_get_preferred_width;
  widget_class->get_preferred_height = dia_layer_list_get_preferred_height;
  widget_class->size_allocate = dia_layer_list_size_allocate;
  widget_class->focus = dia_layer_list_focus;
  widget_class->draw = dia_layer_list_draw;

  container_class->add = dia_layer_list_add;
  container_class->remove = dia_layer_list_remove;
  container_class->forall = dia_layer_list_forall;
  container_class->child_type = dia_layer_list_child_type;
  container_class->set_focus_child = dia_layer_list_set_focus_child;

  /**
   * DiaLayerList:diagram:
   *
   * Since: 0.98
   */
  pspecs[PROP_DIAGRAM] =
    g_param_spec_object ("diagram",
                         "Diagram",
                         "The current diagram",
                         DIA_TYPE_DIAGRAM_DATA,
                         G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}


static void
dia_layer_list_init (DiaLayerList *list)
{
  GtkStyleContext *style = gtk_widget_get_style_context (GTK_WIDGET (list));
  gtk_style_context_add_class (style, "view");
}


GtkWidget *
dia_layer_list_new (void)
{
  return g_object_new (DIA_TYPE_LAYER_LIST, NULL);
}


static void
exclusive_connectable (DiaLayerWidget *layer_row,
                       DiaLayerList   *self)
{
  DiaLayerListPrivate *priv = dia_layer_list_get_instance_private (self);
  GList *list;
  DiaLayerWidget *lw;
  int connectable = FALSE;

  /*  First determine if _any_ other layer widgets are set to connectable  */
  DIA_FOR_LAYER_IN_DIAGRAM (priv->diagram, layer, i, {
    if (dia_layer_widget_get_layer (layer_row) != layer) {
      connectable |= dia_layer_is_connectable (layer);
    }
  });

  /*  Now, toggle the connectability for all layers except the specified one  */
  list = priv->children;
  while (list) {
    lw = DIA_LAYER_WIDGET (list->data);
    if (lw != layer_row) {
      dia_layer_widget_set_connectable (lw, !connectable);
    } else {
      dia_layer_widget_set_connectable (lw, TRUE);
    }
    gtk_widget_queue_draw (GTK_WIDGET (lw));

    list = g_list_next (list);
  }
}


static void
layers_changed (DiagramData  *diagram,
                guint         pos,
                guint         removed,
                guint         added,
                DiaLayerList *self)
{
  DiaLayerListPrivate *priv;
  GtkWidget *widget;
  DiaLayer *layer;
  GList *list = NULL;

  g_return_if_fail (DIA_IS_LAYER_LIST (self));

  priv = dia_layer_list_get_instance_private (self);

  for (int i = 0; i < removed; i++) {
    GList *children = priv->children;
    children = g_list_nth (children, pos + i);

    if (children->data) {
      list = g_list_append (list, children->data);
    }
  }

  dia_layer_list_remove_items (self, list);

  list = NULL;

  for (int i = 0; i < added; i++) {
    layer = data_layer_get_nth (diagram, pos + i);
    widget = dia_layer_widget_new (layer);
    g_signal_connect (widget,
                      "exclusive",
                      G_CALLBACK (exclusive_connectable),
                      self);
    gtk_widget_show (widget);
    list = g_list_append (list, widget);
  }

  dia_layer_list_insert_items (self, list, pos);
}


static void
active_changed (DiagramData  *diagram,
                GParamSpec   *pspec,
                DiaLayerList *self)
{
  DiaLayer *active = dia_diagram_data_get_active_layer (diagram);
  guint pos = data_layer_get_index (diagram, active);

  dia_layer_list_select_item (self, pos);
}


void
dia_layer_list_set_diagram (DiaLayerList *self,
                            DiagramData  *diagram)
{
  DiaLayerListPrivate *priv;
  GtkWidget *layer_widget;
  DiagramData *old = NULL;

  g_return_if_fail (DIA_IS_LAYER_LIST (self));

  priv = dia_layer_list_get_instance_private (self);

  if (priv->diagram) {
    old = g_object_ref (priv->diagram);
  }

  if (g_set_object (&priv->diagram, diagram)) {
    gtk_container_foreach (GTK_CONTAINER (self),
                           (GtkCallback) gtk_widget_destroy,
                           NULL);

    if (old) {
      g_object_disconnect (old,
                           "any-signal::layers-changed",
                           G_CALLBACK (layers_changed),
                           self,
                           "any-signal::notify::active-layer",
                           G_CALLBACK (active_changed),
                           self,
                           NULL);

      g_clear_object (&old);
    }

    if (diagram == NULL) {
      gtk_widget_set_sensitive (GTK_WIDGET (self), FALSE);

      g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_DIAGRAM]);

      return;
    }

    gtk_widget_set_sensitive (GTK_WIDGET (self), TRUE);

    DIA_FOR_LAYER_IN_DIAGRAM (diagram, layer, i, {
      layer_widget = dia_layer_widget_new (layer);
      g_signal_connect (layer_widget,
                        "exclusive",
                        G_CALLBACK (exclusive_connectable),
                        self);
      gtk_widget_show (layer_widget);

      gtk_container_add (GTK_CONTAINER (self),
                         layer_widget);
    });

    g_object_connect (diagram,
                      "signal::layers-changed",
                      G_CALLBACK (layers_changed),
                      self,
                      "signal::notify::active-layer",
                      G_CALLBACK (active_changed),
                      self,
                      NULL);

    g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_DIAGRAM]);
  }

  g_clear_object (&old);
}


DiagramData *
dia_layer_list_get_diagram (DiaLayerList *self)
{
  DiaLayerListPrivate *priv;

  g_return_val_if_fail (DIA_IS_LAYER_LIST (self), NULL);

  priv = dia_layer_list_get_instance_private (self);

  return priv->diagram;
}


static void
scroll_vertical (DiaLayerWidget *list_item,
                 GtkScrollType   scroll_type,
                 double          position,
                 DiaLayerList   *list)
{
  GtkContainer *container;
  DiaLayerListPrivate *priv;
  GList *work;
  GtkWidget *item;
  GtkAdjustment *adj;
  GtkWidget *focus_child;
  GtkAllocation alloc;
  int new_value;

  g_return_if_fail (DIA_IS_LAYER_LIST (list));

  priv = dia_layer_list_get_instance_private (list);

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


static void
dia_layer_list_insert_items (DiaLayerList *list,
                             GList        *items,
                             int           position)
{
  GtkWidget *widget;
  DiaLayerListPrivate *priv;
  GList *tmp_list;
  GList *last;
  int nchildren;

  g_return_if_fail (DIA_IS_LAYER_LIST (list));

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

  priv = dia_layer_list_get_instance_private (list);

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
    DiaLayer *active = dia_diagram_data_get_active_layer (priv->diagram);

    dia_layer_list_select_item (list,
                                data_layer_get_index (priv->diagram, active));
  }

  gtk_widget_queue_resize (GTK_WIDGET (list));
}


static void
dia_layer_list_remove_items (DiaLayerList *list,
                             GList        *items)
{
  GtkWidget *widget;
  DiaLayerListPrivate *priv;
  GtkWidget *new_focus_child;
  GtkWidget *old_focus_child;
  GtkWidget *focus_child;
  GtkContainer *container;
  GList *tmp_list;
  GList *work;
  gboolean grab_focus = FALSE;

  g_return_if_fail (DIA_IS_LAYER_LIST (list));

  priv = dia_layer_list_get_instance_private (list);

  if (!items) {
    return;
  }

  container = GTK_CONTAINER (list);
  focus_child = gtk_container_get_focus_child (container);

  tmp_list = items;
  while (tmp_list) {
    widget = tmp_list->data;
    tmp_list = tmp_list->next;

    if (gtk_widget_get_state_flags (widget) & GTK_STATE_FLAG_SELECTED) {
      dia_layer_list_unselect_child (list, DIA_LAYER_WIDGET (widget));
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
      dia_layer_list_select_child (list, DIA_LAYER_WIDGET (new_focus_child));
    }
  }

  if (gtk_widget_get_visible (GTK_WIDGET (list))) {
    gtk_widget_queue_resize (GTK_WIDGET (list));
  }
}


static void
dia_layer_list_select_item (DiaLayerList *list,
                            int           item)
{
  GList *tmp_list;
  DiaLayerListPrivate *priv;

  g_return_if_fail (DIA_IS_LAYER_LIST (list));

  priv = dia_layer_list_get_instance_private (list);

  tmp_list = g_list_nth (priv->children, item);
  if (tmp_list) {
    dia_layer_list_select_child (list, tmp_list->data);
  }
}


static void
dia_layer_list_select_child (DiaLayerList   *self,
                             DiaLayerWidget *item)
{
  DiaLayerListPrivate *priv;
  DiaLayerWidget *old = NULL;

  g_return_if_fail (DIA_IS_LAYER_LIST (self));
  g_return_if_fail (DIA_IS_LAYER_WIDGET (item));

  priv = dia_layer_list_get_instance_private (self);

  if (priv->selected) {
    old = g_object_ref (priv->selected);
  }

  if (g_set_object (&priv->selected, item)) {
    if (old) {
      gtk_widget_set_state_flags (GTK_WIDGET (old), GTK_STATE_FLAG_NORMAL, TRUE);
    }

    gtk_widget_set_state_flags (GTK_WIDGET (item), GTK_STATE_FLAG_SELECTED, TRUE);

    dia_layer_widget_select (item);
  }

  g_clear_object (&old);
}


static void
dia_layer_list_unselect_child (DiaLayerList   *self,
                               DiaLayerWidget *item)
{
  DiaLayerListPrivate *priv;

  g_return_if_fail (DIA_IS_LAYER_LIST (self));
  g_return_if_fail (DIA_IS_LAYER_WIDGET (item));

  priv = dia_layer_list_get_instance_private (self);

  g_return_if_fail (priv->selected == item);

  gtk_widget_set_state_flags (GTK_WIDGET (priv->selected), GTK_STATE_FLAG_NORMAL, TRUE);

  dia_layer_widget_deselect (priv->selected);

  g_clear_object (&priv->selected);
}
