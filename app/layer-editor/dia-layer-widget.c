/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 * © 2023 Hubert Figuière <hub@figuiere.net>
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
#include <gdk/gdkkeysyms.h>

#include "widgets.h"
#include "dia-layer-widget.h"
#include "layer_dialog.h"

/* The connectability buttons don't quite behave the way they should.
 * The shift-click behavior messes up the active layer.
 * To fix this, we need to rework the code so that the setting of
 * connect_on and connect_off is not tied to the button toggling,
 * but determined by what caused it (creation, user selection,
 * shift-selection).
 */

typedef struct _DiaLayerWidgetPrivate DiaLayerWidgetPrivate;
struct _DiaLayerWidgetPrivate {
  DiaLayer *layer;

  GBinding *name_binding;

  GtkWidget *visible;
  GtkWidget *connectable;
  GtkWidget *label;

  /* If true, the user has set this layers connectivity to on
   * while it was not selected.
   */
  gboolean connect_on;
  /* If true, the user has set this layers connectivity to off
   * while it was selected.
   */
  gboolean connect_off;

  /* If TRUE, we're in the middle of a internal call to
   * dia_layer_widget_*_toggled and should not make undo, update diagram etc.
   *
   * If these calls were not done by simulating button presses, we could avoid
   * this hack.
   */
  gboolean internal_call;

  gboolean shifted;
};

G_DEFINE_TYPE_WITH_PRIVATE (DiaLayerWidget, dia_layer_widget, GTK_TYPE_BIN)


enum {
  EXCLUSIVE,
  SCROLL_VERTICAL,
  LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0, };


enum {
  LW_PROP_0,
  LW_PROP_LAYER,
  LW_PROP_CONNECTABLE,
  LAST_LW_PROP
};
static GParamSpec *lw_pspecs[LAST_LW_PROP] = { NULL, };


