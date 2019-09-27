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

/* Parts of this file are derived from the file layers_dialog.c in the Gimp:
 *
 * The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 */

#include <config.h>

#include <assert.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#undef GTK_DISABLE_DEPRECATED /* GtkListItem, gtk_list_new, ... */
#include <gtk/gtk.h>

#include "intl.h"

#include "layer_dialog.h"
#include "persistence.h"
#include "widgets.h"
#include "interface.h"
#include "dia-layer.h"

#include "dia-application.h" /* dia_diagram_change */


enum {
  COL_FILENAME,
  COL_DIAGRAM
};

struct LayerDialog {
  GtkWidget *dialog;

  GtkWidget *layer_editor;
};

static struct LayerDialog *layer_dialog = NULL;

typedef struct _ButtonData ButtonData;

struct _ButtonData {
  gchar *icon_name;
  gpointer callback;
  char *tooltip;
};

enum LayerChangeType {
  TYPE_DELETE_LAYER,
  TYPE_ADD_LAYER,
  TYPE_RAISE_LAYER,
  TYPE_LOWER_LAYER,
};

struct LayerChange {
  Change change;

  enum LayerChangeType type;
  DiaLayer *layer;
  int index;
  int applied;
};

struct LayerVisibilityChange {
  Change change;

  GList *original_visibility;
  DiaLayer *layer;
  gboolean is_exclusive;
  int applied;
};

static Change *
undo_layer(Diagram *dia, DiaLayer *layer, enum LayerChangeType, int index);
static struct LayerVisibilityChange *
undo_layer_visibility(Diagram *dia, DiaLayer *layer, gboolean exclusive);
static void
layer_visibility_change_apply(struct LayerVisibilityChange *change,
			      Diagram *dia);


static void
layer_view_hide_button_clicked (void * not_used)
{
  integrated_ui_layer_view_show (FALSE);
}

GtkWidget * create_layer_view_widget (void)
{
  GtkWidget  *vbox;
  GtkWidget  *hbox;
  GtkWidget  *label;
  GtkWidget  *hide_button;
  GtkRcStyle *rcstyle;    /* For hide_button */
  GtkWidget  *image;      /* For hide_button */

  /* if layer_dialog were renamed to layer_view_data this would make
   * more sense.
   */
  layer_dialog = g_new0 (struct LayerDialog, 1);

  layer_dialog->dialog = vbox = gtk_vbox_new (FALSE, 1);

  hbox = gtk_hbox_new (FALSE, 1);

  label = gtk_label_new (_ ("Layers"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);

  /* Hide Button */
  hide_button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (hide_button), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click (GTK_BUTTON (hide_button), FALSE);
  gtk_widget_set_tooltip_text (hide_button, _("Close Layer pane"));

  /* make it as small as possible */
  rcstyle = gtk_rc_style_new ();
  rcstyle->xthickness = rcstyle->ythickness = 0;
  gtk_widget_modify_style (hide_button, rcstyle);
  g_object_unref (rcstyle);

  image = gtk_image_new_from_icon_name ("window-close-symbolic",
                                        GTK_ICON_SIZE_MENU);

  gtk_container_add (GTK_CONTAINER (hide_button), image);
  g_signal_connect (G_OBJECT (hide_button), "clicked",
                    G_CALLBACK (layer_view_hide_button_clicked), NULL);

  gtk_box_pack_end (GTK_BOX (hbox), hide_button, FALSE, FALSE, 2);
  gtk_widget_show_all (hbox);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 2);

  layer_dialog->layer_editor = dia_layer_editor_new ();
  gtk_widget_show (layer_dialog->layer_editor);
  gtk_box_pack_start (GTK_BOX (vbox), layer_dialog->layer_editor, TRUE, TRUE, 0);

  return vbox;
}


void
layer_dialog_create (void)
{
  layer_dialog = g_new0(struct LayerDialog, 1);

  layer_dialog->dialog = dia_layer_editor_dialog_new ();
  gtk_widget_show (layer_dialog->dialog);
}


void
layer_dialog_show()
{
  if (is_integrated_ui () == FALSE)
  {
  if (layer_dialog == NULL || layer_dialog->dialog == NULL)
    layer_dialog_create();
  g_assert(layer_dialog != NULL); /* must be valid now */
  gtk_window_present(GTK_WINDOW(layer_dialog->dialog));
  }
}

/*
 * Used to avoid writing to possibly already deleted layer in
 * dia_layer_widget_connectable_toggled(). Must be called before
 * e.g. gtk_list_clear_items() cause that will emit the toggled
 * signal to last focus widget. See bug #329096
 */
static void
_layer_widget_clear_layer (GtkWidget *widget, gpointer user_data)
{
  DiaLayerWidget *lw = DIA_LAYER_WIDGET (widget);

  dia_layer_widget_set_layer (lw, NULL);
}


void
layer_dialog_set_diagram (Diagram *dia)
{
  if (layer_dialog == NULL || layer_dialog->dialog == NULL) {
    layer_dialog_create (); /* May have been destroyed */
  }

  g_assert (layer_dialog != NULL); /* must be valid now */

  if (DIA_IS_LAYER_EDITOR_DIALOG (layer_dialog->dialog)) {
    dia_layer_editor_dialog_set_diagram (DIA_LAYER_EDITOR_DIALOG (layer_dialog->dialog),
                                         dia);
  } else {
    dia_layer_editor_set_diagram (DIA_LAYER_EDITOR (layer_dialog->layer_editor),
                                  dia);
  }
}



