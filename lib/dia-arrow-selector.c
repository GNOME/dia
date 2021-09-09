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

#include "dia-arrow-cell-renderer.h"
#include "dia-arrow-selector.h"
#include "dia-size-selector.h"


enum {
  COL_ARROW,
  N_COL
};


struct _DiaArrowSelector {
  GtkBox vbox;

  GtkBox *sizebox;
  GtkLabel *sizelabel;
  DiaSizeSelector *size;

  GtkWidget    *combo;
  GtkListStore *arrow_store;

  ArrowType     looking_for;
};

G_DEFINE_TYPE (DiaArrowSelector, dia_arrow_selector, GTK_TYPE_BOX)

enum {
    DAS_VALUE_CHANGED,
    DAS_LAST_SIGNAL
};

static guint das_signals[DAS_LAST_SIGNAL] = {0};


static void
dia_arrow_selector_finalize (GObject *object)
{
  DiaArrowSelector *self = DIA_ARROW_SELECTOR (object);

  g_clear_object (&self->arrow_store);

  G_OBJECT_CLASS (dia_arrow_selector_parent_class)->finalize (object);
}


static void
dia_arrow_selector_class_init (DiaArrowSelectorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = dia_arrow_selector_finalize;

  das_signals[DAS_VALUE_CHANGED] = g_signal_new ("value_changed",
                                                 G_TYPE_FROM_CLASS (klass),
                                                 G_SIGNAL_RUN_FIRST,
                                                 0, NULL, NULL,
                                                 g_cclosure_marshal_VOID__VOID,
                                                 G_TYPE_NONE, 0);
}


static void
set_size_sensitivity (DiaArrowSelector *as)
{
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (as->combo), &iter)) {
    Arrow *active = NULL;

    gtk_tree_model_get (GTK_TREE_MODEL (as->arrow_store),
                        &iter,
                        COL_ARROW, &active,
                        -1);

    gtk_widget_set_sensitive (GTK_WIDGET (as->sizelabel),
                              active->type != ARROW_NONE);
    gtk_widget_set_sensitive (GTK_WIDGET (as->size),
                              active->type != ARROW_NONE);

    dia_arrow_free (active);
  } else {
    gtk_widget_set_sensitive (GTK_WIDGET (as->sizelabel), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (as->size), FALSE);
  }
}


static void
arrow_type_change_callback (GtkComboBox *widget, gpointer userdata)
{
  set_size_sensitivity (DIA_ARROW_SELECTOR (userdata));
  g_signal_emit (DIA_ARROW_SELECTOR (userdata),
                 das_signals[DAS_VALUE_CHANGED],
                 0);
}


static void
arrow_size_change_callback(DiaSizeSelector *size, gpointer userdata)
{
  g_signal_emit (DIA_ARROW_SELECTOR (userdata),
                 das_signals[DAS_VALUE_CHANGED],
                 0);
}


static void
dia_arrow_selector_init (DiaArrowSelector *as)
{
  GtkWidget *box;
  GtkWidget *label;
  GtkWidget *size;
  GtkTreeIter iter;
  GtkCellRenderer *renderer;

  gtk_orientable_set_orientation (GTK_ORIENTABLE(as), GTK_ORIENTATION_VERTICAL);
  as->arrow_store = gtk_list_store_new (N_COL, DIA_TYPE_ARROW);

  for (int i = ARROW_NONE; i < MAX_ARROW_TYPE; ++i) {
    ArrowType arrow_type = arrow_type_from_index (i);
    Arrow arrow = { arrow_type, 0.5, 0.5 };

    gtk_list_store_append (as->arrow_store, &iter);
    gtk_list_store_set (as->arrow_store,
                        &iter,
                        COL_ARROW, &arrow,
                        -1);
  }

  as->combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (as->arrow_store));
  g_signal_connect (as->combo,
                    "changed",
                    G_CALLBACK (arrow_type_change_callback),
                    as);

  renderer = dia_arrow_cell_renderer_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (as->combo), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (as->combo),
                                  renderer,
                                  "arrow", COL_ARROW,
                                  NULL);

  gtk_box_pack_start (GTK_BOX (as), as->combo, FALSE, TRUE, 0);
  gtk_widget_show (as->combo);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  as->sizebox = GTK_BOX(box);

  label = gtk_label_new (_("Size: "));
  as->sizelabel = GTK_LABEL (label);
  gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  size = dia_size_selector_new (0.0, 0.0);
  as->size = DIA_SIZE_SELECTOR (size);
  gtk_box_pack_start (GTK_BOX (box), size, TRUE, TRUE, 0);
  gtk_widget_show (size);
  g_signal_connect (size,
                    "value-changed", G_CALLBACK (arrow_size_change_callback),
                    as);

  set_size_sensitivity (as);
  gtk_box_pack_start (GTK_BOX (as), box, TRUE, TRUE, 0);

  gtk_widget_show (box);
}


GtkWidget *
dia_arrow_selector_new (void)
{
  return g_object_new (DIA_TYPE_ARROW_SELECTOR, NULL);
}


Arrow
dia_arrow_selector_get_arrow (DiaArrowSelector *as)
{
  Arrow at;
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (as->combo), &iter)) {
    Arrow *active = NULL;

    gtk_tree_model_get (GTK_TREE_MODEL (as->arrow_store),
                        &iter,
                        COL_ARROW, &active,
                        -1);

    at.type = active->type;

    dia_arrow_free (active);
  } else {
    at.type = ARROW_NONE;
  }

  dia_size_selector_get_size (as->size, &at.width, &at.length);

  return at;
}


static gboolean
set_type (GtkTreeModel *model,
          GtkTreePath  *path,
          GtkTreeIter  *iter,
          gpointer      data)
{
  DiaArrowSelector *self = DIA_ARROW_SELECTOR (data);
  Arrow *arrow;
  gboolean res = FALSE;

  gtk_tree_model_get (model,
                      iter,
                      COL_ARROW, &arrow,
                      -1);

  res = arrow->type == self->looking_for;
  if (res) {
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (self->combo), iter);
  }

  dia_arrow_free (arrow);

  return res;
}


void
dia_arrow_selector_set_arrow (DiaArrowSelector *as,
                              Arrow             arrow)
{
  as->looking_for = arrow.type;
  gtk_tree_model_foreach (GTK_TREE_MODEL (as->arrow_store), set_type, as);
  as->looking_for = ARROW_NONE;

  dia_size_selector_set_size (DIA_SIZE_SELECTOR (as->size), arrow.width, arrow.length);
}
