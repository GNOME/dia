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
#include <gdk/gdkkeysyms.h>

#include "dia-layer.h"
#include "dia-layer-widget.h"
#include "dia-layer-editor.h"
#include "dia-layer-properties.h"
#include "dia-layer-list.h"
#include "layer_dialog.h"

typedef struct _ButtonData ButtonData;

struct _ButtonData {
  gchar *icon_name;
  gpointer callback;
  char *tooltip;
};

typedef struct _DiaLayerEditorPrivate DiaLayerEditorPrivate;
struct _DiaLayerEditorPrivate {
  GtkWidget *list;
  Diagram   *diagram;
};

G_DEFINE_TYPE_WITH_PRIVATE (DiaLayerEditor, dia_layer_editor, GTK_TYPE_BOX)

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


static int
list_button_press (GtkWidget      *widget,
                   GdkEvent       *event,
                   DiaLayerEditor *self)
{
  GtkWidget *event_widget;
  DiaLayerWidget *layer_widget;

  event_widget = gtk_get_event_widget (event);

  if (DIA_IS_LAYER_WIDGET (event_widget)) {
    layer_widget = DIA_LAYER_WIDGET (event_widget);

    if (event->type == GDK_2BUTTON_PRESS) {
      rename_layer (GTK_WIDGET (layer_widget), self);
    }
  }

  return FALSE;
}


static void
new_layer (GtkWidget *widget, DiaLayerEditor *self)
{
  DiaLayerEditorPrivate *priv = dia_layer_editor_get_instance_private (self);
  DiaLayer *layer;
  DiaLayer *active;
  Diagram *dia;
  int pos;
  static int next_layer_num = 1;

  dia = priv->diagram;

  if (dia != NULL) {
    char* new_layer_name = g_strdup_printf (_("New layer %d"),
                                            next_layer_num++);
    layer = dia_layer_new (new_layer_name, DIA_DIAGRAM_DATA (dia));

    active = dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia));
    pos = data_layer_get_index (DIA_DIAGRAM_DATA (dia), active);

    data_add_layer_at (DIA_DIAGRAM_DATA (dia), layer, pos + 1);
    data_set_active_layer (DIA_DIAGRAM_DATA (dia), layer);

    diagram_add_update_all (dia);
    diagram_flush (dia);

    dia_layer_change_new (dia, layer, TYPE_ADD_LAYER, pos + 1);
    undo_set_transactionpoint (dia->undo);

    g_clear_pointer (&new_layer_name, g_free);
    g_clear_object (&layer);
  }
}


static void
delete_layer (GtkWidget *widget, DiaLayerEditor *self)
{
  DiaLayerEditorPrivate *priv = dia_layer_editor_get_instance_private (self);
  Diagram *dia;
  DiaLayer *layer;
  int pos;

  dia = priv->diagram;

  if (dia != NULL) {
    layer = dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia));

    g_return_if_fail (layer);

    pos = data_layer_get_index (DIA_DIAGRAM_DATA (dia), layer);

    dia_layer_change_new (dia, layer, TYPE_DELETE_LAYER, pos);
    undo_set_transactionpoint (dia->undo);

    data_remove_layer (dia->data, layer);

    diagram_add_update_all (dia);
    diagram_flush (dia);
  }
}


static void
rename_layer (GtkWidget *widget, DiaLayerEditor *self)
{
  DiaLayerEditorPrivate *priv = dia_layer_editor_get_instance_private (self);
  DiaLayer *layer;
  GtkWidget *dlg;

  if (priv->diagram == NULL) {
    return;
  }

  layer = dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (priv->diagram));

  dlg = g_object_new (DIA_TYPE_LAYER_PROPERTIES,
                      "layer", layer,
                      "visible", TRUE,
                      NULL);
  gtk_widget_show (dlg);
}


static void
raise_layer (GtkWidget *widget, DiaLayerEditor *self)
{
  DiaLayerEditorPrivate *priv = dia_layer_editor_get_instance_private (self);
  DiaLayer *layer;
  Diagram *dia;

  dia = priv->diagram;

  if (dia != NULL) {
    layer = dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia));

    g_return_if_fail (layer);

    dia_layer_change_new (dia, layer, TYPE_RAISE_LAYER, 0);
    undo_set_transactionpoint (dia->undo);

    data_raise_layer (DIA_DIAGRAM_DATA (dia), layer);

    diagram_add_update_all (dia);
    diagram_flush (dia);
  }
}


static void
lower_layer (GtkWidget *widget, DiaLayerEditor *self)
{
  DiaLayerEditorPrivate *priv = dia_layer_editor_get_instance_private (self);
  DiaLayer *layer;
  Diagram *dia;

  dia = priv->diagram;

  if (dia != NULL) {
    layer = dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia));

    g_return_if_fail (layer);

    dia_layer_change_new (dia, layer, TYPE_LOWER_LAYER, 0);
    undo_set_transactionpoint (dia->undo);

    data_lower_layer (DIA_DIAGRAM_DATA (dia), layer);

    diagram_add_update_all (dia);
    diagram_flush (dia);
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
  gtk_orientable_set_orientation (GTK_ORIENTABLE(self), GTK_ORIENTATION_VERTICAL);

  priv->diagram = NULL;

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolled_win);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
                                       GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (self), scrolled_win, TRUE, TRUE, 2);

  priv->list = dia_layer_list_new ();

  gtk_widget_show (priv->list);
  gtk_container_add (GTK_CONTAINER (scrolled_win), priv->list);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (priv->list),
                                       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));

  g_signal_connect (G_OBJECT (priv->list),
                    "button-press-event",
                    G_CALLBACK (list_button_press),
                    self);

  // inline-toolbar
  button_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

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
  return g_object_new (DIA_TYPE_LAYER_EDITOR, NULL);
}


void
dia_layer_editor_set_diagram (DiaLayerEditor *self,
                              Diagram        *dia)
{
  DiaLayerEditorPrivate *priv;

  g_return_if_fail (DIA_IS_LAYER_EDITOR (self));

  priv = dia_layer_editor_get_instance_private (self);

  if (g_set_object (&priv->diagram, dia)) {
    dia_layer_list_set_diagram (DIA_LAYER_LIST (priv->list),
                                DIA_DIAGRAM_DATA (dia));

    gtk_widget_set_sensitive (GTK_WIDGET (self), dia != NULL);

    g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_DIAGRAM]);
  }
}


Diagram *
dia_layer_editor_get_diagram (DiaLayerEditor *self)
{
  DiaLayerEditorPrivate *priv;

  g_return_val_if_fail (DIA_IS_LAYER_EDITOR (self), NULL);

  priv = dia_layer_editor_get_instance_private (self);

  return priv->diagram;
}