/******* DiaLayerWidget: *****/

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


static void
layer_dialog_edit_layer (Diagram *dia, DiaLayer *layer)
{
  GtkWidget *dlg;

  g_return_if_fail (dia || layer);

  if (layer) {
    dlg = g_object_new (DIA_TYPE_LAYER_PROPERTIES,
                        "layer", layer,
                        "visible", TRUE,
                        NULL);
  } else {
    dlg = g_object_new (DIA_TYPE_LAYER_PROPERTIES,
                        "diagram", dia,
                        "visible", TRUE,
                        NULL);
  }

  gtk_widget_show (dlg);
}


/******** layer changes: */

static void
layer_change_apply(struct LayerChange *change, Diagram *dia)
{
  change->applied = 1;

  switch (change->type) {
  case TYPE_DELETE_LAYER:
    data_remove_layer(dia->data, change->layer);
    break;
  case TYPE_ADD_LAYER:
    data_add_layer_at(dia->data, change->layer, change->index);
    break;
  case TYPE_RAISE_LAYER:
    data_raise_layer(dia->data, change->layer);
    break;
  case TYPE_LOWER_LAYER:
    data_lower_layer(dia->data, change->layer);
    break;
  }

  diagram_add_update_all(dia);
}

static void
layer_change_revert(struct LayerChange *change, Diagram *dia)
{
  switch (change->type) {
  case TYPE_DELETE_LAYER:
    data_add_layer_at(dia->data, change->layer, change->index);
    break;
  case TYPE_ADD_LAYER:
    data_remove_layer(dia->data, change->layer);
    break;
  case TYPE_RAISE_LAYER:
    data_lower_layer(dia->data, change->layer);
    break;
  case TYPE_LOWER_LAYER:
    data_raise_layer(dia->data, change->layer);
    break;
  }

  diagram_add_update_all(dia);

  change->applied = 0;
}

static void
layer_change_free (struct LayerChange *change)
{
  switch (change->type) {
    case TYPE_DELETE_LAYER:
      if (change->applied) {
        g_clear_object (&change->layer);
      }
      break;
    case TYPE_ADD_LAYER:
      if (!change->applied) {
        g_clear_object (&change->layer);
      }
      break;
    case TYPE_RAISE_LAYER:
      break;
    case TYPE_LOWER_LAYER:
      break;
  }
}

static Change *
undo_layer(Diagram *dia, DiaLayer *layer, enum LayerChangeType type, int index)
{
  struct LayerChange *change;

  change = g_new0(struct LayerChange, 1);

  change->change.apply = (UndoApplyFunc) layer_change_apply;
  change->change.revert = (UndoRevertFunc) layer_change_revert;
  change->change.free = (UndoFreeFunc) layer_change_free;

  change->type = type;
  change->layer = layer;
  change->index = index;
  change->applied = 1;

  undo_push_change(dia->undo, (Change *) change);
  return (Change *)change;
}

static void
layer_visibility_change_apply(struct LayerVisibilityChange *change,
			      Diagram *dia)
{
  GPtrArray *layers;
  DiaLayer *layer = change->layer;
  int visible = FALSE;
  int i;

  if (change->is_exclusive) {
    /*  First determine if _any_ other layer widgets are set to visible.
     *  If there is, exclusive switching turns all off.  */
    for (i=0;i<dia->data->layers->len;i++) {
      DiaLayer *temp_layer = g_ptr_array_index(dia->data->layers, i);
      if (temp_layer != layer) {
        visible |= dia_layer_is_visible (temp_layer);
      }
    }

    /*  Now, toggle the visibility for all layers except the specified one  */
    layers = dia->data->layers;
    for (i = 0; i < layers->len; i++) {
      DiaLayer *temp_layer = (DiaLayer *) g_ptr_array_index(layers, i);
      if (temp_layer == layer) {
        dia_layer_set_visible (temp_layer, TRUE);
      } else {
        dia_layer_set_visible (temp_layer, !visible);
      }
    }
  } else {
    dia_layer_set_visible (layer, !dia_layer_is_visible (layer));
  }
  diagram_add_update_all (dia);
}

/** Revert to the visibility before this change was applied.
 */
static void
layer_visibility_change_revert(struct LayerVisibilityChange *change,
			       Diagram *dia)
{
  GList *vis = change->original_visibility;
  GPtrArray *layers = dia->data->layers;
  int i;

  for (i = 0; vis != NULL && i < layers->len; vis = g_list_next(vis), i++) {
    DiaLayer *layer = DIA_LAYER (g_ptr_array_index (layers, i));
    dia_layer_set_visible (layer, GPOINTER_TO_INT (vis->data));
  }

  if (vis != NULL || i < layers->len) {
    printf("Internal error: visibility undo has %d visibilities, but %d layers\n",
	   g_list_length(change->original_visibility), layers->len);
  }

  diagram_add_update_all(dia);
}

static void
layer_visibility_change_free(struct LayerVisibilityChange *change)
{
  g_list_free(change->original_visibility);
}