static void
dia_layer_widget_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  DiaLayerWidget *self = DIA_LAYER_WIDGET (object);

  switch (property_id) {
    case LW_PROP_LAYER:
      dia_layer_widget_set_layer (self, g_value_get_object (value));
      break;
    case LW_PROP_CONNECTABLE:
      dia_layer_widget_set_connectable (self, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_layer_widget_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  DiaLayerWidget *self = DIA_LAYER_WIDGET (object);

  switch (property_id) {
    case LW_PROP_LAYER:
      g_value_set_object (value, dia_layer_widget_get_layer (self));
      break;
    case LW_PROP_CONNECTABLE:
      g_value_set_boolean (value, dia_layer_widget_get_connectable (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_layer_widget_finalize (GObject *object)
{
  DiaLayerWidget *self = DIA_LAYER_WIDGET (object);
  DiaLayerWidgetPrivate *priv = dia_layer_widget_get_instance_private (self);

  g_clear_object (&priv->layer);

  G_OBJECT_CLASS (dia_layer_widget_parent_class)->finalize (object);
}


static void
dia_layer_widget_realize (GtkWidget *widget)
{
  GdkWindowAttr attributes;
  int attributes_mask;
  GtkAllocation alloc;
  GdkWindow *window;
  GtkStyleContext *style;
  GdkRGBA fg;

  /*GTK_WIDGET_CLASS (parent_class)->realize (widget);*/

  g_return_if_fail (DIA_IS_LAYER_WIDGET (widget));

  gtk_widget_set_realized (widget, TRUE);

  gtk_widget_get_allocation (widget, &alloc);

  attributes.x = alloc.x;
  attributes.y = alloc.y;
  attributes.width = alloc.width;
  attributes.height = alloc.height;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.event_mask = (gtk_widget_get_events (widget) |
                           GDK_EXPOSURE_MASK |
                           GDK_BUTTON_PRESS_MASK |
                           GDK_BUTTON_RELEASE_MASK |
                           GDK_KEY_PRESS_MASK |
                           GDK_KEY_RELEASE_MASK);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;
  window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
  gtk_widget_set_window (widget, window);
  gdk_window_set_user_data (window, widget);

  style = gtk_widget_get_style_context (widget);
  gtk_style_context_get_color (style, GTK_STATE_FLAG_NORMAL, &fg);
  gdk_window_set_background_rgba (window, &fg);
}


static void
dia_layer_widget_size_request (GtkWidget      *widget,
                               GtkRequisition *requisition)
{
  GtkBin *bin;
  GtkWidget *child;
  GtkRequisition child_requisition;
  GtkStyleContext *style;
  GtkBorder padding;
  int focus_width;
  int focus_pad;
  int border_width;

  g_return_if_fail (DIA_IS_LAYER_WIDGET (widget));
  g_return_if_fail (requisition != NULL);

  bin = GTK_BIN (widget);
  gtk_widget_style_get (widget,
                        "focus-line-width", &focus_width,
                        "focus-padding", &focus_pad,
                        NULL);

  style = gtk_widget_get_style_context (widget);
  gtk_style_context_get_padding (style, GTK_STATE_FLAG_NORMAL, &padding);
  border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));

  requisition->width = 2 * (border_width + padding.left + focus_width + focus_pad - 1);
  requisition->height = 2 * (border_width + focus_width + focus_pad - 1);

  child = gtk_bin_get_child (bin);

  if (child && gtk_widget_get_visible (child)) {
    gtk_widget_get_preferred_size (child, NULL, &child_requisition);

    requisition->width += child_requisition.width;
    requisition->height += child_requisition.height;
  }
}


static void
dia_layer_widget_get_preferred_width (GtkWidget *widget,
                                      gint      *minimal_width,
                                      gint      *natural_width)
{
  GtkRequisition requisition;

  dia_layer_widget_size_request (widget, &requisition);

  *minimal_width = *natural_width = requisition.width;
}


static void
dia_layer_widget_get_preferred_height (GtkWidget *widget,
                                       gint      *minimal_height,
                                       gint      *natural_height)
{
  GtkRequisition requisition;

  dia_layer_widget_size_request (widget, &requisition);

  *minimal_height = *natural_height = requisition.height;
}


static void
dia_layer_widget_size_allocate (GtkWidget     *widget,
                                GtkAllocation *allocation)
{
  GtkBin *bin;
  GtkStyleContext *style;
  int border_width;
  GtkAllocation child_allocation;

  g_return_if_fail (DIA_IS_LAYER_WIDGET (widget));
  g_return_if_fail (allocation != NULL);


  gtk_widget_set_allocation (widget, allocation);

  style = gtk_widget_get_style_context (widget);
  border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));

  if (gtk_widget_get_realized (widget)) {
    gdk_window_move_resize (gtk_widget_get_window (widget),
                            allocation->x, allocation->y,
                            allocation->width, allocation->height);
  }

  bin = GTK_BIN (widget);

  if (gtk_bin_get_child (bin)) {
    GtkBorder padding;
    gtk_style_context_get_padding (style, GTK_STATE_FLAG_NORMAL, &padding);
    child_allocation.x = (border_width + padding.left);
    child_allocation.y = border_width;
    child_allocation.width = allocation->width - child_allocation.x * 2;
    child_allocation.height = allocation->height - child_allocation.y * 2;

    gtk_widget_size_allocate (gtk_bin_get_child (bin), &child_allocation);
  }
}


static void
dia_layer_widget_style_set (GtkWidget *widget,
                            GtkStyle  *previous_style)
{
  GtkStyleContext *style;

  g_return_if_fail (widget != NULL);

  if (previous_style && gtk_widget_get_realized (widget)) {
    GdkRGBA bg;
    style = gtk_widget_get_style_context (widget);
    gtk_style_context_get_background_color (style, gtk_widget_get_state_flags (widget), &bg);
    gdk_window_set_background_rgba (gtk_widget_get_window (widget), &bg);
  }
}


static int
dia_layer_widget_draw (GtkWidget      *widget,
                         cairo_t        *ctx)
{
  GtkAllocation alloc;
  GtkStyleContext *style;

  g_return_val_if_fail (widget != NULL, FALSE);

  if (gtk_widget_is_drawable (widget)) {
    style = gtk_widget_get_style_context (widget);

    gtk_widget_get_allocation (widget, &alloc);

    if (gtk_widget_get_state_flags (GTK_WIDGET (widget)) & GTK_STATE_FLAG_NORMAL) {
      gtk_render_background (style, ctx, 0, 0, alloc.width, alloc.height);
    } else {
      gtk_style_context_save (style);
      gtk_style_context_set_state (style, gtk_widget_get_state_flags (GTK_WIDGET (widget)));
      gtk_render_background (style, ctx, 0, 0, alloc.width, alloc.height);
      gtk_render_frame (style, ctx, 0, 0, alloc.width, alloc.height);
      gtk_style_context_restore (style);
    }

    GTK_WIDGET_CLASS (dia_layer_widget_parent_class)->draw (widget, ctx);


    if (gtk_widget_has_focus (widget)) {
      gtk_render_focus (style, ctx,
                        0, 0, alloc.width, alloc.height);
    }
  }

  return FALSE;
}


static void
dia_layer_widget_class_init (DiaLayerWidgetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkBindingSet *binding_set;

  object_class->set_property = dia_layer_widget_set_property;
  object_class->get_property = dia_layer_widget_get_property;
  object_class->finalize = dia_layer_widget_finalize;

  gtk_widget_class_set_css_name (widget_class, "row");

  widget_class->realize = dia_layer_widget_realize;
  widget_class->get_preferred_width = dia_layer_widget_get_preferred_width;
  widget_class->get_preferred_height = dia_layer_widget_get_preferred_height;
  widget_class->size_allocate = dia_layer_widget_size_allocate;
  widget_class->style_set = dia_layer_widget_style_set;
  widget_class->draw = dia_layer_widget_draw;

  signals[EXCLUSIVE] =
    g_signal_new ("exclusive",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL,
                  NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  signals[SCROLL_VERTICAL] =
    g_signal_new ("scroll-vertical",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 2, GTK_TYPE_SCROLL_TYPE, G_TYPE_DOUBLE);

  /**
   * DiaLayerWidget:layer:
   *
   * #DiaLayer to control
   *
   * Since: 0.98
   */
  lw_pspecs[LW_PROP_LAYER] =
    g_param_spec_object ("layer",
                         "Layer",
                         "The layer",
                         DIA_TYPE_LAYER,
                         G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * DiaLayerWidget:connectable:
   *
   * Is the layer connectable
   *
   * Since: 0.98
   */
  lw_pspecs[LW_PROP_CONNECTABLE] =
    g_param_spec_boolean ("connectable",
                          "Connectable",
                          "Is the layer connectable",
                          TRUE,
                          G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_LW_PROP, lw_pspecs);

  binding_set = gtk_binding_set_by_class (klass);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_Up, 0,
                                "scroll-vertical", 2,
                                G_TYPE_ENUM, GTK_SCROLL_STEP_BACKWARD,
                                G_TYPE_DOUBLE, 0.0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Up, 0,
                                "scroll-vertical", 2,
                                G_TYPE_ENUM, GTK_SCROLL_STEP_BACKWARD,
                                G_TYPE_DOUBLE, 0.0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_Down, 0,
                                "scroll-vertical", 2,
                                G_TYPE_ENUM, GTK_SCROLL_STEP_FORWARD,
                                G_TYPE_DOUBLE, 0.0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Down, 0,
                                "scroll-vertical", 2,
                                G_TYPE_ENUM, GTK_SCROLL_STEP_FORWARD,
                                G_TYPE_DOUBLE, 0.0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_Page_Up, 0,
                                "scroll-vertical", 2,
                                G_TYPE_ENUM, GTK_SCROLL_PAGE_BACKWARD,
                                G_TYPE_DOUBLE, 0.0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Page_Up, 0,
                                "scroll-vertical", 2,
                                G_TYPE_ENUM, GTK_SCROLL_PAGE_BACKWARD,
                                G_TYPE_DOUBLE, 0.0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_Page_Down, 0,
                                "scroll-vertical", 2,
                                G_TYPE_ENUM, GTK_SCROLL_PAGE_FORWARD,
                                G_TYPE_DOUBLE, 0.0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Page_Down, 0,
                                "scroll-vertical", 2,
                                G_TYPE_ENUM, GTK_SCROLL_PAGE_FORWARD,
                                G_TYPE_DOUBLE, 0.0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_Home, GDK_CONTROL_MASK,
                                "scroll-vertical", 2,
                                G_TYPE_ENUM, GTK_SCROLL_JUMP,
                                G_TYPE_DOUBLE, 0.0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Home, GDK_CONTROL_MASK,
                                "scroll-vertical", 2,
                                G_TYPE_ENUM, GTK_SCROLL_JUMP,
                                G_TYPE_DOUBLE, 0.0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_End, GDK_CONTROL_MASK,
                                "scroll-vertical", 2,
                                G_TYPE_ENUM, GTK_SCROLL_JUMP,
                                G_TYPE_DOUBLE, 1.0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_End, GDK_CONTROL_MASK,
                                "scroll-vertical", 2,
                                G_TYPE_ENUM, GTK_SCROLL_JUMP,
                                G_TYPE_DOUBLE, 1.0);
}


static gboolean
button_event (GtkWidget      *widget,
              GdkEventButton *event,
              gpointer        userdata)
{
  DiaLayerWidget *self = DIA_LAYER_WIDGET (userdata);
  DiaLayerWidgetPrivate *priv = dia_layer_widget_get_instance_private (self);

  priv->shifted = event->state & GDK_SHIFT_MASK;

  priv->internal_call = FALSE;
  /* Redraw the label? */
  gtk_widget_queue_draw (GTK_WIDGET (self));

  return FALSE;
}


static void
connectable_toggled (GtkToggleButton *widget,
                     gpointer         userdata)
{
  DiaLayerWidget *self = DIA_LAYER_WIDGET (userdata);
  DiaLayerWidgetPrivate *priv = dia_layer_widget_get_instance_private (self);

  if (!priv->layer)
    return;

  if (priv->shifted) {
    priv->shifted = FALSE;
    priv->internal_call = TRUE;
    g_signal_emit (self, signals[EXCLUSIVE], 0);
    priv->internal_call = FALSE;
  } else {
    dia_layer_set_connectable (priv->layer,
                               gtk_toggle_button_get_active (widget));
  }

  if (priv->layer == dia_diagram_data_get_active_layer (dia_layer_get_parent_diagram (priv->layer))) {
    priv->connect_off = !gtk_toggle_button_get_active (widget);
    if (priv->connect_off) {
      priv->connect_on = FALSE;
    }
  } else {
    priv->connect_on = gtk_toggle_button_get_active (widget);
    if (priv->connect_on) {
      priv->connect_off = FALSE;
    }
  }

  gtk_widget_queue_draw (GTK_WIDGET (self));

  if (!priv->internal_call) {
    Diagram *diagram = DIA_DIAGRAM (dia_layer_get_parent_diagram (priv->layer));

    diagram_add_update_all (diagram);
    diagram_flush (diagram);
  }
}


static void
visible_toggled (GtkToggleButton *widget,
                 gpointer         userdata)
{
  DiaLayerWidget *self = DIA_LAYER_WIDGET (userdata);
  DiaLayerWidgetPrivate *priv = dia_layer_widget_get_instance_private (self);
  DiaChange *change;

  /* Have to use this internal_call hack 'cause there's no way to switch
   * a toggle button without causing the 'toggled' event:(
   */
  if (!priv->internal_call) {
    Diagram *dia = DIA_DIAGRAM (dia_layer_get_parent_diagram (priv->layer));
    change = dia_layer_visibility_change_new (dia, priv->layer, priv->shifted);
    /** This apply kills 'lw', thus we have to hold onto 'lw->dia' */
    dia_change_apply (change, DIA_DIAGRAM_DATA (dia));
    undo_set_transactionpoint (dia->undo);
  }
}


static void
dia_layer_widget_init (DiaLayerWidget *self)
{
  DiaLayerWidgetPrivate *priv = dia_layer_widget_get_instance_private (self);
  GtkWidget *hbox;

  gtk_widget_set_has_window (GTK_WIDGET (self), TRUE);
  gtk_widget_set_can_focus (GTK_WIDGET (self), TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

  gtk_widget_set_margin_top (hbox, 3);
  gtk_widget_set_margin_bottom (hbox, 3);

  priv->internal_call = FALSE;
  priv->shifted = FALSE;

  priv->layer = NULL;

  priv->connect_on = FALSE;
  priv->connect_off = FALSE;

  priv->visible = dia_toggle_button_new_with_icon_names ("dia-visible",
                                                         "dia-visible-empty");

  g_signal_connect (G_OBJECT (priv->visible),
                    "button-release-event",
                    G_CALLBACK (button_event),
                    self);
  g_signal_connect (G_OBJECT (priv->visible),
                    "button-press-event",
                    G_CALLBACK (button_event),
                    self);
  g_signal_connect (G_OBJECT (priv->visible),
                    "toggled",
                    G_CALLBACK (visible_toggled),
                    self);
  gtk_box_pack_start (GTK_BOX (hbox), priv->visible, FALSE, TRUE, 2);
  gtk_widget_show (priv->visible);

  priv->connectable =
    dia_toggle_button_new_with_icon_names ("dia-connectable",
                                           "dia-connectable-empty");

  g_signal_connect (G_OBJECT (priv->connectable),
                    "button-release-event",
                    G_CALLBACK (button_event),
                    self);
  g_signal_connect (G_OBJECT (priv->connectable),
                    "button-press-event",
                    G_CALLBACK (button_event),
                    self);
  g_signal_connect (G_OBJECT (priv->connectable),
                    "toggled",
                    G_CALLBACK (connectable_toggled),
                    self);

  gtk_box_pack_start (GTK_BOX (hbox), priv->connectable, FALSE, TRUE, 2);
  gtk_widget_show (priv->connectable);

  priv->label = gtk_label_new ("layer_default_label");
  gtk_label_set_justify (GTK_LABEL (priv->label), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (hbox), priv->label, FALSE, TRUE, 0);
  gtk_widget_show (priv->label);

  gtk_widget_show (hbox);

  gtk_container_add (GTK_CONTAINER (self), hbox);
}


void
dia_layer_widget_set_layer (DiaLayerWidget *self,
                            DiaLayer       *layer)
{
  DiaLayerWidgetPrivate *priv;

  g_return_if_fail (DIA_IS_LAYER_WIDGET (self));

  priv = dia_layer_widget_get_instance_private (self);

  if (g_set_object (&priv->layer, layer)) {
    g_clear_object (&priv->name_binding);

    if (!layer) {
      gtk_widget_set_sensitive (GTK_WIDGET (self), FALSE);

      g_object_notify_by_pspec (G_OBJECT (self), lw_pspecs[LW_PROP_LAYER]);

      return;
    }

    priv->name_binding = g_object_bind_property (layer, "name",
                                                priv->label, "label",
                                                G_BINDING_SYNC_CREATE);

    priv->internal_call = TRUE;
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->visible),
                                  dia_layer_is_visible (priv->layer));

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->connectable),
                                  dia_layer_is_connectable (priv->layer));
    priv->internal_call = FALSE;

    /* These may get toggled when the button is set without the widget being
    * selected first.
    * The connect_on state gets also used to restore with just a deselect
    * of the active layer.
    */
    priv->connect_on = dia_layer_is_connectable (layer);
    priv->connect_off = FALSE;

    gtk_widget_set_sensitive (GTK_WIDGET (self), TRUE);

    g_object_notify_by_pspec (G_OBJECT (self), lw_pspecs[LW_PROP_LAYER]);
  }
}


