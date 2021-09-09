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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "dia-application.h"
#include "dia-layer.h"
#include "dia-layer-properties.h"
#include "layer_dialog.h"

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
                                    dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (priv->diagram))) + 1;

    layer = dia_layer_new (gtk_entry_get_text (GTK_ENTRY (priv->entry)),
                           DIA_DIAGRAM_DATA (priv->diagram));
    data_add_layer_at (DIA_DIAGRAM_DATA (priv->diagram), layer, pos);
    data_set_active_layer (DIA_DIAGRAM_DATA (priv->diagram), layer);

    diagram_add_update_all (priv->diagram);
    diagram_flush (priv->diagram);

    dia_layer_change_new (priv->diagram, layer, TYPE_ADD_LAYER, pos);
    undo_set_transactionpoint (priv->diagram->undo);

    g_clear_object (&layer);
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
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))),
                      vbox,
                      TRUE,
                      TRUE,
                      0);

  /*  the name entry hbox, label and entry  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
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

    g_clear_pointer (&name, g_free);
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