static struct LayerVisibilityChange *
undo_layer_visibility(Diagram *dia, DiaLayer *layer, gboolean exclusive)
{
  struct LayerVisibilityChange *change;
  GList *visibilities = NULL;
  int i;
  GPtrArray *layers = dia->data->layers;

  change = g_new0(struct LayerVisibilityChange, 1);

  change->change.apply = (UndoApplyFunc) layer_visibility_change_apply;
  change->change.revert = (UndoRevertFunc) layer_visibility_change_revert;
  change->change.free = (UndoFreeFunc) layer_visibility_change_free;

  for (i = 0; i < layers->len; i++) {
    DiaLayer *temp_layer = DIA_LAYER (g_ptr_array_index (layers, i));
    visibilities = g_list_append (visibilities, GINT_TO_POINTER (dia_layer_is_visible (temp_layer)));
  }

  change->original_visibility = visibilities;
  change->layer = layer;
  change->is_exclusive = exclusive;

  undo_push_change(dia->undo, (Change *) change);
  return change;
}

/*!
 * \brief edit a layers name, possibly also creating the layer
 */
void
diagram_edit_layer(Diagram *dia, DiaLayer *layer)
{
  g_return_if_fail(dia != NULL);

  layer_dialog_edit_layer (layer ? NULL : dia, layer);
}

typedef struct _DiaLayerEditorPrivate DiaLayerEditorPrivate;
struct _DiaLayerEditorPrivate {
  GtkWidget *list;
  Diagram   *diagram;
};

G_DEFINE_TYPE_WITH_PRIVATE (DiaLayerEditor, dia_layer_editor, GTK_TYPE_VBOX)

enum {
  PROP_0,
  PROP_DIAGRAM,
  LAST_PROP
};

