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

#include <config.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "intl.h"

#include "dia-layer-widget.h"
#include "dia-layer-editor.h"
#include "dia-layer-properties.h"
#include "dia-layer.h"
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

    g_assert (GTK_LIST (priv->list)->selection != NULL);
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

    dia_layer_change_new (dia, layer, TYPE_ADD_LAYER, dia->data->layers->len - pos);
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
  GtkWidget *dlg;

  dia = priv->diagram;

  if (dia == NULL) {
    return;
  }

  layer = dia->data->active_layer;

  dlg = g_object_new (DIA_TYPE_LAYER_PROPERTIES,
                     "layer", layer,
                     "visible", TRUE,
                     NULL);
  gtk_widget_show (dlg);
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
    g_assert (GTK_LIST (priv->list)->selection != NULL);
    selected = GTK_LIST (priv->list)->selection->data;

    layer = dia->data->active_layer;

    data_remove_layer (dia->data, layer);
    diagram_add_update_all (dia);
    diagram_flush (dia);

    pos = gtk_list_child_position (GTK_LIST (priv->list), selected);
    gtk_container_remove (GTK_CONTAINER (priv->list), selected);

    dia_layer_change_new (dia, layer, TYPE_DELETE_LAYER,
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
    g_assert (GTK_LIST (priv->list)->selection != NULL);
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

      dia_layer_change_new (dia, layer, TYPE_RAISE_LAYER, 0);
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
    g_assert (GTK_LIST (priv->list)->selection != NULL);
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

      dia_layer_change_new (dia, layer, TYPE_LOWER_LAYER, 0);
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