DiaLayer *
dia_layer_widget_get_layer (DiaLayerWidget *self)
{
  DiaLayerWidgetPrivate *priv;

  g_return_val_if_fail (DIA_IS_LAYER_WIDGET (self), NULL);

  priv = dia_layer_widget_get_instance_private (self);

  return priv->layer;
}


void
dia_layer_widget_set_connectable (DiaLayerWidget *self, gboolean on)
{
  DiaLayerWidgetPrivate *priv;

  g_return_if_fail (DIA_IS_LAYER_WIDGET (self));

  priv = dia_layer_widget_get_instance_private (self);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->connectable), on);

  g_object_notify_by_pspec (G_OBJECT (self), lw_pspecs[LW_PROP_CONNECTABLE]);
}


gboolean
dia_layer_widget_get_connectable (DiaLayerWidget *self)
{
  DiaLayerWidgetPrivate *priv;

  g_return_val_if_fail (DIA_IS_LAYER_WIDGET (self), FALSE);

  priv = dia_layer_widget_get_instance_private (self);

  return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->connectable));
}


GtkWidget *
dia_layer_widget_new (DiaLayer *layer)
{
  return g_object_new (DIA_TYPE_LAYER_WIDGET, "layer", layer, NULL);
}


void
dia_layer_widget_select (DiaLayerWidget *self)
{
  DiaLayerWidgetPrivate *priv;
  DiagramData *diagram;

  g_return_if_fail (DIA_IS_LAYER_WIDGET (self));

  priv = dia_layer_widget_get_instance_private (self);

  g_return_if_fail (priv->layer != NULL);

  diagram = dia_layer_get_parent_diagram (priv->layer);

  /* Don't deselect if we're selected the active layer.  This can happen
   * if the window has been defocused. */
  if (dia_diagram_data_get_active_layer (diagram) != priv->layer) {
    diagram_remove_all_selected (DIA_DIAGRAM (diagram), TRUE);
  }
  diagram_update_extents (DIA_DIAGRAM (diagram));
  data_set_active_layer (diagram, priv->layer);
  diagram_add_update_all (DIA_DIAGRAM (diagram));
  diagram_flush (DIA_DIAGRAM (diagram));

  priv->internal_call = TRUE;
  if (priv->connect_off) { /* If the user wants this off, it becomes so */
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->connectable), FALSE);
  } else {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->connectable), TRUE);
  }
  priv->internal_call = FALSE;
}


void
dia_layer_widget_deselect (DiaLayerWidget *self)
{
  DiaLayerWidgetPrivate *priv;

  g_return_if_fail (DIA_IS_LAYER_WIDGET (self));

  priv = dia_layer_widget_get_instance_private (self);

  priv->internal_call = TRUE;
  /** Set to on if the user has requested so. */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->connectable),
                                priv->connect_on);
  priv->internal_call = FALSE;
}