static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
dia_layer_editor_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  DiaLayerEditor *self = DIA_LAYER_EDITOR (object);

  switch (property_id) {
    case PROP_DIAGRAM:
      dia_layer_editor_set_diagram (self, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_layer_editor_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  DiaLayerEditor *self = DIA_LAYER_EDITOR (object);

  switch (property_id) {
    case PROP_DIAGRAM:
      g_value_set_object (value, dia_layer_editor_get_diagram (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_layer_editor_finalize (GObject *object)
{
  DiaLayerEditor *self = DIA_LAYER_EDITOR (object);
  DiaLayerEditorPrivate *priv = dia_layer_editor_get_instance_private (self);

  g_clear_object (&priv->diagram);

  G_OBJECT_CLASS (dia_layer_editor_parent_class)->finalize (object);
}


static void
dia_layer_editor_class_init (DiaLayerEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = dia_layer_editor_set_property;
  object_class->get_property = dia_layer_editor_get_property;
  object_class->finalize = dia_layer_editor_finalize;

  /**
   * DiaLayerEditor:diagram:
   *
   * Since: 0.98
   */
  pspecs[PROP_DIAGRAM] =
    g_param_spec_object ("diagram",
                         "Diagram",
                         "The current diagram",
                         DIA_TYPE_DIAGRAM,
                         G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}


static void rename_layer (GtkWidget *widget, DiaLayerEditor *self);

static gint
list_event (GtkWidget      *widget,
            GdkEvent       *event,
            DiaLayerEditor *self)
{
  GdkEventKey *kevent;
  GtkWidget *event_widget;
  DiaLayerWidget *layer_widget;

  event_widget = gtk_get_event_widget (event);

  if (GTK_IS_LIST_ITEM (event_widget)) {
    layer_widget = DIA_LAYER_WIDGET (event_widget);

    switch (event->type) {
      case GDK_2BUTTON_PRESS:
        rename_layer (GTK_WIDGET (layer_widget), self);
        return TRUE;

      case GDK_KEY_PRESS:
        kevent = (GdkEventKey *) event;
        switch (kevent->keyval) {
          case GDK_Up:
            /* printf ("up arrow\n"); */
            break;
          case GDK_Down:
            /* printf ("down arrow\n"); */
            break;
          default:
            return FALSE;
        }
        return TRUE;

      default:
        break;
    }
  }

  return FALSE;
}


static void
exclusive_connectable (DiaLayerWidget *layer_row,
                       DiaLayerEditor *self)
{
  DiaLayerEditorPrivate *priv = dia_layer_editor_get_instance_private (self);
  GList *list;
  DiaLayerWidget *lw;
  DiaLayer *layer;
  int connectable = FALSE;
  int i;

  /*  First determine if _any_ other layer widgets are set to connectable  */
  for (i = 0; i < data_layer_count (DIA_DIAGRAM_DATA (priv->diagram)); i++) {
    layer = data_layer_get_nth (DIA_DIAGRAM_DATA (priv->diagram), i);

    if (dia_layer_widget_get_layer (DIA_LAYER_WIDGET (layer_row)) != layer) {
      connectable |= dia_layer_is_connectable (layer);
    }
  }

  /*  Now, toggle the connectability for all layers except the specified one  */
  list = GTK_LIST (priv->list)->children;
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
new_layer (GtkWidget *widget, DiaLayerEditor *self)
{
  DiaLayerEditorPrivate *priv = dia_layer_editor_get_instance_private (self);
  DiaLayer *layer;
  Diagram *dia;
  GtkWidget *selected;
  GList *list = NULL;
  GtkWidget *layer_widget;
  int pos;
  static int next_layer_num = 1;

  dia = priv->diagram;

  if (dia != NULL) {
    gchar* new_layer_name = g_strdup_printf (_("New layer %d"),
                                             next_layer_num++);
    layer = dia_layer_new (new_layer_name, dia->data);

    assert (GTK_LIST (priv->list)->selection != NULL);
    selected = GTK_LIST (priv->list)->selection->data;
    pos = gtk_list_child_position (GTK_LIST (priv->list), selected);

    data_add_layer_at (dia->data, layer, dia->data->layers->len - pos);

    diagram_add_update_all (dia);
    diagram_flush (dia);

    layer_widget = dia_layer_widget_new (layer, self);
    g_signal_connect (layer_widget,
                      "exclusive",
                      G_CALLBACK (exclusive_connectable),
                      self);
    gtk_widget_show (layer_widget);

    list = g_list_prepend (list, layer_widget);

    gtk_list_insert_items (GTK_LIST (priv->list), list, pos);

    gtk_list_select_item (GTK_LIST (priv->list), pos);

    undo_layer (dia, layer, TYPE_ADD_LAYER, dia->data->layers->len - pos);
    undo_set_transactionpoint (dia->undo);

    g_free (new_layer_name);
  }
}


static void
rename_layer (GtkWidget *widget, DiaLayerEditor *self)
{
  DiaLayerEditorPrivate *priv = dia_layer_editor_get_instance_private (self);
  Diagram *dia;
  DiaLayer *layer;

  dia = priv->diagram;

  if (dia == NULL) {
    return;
  }

  layer = dia->data->active_layer;
  layer_dialog_edit_layer (NULL, layer);
}


static void
delete_layer (GtkWidget *widget, DiaLayerEditor *self)
{
  DiaLayerEditorPrivate *priv = dia_layer_editor_get_instance_private (self);
  Diagram *dia;
  GtkWidget *selected;
  DiaLayer *layer;
  int pos;

  dia = priv->diagram;

  if ((dia != NULL) && (dia->data->layers->len > 1)) {
    assert (GTK_LIST (priv->list)->selection != NULL);
    selected = GTK_LIST (priv->list)->selection->data;

    layer = dia->data->active_layer;

    data_remove_layer (dia->data, layer);
    diagram_add_update_all (dia);
    diagram_flush (dia);

    pos = gtk_list_child_position (GTK_LIST (priv->list), selected);
    gtk_container_remove (GTK_CONTAINER (priv->list), selected);

    undo_layer (dia, layer, TYPE_DELETE_LAYER,
                dia->data->layers->len - pos);
    undo_set_transactionpoint (dia->undo);

    if (--pos < 0) {
      pos = 0;
    }

    gtk_list_select_item (GTK_LIST (priv->list), pos);
  }
}


static void
raise_layer (GtkWidget *widget, DiaLayerEditor *self)
{
  DiaLayerEditorPrivate *priv = dia_layer_editor_get_instance_private (self);
  DiaLayer *layer;
  Diagram *dia;
  GtkWidget *selected;
  GList *list = NULL;
  int pos;

  dia = priv->diagram;

  if ((dia != NULL) && (dia->data->layers->len > 1)) {
    assert (GTK_LIST (priv->list)->selection != NULL);
    selected = GTK_LIST (priv->list)->selection->data;

    pos = gtk_list_child_position (GTK_LIST (priv->list), selected);

    if (pos > 0) {
      layer = dia_layer_widget_get_layer (DIA_LAYER_WIDGET (selected));
      data_raise_layer (dia->data, layer);

      list = g_list_prepend (list, selected);

      g_object_ref (selected);

      gtk_list_remove_items (GTK_LIST (priv->list),
                             list);

      gtk_list_insert_items (GTK_LIST (priv->list),
                             list, pos - 1);

      g_object_unref (selected);

      gtk_list_select_item (GTK_LIST (priv->list), pos-1);

      diagram_add_update_all (dia);
      diagram_flush (dia);

      undo_layer (dia, layer, TYPE_RAISE_LAYER, 0);
      undo_set_transactionpoint (dia->undo);
    }
  }
}


static void
lower_layer (GtkWidget *widget, DiaLayerEditor *self)
{
  DiaLayerEditorPrivate *priv = dia_layer_editor_get_instance_private (self);
  DiaLayer *layer;
  Diagram *dia;
  GtkWidget *selected;
  GList *list = NULL;
  int pos;

  dia = priv->diagram;

  if ((dia != NULL) && (dia->data->layers->len > 1)) {
    assert (GTK_LIST (priv->list)->selection != NULL);
    selected = GTK_LIST (priv->list)->selection->data;

    pos = gtk_list_child_position (GTK_LIST (priv->list), selected);

    if (pos < dia->data->layers->len-1) {
      layer = dia_layer_widget_get_layer (DIA_LAYER_WIDGET (selected));
      data_lower_layer (dia->data, layer);

      list = g_list_prepend (list, selected);

      g_object_ref (selected);

      gtk_list_remove_items (GTK_LIST (priv->list),
                             list);

      gtk_list_insert_items (GTK_LIST (priv->list),
                             list, pos + 1);

      g_object_unref (selected);

      gtk_list_select_item (GTK_LIST (priv->list), pos+1);

      diagram_add_update_all (dia);
      diagram_flush (dia);

      undo_layer (dia, layer, TYPE_LOWER_LAYER, 0);
      undo_set_transactionpoint (dia->undo);
    }
  }
}


static ButtonData editor_buttons[] = {
  { "list-add", new_layer, N_("New Layer") },
  { "list-remove", delete_layer, N_("Delete Layer") },
  { "document-properties", rename_layer, N_("Rename Layer") },
  { "go-up", raise_layer, N_("Raise Layer") },
  { "go-down", lower_layer, N_("Lower Layer") },
};


static void
dia_layer_editor_init (DiaLayerEditor *self)
{
  GtkWidget *scrolled_win;
  GtkWidget *button_box;
  GtkWidget *button;
  DiaLayerEditorPrivate *priv = dia_layer_editor_get_instance_private (self);

  gtk_container_set_border_width (GTK_CONTAINER (self), 6);
  g_object_set (self, "width-request", 250, NULL);

  priv->diagram = NULL;

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolled_win);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
                                       GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (self), scrolled_win, TRUE, TRUE, 2);

  priv->list = gtk_list_new ();
  gtk_widget_show (priv->list);
  gtk_list_set_selection_mode (GTK_LIST (priv->list), GTK_SELECTION_BROWSE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win), priv->list);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (priv->list),
                                       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));

  g_signal_connect (G_OBJECT (priv->list),
                    "event",
                    G_CALLBACK (list_event),
                    self);

  // inline-toolbar
  button_box = gtk_hbox_new (FALSE, 0);

  for (int i = 0; i < G_N_ELEMENTS (editor_buttons); i++) {
    GtkWidget * image;

    button = gtk_button_new ();

    image = gtk_image_new_from_icon_name (editor_buttons[i].icon_name,
                                          GTK_ICON_SIZE_BUTTON);

    gtk_button_set_image (GTK_BUTTON (button), image);

    g_signal_connect (G_OBJECT (button),
                      "clicked",
                      G_CALLBACK (editor_buttons[i].callback),
                      self);

    gtk_widget_set_tooltip_text (button, gettext (editor_buttons[i].tooltip));

    gtk_box_pack_start (GTK_BOX (button_box), button, TRUE, TRUE, 0);

    gtk_widget_show (button);
  }

  gtk_box_pack_start (GTK_BOX (self), button_box, FALSE, FALSE, 2);
  gtk_widget_show (button_box);
}


GtkWidget *
dia_layer_editor_new (void)
{
  return g_object_new (DIA_TYPE_LAYER_EDITOR,
                       NULL);
}


void
dia_layer_editor_set_diagram (DiaLayerEditor *self,
                              Diagram        *dia)
{
  DiaLayerEditorPrivate *priv;
  GtkWidget *layer_widget;
  DiaLayer *layer;
  DiaLayer *active_layer = NULL;
  int sel_pos;
  int i,j;

  g_return_if_fail (DIA_IS_LAYER_EDITOR (self));

  priv = dia_layer_editor_get_instance_private (self);

  g_clear_object (&priv->diagram);

  if (dia == NULL) {
    g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_DIAGRAM]);

    return;
  }

  priv->diagram = g_object_ref (dia);

  active_layer = DIA_DIAGRAM_DATA (dia)->active_layer;

  gtk_container_foreach (GTK_CONTAINER (priv->list),
                         _layer_widget_clear_layer, NULL);
  gtk_list_clear_items (GTK_LIST (priv->list), 0, -1);

  sel_pos = 0;
  for (i = data_layer_count (DIA_DIAGRAM_DATA (dia)) - 1, j = 0; i >= 0; i--, j++) {
    layer = data_layer_get_nth (DIA_DIAGRAM_DATA (dia), i);

    layer_widget = dia_layer_widget_new (layer, self);
    gtk_widget_show (layer_widget);

    gtk_container_add (GTK_CONTAINER (priv->list),
                       layer_widget);

    if (layer == active_layer) {
      sel_pos = j;
    }
  }

  gtk_list_select_item (GTK_LIST (priv->list), sel_pos);

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_DIAGRAM]);
}


