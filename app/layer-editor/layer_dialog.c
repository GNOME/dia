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
  GtkWidget  *image;      /* For hide_button */

  /* if layer_dialog were renamed to layer_view_data this would make
   * more sense.
   */
  layer_dialog = g_new0 (struct LayerDialog, 1);

  layer_dialog->dialog = vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);

  label = gtk_label_new (_ ("Layers"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);

  /* Hide Button */
  hide_button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (hide_button), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click (GTK_BUTTON (hide_button), FALSE);
  gtk_widget_set_tooltip_text (hide_button, _("Close Layer pane"));

  /* make it as small as possible */
  // Gtk3 disabled -- Hub
  // rcstyle = gtk_rc_style_new ();
  // rcstyle->xthickness = rcstyle->ythickness = 0;
  // gtk_widget_modify_style (hide_button, rcstyle);
  // g_clear_object (&rcstyle);

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
layer_dialog_show (void)
{
  if (is_integrated_ui () == FALSE) {
    if (layer_dialog == NULL || layer_dialog->dialog == NULL) {
      layer_dialog_create();
    }
    g_assert (layer_dialog != NULL); /* must be valid now */
    gtk_window_present (GTK_WINDOW (layer_dialog->dialog));
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

struct _DiaLayerChange {
  DiaChange change;

  enum LayerChangeType type;
  DiaLayer *layer;
  int index;
  int applied;
};

DIA_DEFINE_CHANGE (DiaLayerChange, dia_layer_change)


static void
dia_layer_change_apply (DiaChange *self, DiagramData *dia)
{
  DiaLayerChange *change = DIA_LAYER_CHANGE (self);
  change->applied = 1;

  switch (change->type) {
    case TYPE_DELETE_LAYER:
      data_remove_layer (dia, change->layer);
      break;
    case TYPE_ADD_LAYER:
      data_add_layer_at (dia, change->layer, change->index);
      break;
    case TYPE_RAISE_LAYER:
      data_raise_layer (dia, change->layer);
      break;
    case TYPE_LOWER_LAYER:
      data_lower_layer (dia, change->layer);
      break;
    default:
      g_return_if_reached ();
  }

  diagram_add_update_all (DIA_DIAGRAM (dia));
}


static void
dia_layer_change_revert (DiaChange *self, DiagramData *dia)
{
  DiaLayerChange *change = DIA_LAYER_CHANGE (self);

  switch (change->type) {
    case TYPE_DELETE_LAYER:
      data_add_layer_at (dia, change->layer, change->index);
      break;
    case TYPE_ADD_LAYER:
      data_remove_layer (dia, change->layer);
      break;
    case TYPE_RAISE_LAYER:
      data_lower_layer (dia, change->layer);
      break;
    case TYPE_LOWER_LAYER:
      data_raise_layer (dia, change->layer);
      break;
    default:
      g_return_if_reached ();
  }

  diagram_add_update_all (DIA_DIAGRAM (dia));

  change->applied = 0;
}


static void
dia_layer_change_free (DiaChange *self)
{
  DiaLayerChange *change = DIA_LAYER_CHANGE (self);

  g_clear_object (&change->layer);
}


DiaChange *
dia_layer_change_new (Diagram              *dia,
                      DiaLayer             *layer,
                      enum LayerChangeType  type,
                      int                   index)
{
  DiaLayerChange *change = dia_change_new (DIA_TYPE_LAYER_CHANGE);

  change->type = type;
  g_set_object (&change->layer, layer);
  change->index = index;
  change->applied = 1;

  undo_push_change (dia->undo, DIA_CHANGE (change));

  return DIA_CHANGE (change);
}



struct _DiaLayerVisibilityChange {
  DiaChange change;

  GList *original_visibility;
  DiaLayer *layer;
  gboolean is_exclusive;
  int applied;
};

DIA_DEFINE_CHANGE (DiaLayerVisibilityChange, dia_layer_visibility_change)


void
dia_layer_visibility_change_apply (DiaChange   *self,
                                   DiagramData *dia)
{
  DiaLayerVisibilityChange *change = DIA_LAYER_VISIBILITY_CHANGE (self);
  DiaLayer *layer = change->layer;
  int visible = FALSE;

  if (change->is_exclusive) {
    /*  First determine if _any_ other layer widgets are set to visible.
     *  If there is, exclusive switching turns all off.  */
    DIA_FOR_LAYER_IN_DIAGRAM (dia, temp_layer, i, {
      if (temp_layer != layer) {
        visible |= dia_layer_is_visible (temp_layer);
      }
    });

    /*  Now, toggle the visibility for all layers except the specified one  */
    DIA_FOR_LAYER_IN_DIAGRAM (dia, temp_layer, i, {
      if (temp_layer == layer) {
        dia_layer_set_visible (temp_layer, TRUE);
      } else {
        dia_layer_set_visible (temp_layer, !visible);
      }
    });
  } else {
    dia_layer_set_visible (layer, !dia_layer_is_visible (layer));
  }

  diagram_add_update_all (DIA_DIAGRAM (dia));
}


/*
 * Revert to the visibility before this change was applied.
 */
static void
dia_layer_visibility_change_revert (DiaChange   *self,
                                    DiagramData *dia)
{
  DiaLayerVisibilityChange *change = DIA_LAYER_VISIBILITY_CHANGE (self);
  GList *vis = change->original_visibility;
  int i;

  for (i = 0; vis != NULL && i < data_layer_count (dia); vis = g_list_next (vis), i++) {
    DiaLayer *layer = data_layer_get_nth (dia, i);
    dia_layer_set_visible (layer, GPOINTER_TO_INT (vis->data));
  }

  if (vis != NULL || i < data_layer_count (dia)) {
    g_critical ("Internal error: visibility undo has %d visibilities, but %d layers\n",
                g_list_length (change->original_visibility),
                data_layer_count (dia));
  }

  diagram_add_update_all (DIA_DIAGRAM (dia));
}


static void
dia_layer_visibility_change_free (DiaChange *self)
{
  DiaLayerVisibilityChange *change = DIA_LAYER_VISIBILITY_CHANGE (self);

  g_list_free (change->original_visibility);
}


DiaChange *
dia_layer_visibility_change_new (Diagram *dia, DiaLayer *layer, gboolean exclusive)
{
  DiaLayerVisibilityChange *change = dia_change_new (DIA_TYPE_LAYER_VISIBILITY_CHANGE);
  GList *visibilities = NULL;

  DIA_FOR_LAYER_IN_DIAGRAM (DIA_DIAGRAM_DATA (dia), temp_layer, i, {
    visibilities = g_list_append (visibilities,
                                  GINT_TO_POINTER (dia_layer_is_visible (temp_layer)));
  });

  change->original_visibility = visibilities;
  change->layer = layer;
  change->is_exclusive = exclusive;

  undo_push_change (dia->undo, DIA_CHANGE (change));

  return DIA_CHANGE (change);
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
