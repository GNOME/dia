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
 */

#include <gtk/gtk.h>

#include "intl.h"

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
struct _DiaLayerWidgetPrivate
{
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

  DiaLayerEditor *editor;

  /* If TRUE, we're in the middle of a internal call to
   * dia_layer_widget_*_toggled and should not make undo, update diagram etc.
   *
   * If these calls were not done by simulating button presses, we could avoid
   * this hack.
   */
  gboolean internal_call;

  gboolean shifted;
};

G_DEFINE_TYPE_WITH_PRIVATE (DiaLayerWidget, dia_layer_widget, GTK_TYPE_LIST_ITEM)

enum {
  EXCLUSIVE,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

enum {
  LW_PROP_0,
  LW_PROP_LAYER,
  LW_PROP_EDITOR,
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
    case LW_PROP_EDITOR:
      dia_layer_widget_set_editor (self, g_value_get_object (value));
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
    case LW_PROP_EDITOR:
      g_value_set_object (value, dia_layer_widget_get_editor (self));
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
  g_clear_object (&priv->editor);

  G_OBJECT_CLASS (dia_layer_widget_parent_class)->finalize (object);
}


static void
dia_layer_widget_class_init (DiaLayerWidgetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = dia_layer_widget_set_property;
  object_class->get_property = dia_layer_widget_get_property;
  object_class->finalize = dia_layer_widget_finalize;

  signals[EXCLUSIVE] =
    g_signal_new ("exclusive",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL,
                  NULL,
                  NULL,
                  G_TYPE_NONE, 0);

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
   * DiaLayerWidget:editor:
   *
   * The #DiaLayerEditor this is for
   *
   * Since: 0.98
   */
  lw_pspecs[LW_PROP_EDITOR] =
    g_param_spec_object ("editor",
                         "Editor",
                         "The editor",
                         DIA_TYPE_LAYER_EDITOR,
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

  if (priv->layer == dia_layer_get_parent_diagram (priv->layer)->active_layer) {
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
visible_clicked (GtkToggleButton *widget,
                 gpointer         userdata)
{
  DiaLayerWidget *self = DIA_LAYER_WIDGET (userdata);
  DiaLayerWidgetPrivate *priv = dia_layer_widget_get_instance_private (self);
  struct LayerVisibilityChange *change;

  /* Have to use this internal_call hack 'cause there's no way to switch
   * a toggle button without causing the 'clicked' event:(
   */
  if (!priv->internal_call) {
    Diagram *dia = DIA_DIAGRAM (dia_layer_get_parent_diagram (priv->layer));
    change = undo_layer_visibility (dia, priv->layer, priv->shifted);
    /** This apply kills 'lw', thus we have to hold onto 'lw->dia' */
    layer_visibility_change_apply (change, dia);
    undo_set_transactionpoint (dia->undo);
  }
}

static void
select_callback (GtkWidget *widget, gpointer data)
{
  DiaLayerWidget *self = DIA_LAYER_WIDGET (widget);
  DiaLayerWidgetPrivate *priv = dia_layer_widget_get_instance_private (self);
  DiagramData *diagram;

  g_return_if_fail (priv->layer != NULL);

  diagram = dia_layer_get_parent_diagram (priv->layer);

  /* Don't deselect if we're selected the active layer.  This can happen
   * if the window has been defocused. */
  if (diagram->active_layer != priv->layer) {
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

static void
deselect_callback (GtkWidget *widget, gpointer data)
{
  DiaLayerWidget *self = DIA_LAYER_WIDGET (widget);
  DiaLayerWidgetPrivate *priv = dia_layer_widget_get_instance_private (self);

  priv->internal_call = TRUE;
  /** Set to on if the user has requested so. */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->connectable),
                                priv->connect_on);
  priv->internal_call = FALSE;
}

static void
dia_layer_widget_init (DiaLayerWidget *self)
{
  DiaLayerWidgetPrivate *priv = dia_layer_widget_get_instance_private (self);
  GtkWidget *hbox;

  hbox = gtk_hbox_new (FALSE, 0);

  priv->internal_call = FALSE;
  priv->shifted = FALSE;

  priv->layer = NULL;

  priv->connect_on = FALSE;
  priv->connect_off = FALSE;

  priv->visible = dia_toggle_button_new_with_icon_names ("dia-visible",
                                                         "dia-visible-empty");

  priv->editor = NULL;

  g_signal_connect (G_OBJECT (priv->visible),
                    "button-release-event",
                    G_CALLBACK (button_event),
                    self);
  g_signal_connect (G_OBJECT (priv->visible),
                    "button-press-event",
                    G_CALLBACK (button_event),
                    self);
  g_signal_connect (G_OBJECT (priv->visible),
                    "clicked",
                    G_CALLBACK (visible_clicked),
                    self);
  gtk_box_pack_start (GTK_BOX (hbox), priv->visible, FALSE, TRUE, 2);
  gtk_widget_show (priv->visible);

  /*gtk_image_new_from_stock(GTK_STOCK_CONNECT,
			    GTK_ICON_SIZE_BUTTON), */
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
                    "clicked",
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

  g_signal_connect (G_OBJECT (self),
                    "select",
                    G_CALLBACK (select_callback),
                    NULL);
  g_signal_connect (G_OBJECT (self),
                    "deselect",
                    G_CALLBACK (deselect_callback),
                    NULL);
}


void
dia_layer_widget_set_layer (DiaLayerWidget *self,
                            DiaLayer       *layer)
{
  DiaLayerWidgetPrivate *priv;

  g_return_if_fail (DIA_IS_LAYER_WIDGET (self));

  priv = dia_layer_widget_get_instance_private (self);

  g_clear_object (&priv->layer);
  if (layer) {
    priv->layer = g_object_ref (layer);

    g_clear_object (&priv->name_binding);
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
  } else {
    gtk_widget_set_sensitive (GTK_WIDGET (self), FALSE);
  }

  g_object_notify_by_pspec (G_OBJECT (self), lw_pspecs[LW_PROP_LAYER]);
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
dia_layer_widget_set_editor (DiaLayerWidget *self,
                             DiaLayerEditor *editor)
{
  DiaLayerWidgetPrivate *priv;

  g_return_if_fail (DIA_IS_LAYER_WIDGET (self));

  priv = dia_layer_widget_get_instance_private (self);

  g_clear_object (&priv->editor);
  if (editor) {
    priv->editor = g_object_ref (editor);
  }

  g_object_notify_by_pspec (G_OBJECT (self), lw_pspecs[LW_PROP_EDITOR]);
}


DiaLayerEditor *
dia_layer_widget_get_editor (DiaLayerWidget *self)
{
  DiaLayerWidgetPrivate *priv;

  g_return_val_if_fail (DIA_IS_LAYER_WIDGET (self), NULL);

  priv = dia_layer_widget_get_instance_private (self);

  return priv->editor;
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
dia_layer_widget_new (DiaLayer *layer, DiaLayerEditor *editor)
{
  return g_object_new (DIA_TYPE_LAYER_WIDGET,
                       "layer", layer,
                       "editor", editor,
                       NULL);
}