Diagram *
dia_layer_editor_get_diagram (DiaLayerEditor *self)
{
  DiaLayerEditorPrivate *priv;

  g_return_val_if_fail (DIA_IS_LAYER_EDITOR (self), NULL);

  priv = dia_layer_editor_get_instance_private (self);

  return priv->diagram;
}





typedef struct _DiaLayerEditorDialogPrivate DiaLayerEditorDialogPrivate;
struct _DiaLayerEditorDialogPrivate {
  Diagram      *diagram;

  GtkListStore *store;

  GtkWidget    *combo;
  GtkWidget    *editor;
};

G_DEFINE_TYPE_WITH_PRIVATE (DiaLayerEditorDialog, dia_layer_editor_dialog, GTK_TYPE_DIALOG)

enum {
  DLG_PROP_0,
  DLG_PROP_DIAGRAM,
  LAST_DLG_PROP
};

static GParamSpec *dlg_pspecs[LAST_DLG_PROP] = { NULL, };


static void
dia_layer_editor_dialog_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  DiaLayerEditorDialog *self = DIA_LAYER_EDITOR_DIALOG (object);

  switch (property_id) {
    case DLG_PROP_DIAGRAM:
      dia_layer_editor_dialog_set_diagram (self, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_layer_editor_dialog_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  DiaLayerEditorDialog *self = DIA_LAYER_EDITOR_DIALOG (object);


  switch (property_id) {
    case DLG_PROP_DIAGRAM:
      g_value_set_object (value, dia_layer_editor_dialog_get_diagram (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_layer_editor_dialog_finalize (GObject *object)
{
  DiaLayerEditorDialog *self = DIA_LAYER_EDITOR_DIALOG (object);
  DiaLayerEditorDialogPrivate *priv = dia_layer_editor_dialog_get_instance_private (self);

  g_clear_object (&priv->diagram);

  G_OBJECT_CLASS (dia_layer_editor_dialog_parent_class)->finalize (object);
}


static gboolean
dia_layer_editor_dialog_delete_event (GtkWidget *widget, GdkEventAny *event)
{
  gtk_widget_hide (widget);

  /* We're caching, so don't destroy */
  return TRUE;
}


static void
dia_layer_editor_dialog_class_init (DiaLayerEditorDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = dia_layer_editor_dialog_set_property;
  object_class->get_property = dia_layer_editor_dialog_get_property;
  object_class->finalize = dia_layer_editor_dialog_finalize;

  widget_class->delete_event = dia_layer_editor_dialog_delete_event;

  /**
   * DiaLayerEditorDialog:diagram:
   *
   * Since: 0.98
   */
  dlg_pspecs[DLG_PROP_DIAGRAM] =
    g_param_spec_object ("diagram",
                         "Diagram",
                         "The current diagram",
                         DIA_TYPE_DIAGRAM,
                         G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_DLG_PROP, dlg_pspecs);
}


static void
diagrams_changed (GListModel *diagrams,
                  guint       position,
                  guint       removed,
                  guint       added,
                  gpointer    self)
{
  DiaLayerEditorDialogPrivate *priv = dia_layer_editor_dialog_get_instance_private (self);
  Diagram *dia;
  int i;
  int current_nr;
  GtkTreeIter iter;
  char *basename = NULL;

  g_return_if_fail (DIA_IS_LAYER_EDITOR_DIALOG (self));

  current_nr = -1;

  gtk_list_store_clear (priv->store);

  if (g_list_model_get_n_items (diagrams) < 1) {
    gtk_widget_set_sensitive (self, FALSE);

    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (priv->combo),
                                   NULL);

    return;
  }

  gtk_widget_set_sensitive (self, TRUE);

  i = 0;
  while ((dia = DIA_DIAGRAM (g_list_model_get_item (diagrams, i)))) {
    basename = g_path_get_basename (dia->filename);

    gtk_list_store_append (priv->store, &iter);
    gtk_list_store_set (priv->store,
                        &iter,
                        COL_FILENAME,
                        basename,
                        COL_DIAGRAM,
                        dia,
                        -1);

    if (dia == priv->diagram) {
      current_nr = i;
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (priv->combo),
                                     &iter);
    }

    i++;

    g_free (basename);
    g_clear_object (&dia);
  }

  if (current_nr == -1) {
    dia = NULL;
    if (g_list_model_get_n_items (diagrams) > 0) {
      dia = g_list_model_get_item (diagrams, i);
    }

    layer_dialog_set_diagram (dia);

    g_clear_object (&dia);
  }
}


static void
diagram_changed (GtkComboBox *widget,
                 gpointer     self)
{
  DiaLayerEditorDialogPrivate *priv = dia_layer_editor_dialog_get_instance_private (self);
  GtkTreeIter iter;
  Diagram *diagram = NULL;

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (priv->combo),
                                     &iter)) {
    gtk_tree_model_get (GTK_TREE_MODEL (priv->store),
                        &iter,
                        COL_DIAGRAM,
                        &diagram,
                        -1);
  }

  layer_dialog_set_diagram (diagram);

  g_clear_object (&diagram);
}


static void
dia_layer_editor_dialog_init (DiaLayerEditorDialog *self)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkCellRenderer *renderer;
  DiaLayerEditorDialogPrivate *priv = dia_layer_editor_dialog_get_instance_private (self);

  priv->diagram = NULL;

  gtk_window_set_title (GTK_WINDOW (self), _("Layers"));
  gtk_window_set_role (GTK_WINDOW (self), "layer_window");
  gtk_window_set_resizable (GTK_WINDOW (self), TRUE);

  g_signal_connect (G_OBJECT (self), "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    self);

  vbox = gtk_dialog_get_content_area (GTK_DIALOG (self));

  hbox = gtk_hbox_new (FALSE, 1);

  priv->store = gtk_list_store_new (2,
                                    G_TYPE_STRING,
                                    DIA_TYPE_DIAGRAM);
  priv->combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (priv->store));

  g_signal_connect (dia_application_get_diagrams (dia_application_get_default ()),
                    "items-changed",
                    G_CALLBACK (diagrams_changed),
                    self);

  g_signal_connect (priv->combo,
                    "changed",
                    G_CALLBACK (diagram_changed),
                    self);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->combo),
                              renderer,
                              TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (priv->combo),
                                  renderer,
                                  "text", COL_FILENAME,
                                  NULL);


  gtk_box_pack_start (GTK_BOX (hbox), priv->combo, TRUE, TRUE, 2);
  gtk_widget_show (priv->combo);

  gtk_box_pack_start(GTK_BOX (vbox), hbox, FALSE, FALSE, 2);
  gtk_widget_show (hbox);

  priv->editor = dia_layer_editor_new ();
  g_object_bind_property (self, "diagram",
                          priv->editor, "diagram",
                          G_BINDING_DEFAULT);
  gtk_box_pack_start (GTK_BOX (vbox), priv->editor, TRUE, TRUE, 2);
  gtk_widget_show (priv->editor);

  persistence_register_window (GTK_WINDOW (self));
}


