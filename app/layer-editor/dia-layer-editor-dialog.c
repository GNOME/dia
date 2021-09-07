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

#include "persistence.h"
#include "dia-application.h"
#include "dia-layer-editor.h"
#include "dia-layer-editor-dialog.h"
#include "layer_dialog.h"

enum {
  COL_FILENAME,
  COL_DIAGRAM
};

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

    g_clear_pointer (&basename, g_free);
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

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);

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


static gboolean
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
  DiaLayerEditorDialogPrivate *priv;

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

  g_object_notify_by_pspec (G_OBJECT (self), dlg_pspecs[DLG_PROP_DIAGRAM]);
}


Diagram *
dia_layer_editor_dialog_get_diagram (DiaLayerEditorDialog *self)
{
  DiaLayerEditorDialogPrivate *priv;

  g_return_val_if_fail (DIA_IS_LAYER_EDITOR_DIALOG (self), NULL);

  priv = dia_layer_editor_dialog_get_instance_private (self);

  return priv->diagram;
}
