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

#include "dia-layer.h"
#include "dia-layer-editor.h"
#include "dia-layer-properties.h"
#include "dia-layer-widget.h"
#include "layer_dialog.h"
#include "interface.h"

struct LayerDialog {
  GtkWidget *dialog;

  GtkWidget *layer_editor;
};

static struct LayerDialog *layer_dialog = NULL;


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

Change *
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

void
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

struct LayerVisibilityChange *
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
diagram_edit_layer (Diagram *dia, DiaLayer *layer)
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