GtkWidget *
dia_layer_editor_dialog_new (void)
{
  return g_object_new (DIA_TYPE_LAYER_EDITOR_DIALOG, NULL);
}


gboolean
find_diagram (GtkTreeModel *model,
              GtkTreePath  *path,
              GtkTreeIter  *iter,
              gpointer      self)
{
  DiaLayerEditorDialogPrivate *priv = dia_layer_editor_dialog_get_instance_private (self);
  Diagram *diagram = NULL;

  gtk_tree_model_get (model, iter, COL_DIAGRAM, &diagram, -1);

  if (diagram == priv->diagram) {
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (priv->combo),
                                   iter);

    g_clear_object (&diagram);

    return TRUE;
  }

  g_clear_object (&diagram);

  return FALSE;
}


void
dia_layer_editor_dialog_set_diagram (DiaLayerEditorDialog *self,
                                     Diagram              *dia)
{
  DiaLayerEditorDialogPrivate *priv = dia_layer_editor_dialog_get_instance_private (self);

  g_return_if_fail (DIA_IS_LAYER_EDITOR_DIALOG (self));

  priv = dia_layer_editor_dialog_get_instance_private (self);

  g_clear_object (&priv->diagram);
  if (dia) {
    priv->diagram = g_object_ref (dia);

    g_signal_handlers_block_by_func (priv->combo, diagram_changed, self);
    gtk_tree_model_foreach (GTK_TREE_MODEL (priv->store),
                            find_diagram,
                            self);
    g_signal_handlers_unblock_by_func (priv->combo, diagram_changed, self);
  } else {
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (priv->combo),
                                   NULL);
  }

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[DLG_PROP_DIAGRAM]);
}


Diagram *
dia_layer_editor_dialog_get_diagram (DiaLayerEditorDialog *self)
{
  DiaLayerEditorDialogPrivate *priv;

  g_return_val_if_fail (DIA_IS_LAYER_EDITOR_DIALOG (self), NULL);

  priv = dia_layer_editor_dialog_get_instance_private (self);

  return priv->diagram;
}







typedef struct _DiaLayerPropertiesPrivate DiaLayerPropertiesPrivate;
struct _DiaLayerPropertiesPrivate {
  GtkWidget *entry;

  DiaLayer  *layer;
  Diagram   *diagram;
};

G_DEFINE_TYPE_WITH_PRIVATE (DiaLayerProperties, dia_layer_properties, GTK_TYPE_DIALOG)

enum {
  LP_PROP_0,
  LP_PROP_LAYER,
  LP_PROP_DIAGRAM,
  LAST_LP_PROP
};

static GParamSpec *lp_pspecs[LAST_LP_PROP] = { NULL, };


static void
dia_layer_properties_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  DiaLayerProperties *self = DIA_LAYER_PROPERTIES (object);

  switch (property_id) {
    case LP_PROP_LAYER:
      dia_layer_properties_set_layer (self, g_value_get_object (value));
      break;
    case LP_PROP_DIAGRAM:
      dia_layer_properties_set_diagram (self, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_layer_properties_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  DiaLayerProperties *self = DIA_LAYER_PROPERTIES (object);

  switch (property_id) {
    case LP_PROP_LAYER:
      g_value_set_object (value, dia_layer_properties_get_layer (self));
      break;
    case LP_PROP_DIAGRAM:
      g_value_set_object (value, dia_layer_properties_get_diagram (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_layer_properties_finalize (GObject *object)
{
  DiaLayerProperties *self = DIA_LAYER_PROPERTIES (object);
  DiaLayerPropertiesPrivate *priv = dia_layer_properties_get_instance_private (self);

  g_clear_object (&priv->layer);
  g_clear_object (&priv->diagram);

  G_OBJECT_CLASS (dia_layer_properties_parent_class)->finalize (object);
}


static void
dia_layer_properties_response (GtkDialog *dialog,
                               int        response)
{
  DiaLayerProperties *self = DIA_LAYER_PROPERTIES (dialog);
  DiaLayerPropertiesPrivate *priv = dia_layer_properties_get_instance_private (self);

  if (response != GTK_RESPONSE_OK) {
    gtk_widget_destroy (GTK_WIDGET (dialog));

    return;
  }

  if (priv->layer) {
    Diagram *dia = DIA_DIAGRAM (dia_layer_get_parent_diagram (priv->layer));

    g_object_set (priv->layer,
                  "name", gtk_entry_get_text (GTK_ENTRY (priv->entry)),
                  NULL);

    diagram_add_update_all (dia);
    diagram_flush (dia);
    /* FIXME: undo handling */

    /* reflect name change on listeners */
    /* TODO, is this defunt with notfy::name? */
    dia_application_diagram_change (dia_application_get_default (),
                                    dia,
                                    DIAGRAM_CHANGE_LAYER,
                                    priv->layer);
  } else if (priv->diagram) {
    DiaLayer *layer;
    int pos = data_layer_get_index (DIA_DIAGRAM_DATA (priv->diagram),
                                    DIA_DIAGRAM_DATA (priv->diagram)->active_layer) + 1;

    layer = dia_layer_new (gtk_entry_get_text (GTK_ENTRY (priv->entry)),
                           DIA_DIAGRAM_DATA (priv->diagram));
    data_add_layer_at (DIA_DIAGRAM_DATA (priv->diagram), layer, pos);
    data_set_active_layer (DIA_DIAGRAM_DATA (priv->diagram), layer);

    diagram_add_update_all (priv->diagram);
    diagram_flush (priv->diagram);

    undo_layer (priv->diagram, layer, TYPE_ADD_LAYER, pos);
    undo_set_transactionpoint (priv->diagram->undo);
  } else {
    g_critical ("Huh, no layer or diagram");
  }

  gtk_widget_destroy (GTK_WIDGET (dialog));
}


static void
dia_layer_properties_class_init (DiaLayerPropertiesClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

  object_class->set_property = dia_layer_properties_set_property;
  object_class->get_property = dia_layer_properties_get_property;
  object_class->finalize = dia_layer_properties_finalize;

  dialog_class->response = dia_layer_properties_response;

  /**
   * DiaLayerProperties:layer:
   *
   * #DiaLayer to rename
   *
   * Overrides #DiaLayerProperties:diagram
   *
   * Since: 0.98
   */
  lp_pspecs[LP_PROP_LAYER] =
    g_param_spec_object ("layer",
                         "Layer",
                         "The layer",
                         DIA_TYPE_LAYER,
                         G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * DiaLayerProperties:diagram:
   *
   * #Diagram to add a #DiaLayer to
   *
   * Overrides #DiaLayerProperties:layer
   *
   * Since: 0.98
   */
  lp_pspecs[LP_PROP_DIAGRAM] =
    g_param_spec_object ("diagram",
                         "Diagram",
                         "The diagram",
                         DIA_TYPE_DIAGRAM,
                         G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_LP_PROP, lp_pspecs);
}


static void
dia_layer_properties_init (DiaLayerProperties *self)
{
  DiaLayerPropertiesPrivate *priv = dia_layer_properties_get_instance_private (self);
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;

  gtk_window_set_role (GTK_WINDOW (self), "edit_layer_attrributes");
  gtk_window_set_title (GTK_WINDOW (self), _("Edit Layer"));
  gtk_window_set_position (GTK_WINDOW (self), GTK_WIN_POS_MOUSE);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))),
                      vbox,
                      TRUE,
                      TRUE,
                      0);

  /*  the name entry hbox, label and entry  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new (_("Layer name:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  priv->entry = gtk_entry_new ();
  gtk_entry_set_activates_default (GTK_ENTRY (priv->entry), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), priv->entry, TRUE, TRUE, 0);

  gtk_widget_show (priv->entry);
  gtk_widget_show (hbox);

  gtk_dialog_add_buttons (GTK_DIALOG (self),
                         _("_OK"), GTK_RESPONSE_OK,
                         _("_Cancel"), GTK_RESPONSE_CANCEL,
                         NULL);

  gtk_widget_show (vbox);
}


void
dia_layer_properties_set_layer (DiaLayerProperties *self,
                                DiaLayer           *layer)
{
  DiaLayerPropertiesPrivate *priv;

  g_return_if_fail (DIA_IS_LAYER_PROPERTIES (self));

  priv = dia_layer_properties_get_instance_private (self);

  g_clear_object (&priv->diagram);
  g_object_notify_by_pspec (G_OBJECT (self), lp_pspecs[LP_PROP_DIAGRAM]);

  gtk_window_set_title (GTK_WINDOW (self), _("Edit Layer"));

  g_clear_object (&priv->layer);
  if (layer) {
    priv->layer = g_object_ref (layer);

    gtk_entry_set_text (GTK_ENTRY (priv->entry),
                        dia_layer_get_name (priv->layer));
  }

  g_object_notify_by_pspec (G_OBJECT (self), lp_pspecs[LP_PROP_LAYER]);
}


DiaLayer *
dia_layer_properties_get_layer (DiaLayerProperties *self)
{
  DiaLayerPropertiesPrivate *priv;

  g_return_val_if_fail (DIA_IS_LAYER_PROPERTIES (self), NULL);

  priv = dia_layer_properties_get_instance_private (self);

  return priv->layer;
}


void
dia_layer_properties_set_diagram (DiaLayerProperties *self,
                                  Diagram            *dia)
{
  DiaLayerPropertiesPrivate *priv;

  g_return_if_fail (DIA_IS_LAYER_PROPERTIES (self));

  priv = dia_layer_properties_get_instance_private (self);

  g_clear_object (&priv->layer);
  g_object_notify_by_pspec (G_OBJECT (self), lp_pspecs[LP_PROP_LAYER]);

  gtk_window_set_title (GTK_WINDOW (self), _("Add Layer"));

  g_clear_object (&priv->diagram);
  if (dia) {
    char *name;

    priv->diagram = g_object_ref (dia);

    name = g_strdup_printf (_("New layer %d"),
                            data_layer_count (DIA_DIAGRAM_DATA (dia)));
    gtk_entry_set_text (GTK_ENTRY (priv->entry), name);

    g_free (name);
  }

  g_object_notify_by_pspec (G_OBJECT (self), lp_pspecs[LP_PROP_DIAGRAM]);
}


Diagram *
dia_layer_properties_get_diagram (DiaLayerProperties *self)
{
  DiaLayerPropertiesPrivate *priv;

  g_return_val_if_fail (DIA_IS_LAYER_PROPERTIES (self), NULL);

  priv = dia_layer_properties_get_instance_private (self);

  return priv->diagram;
}
