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

#include "dia-line-cell-renderer.h"
#include "dia-line-style-selector.h"

enum {
  COL_LINE,
  N_COL
};


struct _DiaLineStyleSelector {
  GtkBox vbox;

  GtkLabel *lengthlabel;
  GtkSpinButton *dashlength;

  GtkWidget    *combo;
  GtkListStore *line_store;
  DiaLineStyle  looking_for;
};

G_DEFINE_TYPE (DiaLineStyleSelector, dia_line_style_selector, GTK_TYPE_BOX)

enum {
  DLS_VALUE_CHANGED,
  DLS_LAST_SIGNAL
};

static guint dls_signals[DLS_LAST_SIGNAL] = { 0 };

static void
dia_line_style_selector_class_init (DiaLineStyleSelectorClass *klass)
{
  dls_signals[DLS_VALUE_CHANGED] = g_signal_new ("value-changed",
                                                 G_TYPE_FROM_CLASS (klass),
                                                 G_SIGNAL_RUN_FIRST,
                                                 0, NULL, NULL,
                                                 g_cclosure_marshal_VOID__VOID,
                                                 G_TYPE_NONE, 0);
}


static void
set_linestyle_sensitivity (DiaLineStyleSelector *fs)
{
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (fs->combo), &iter)) {
    DiaLineStyle line;

    gtk_tree_model_get (GTK_TREE_MODEL (fs->line_store),
                        &iter,
                        COL_LINE, &line,
                        -1);

    gtk_widget_set_sensitive (GTK_WIDGET (fs->lengthlabel),
                              line != DIA_LINE_STYLE_SOLID);
    gtk_widget_set_sensitive (GTK_WIDGET (fs->dashlength),
                              line != DIA_LINE_STYLE_SOLID);
  } else {
    gtk_widget_set_sensitive (GTK_WIDGET (fs->lengthlabel), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (fs->dashlength), FALSE);
  }
}


static void
linestyle_type_change_callback (GtkComboBox *menu, gpointer data)
{
  set_linestyle_sensitivity (DIA_LINE_STYLE_SELECTOR (data));
  g_signal_emit (DIA_LINE_STYLE_SELECTOR (data),
                 dls_signals[DLS_VALUE_CHANGED],
                 0);
}


static void
linestyle_dashlength_change_callback (GtkSpinButton *sb, gpointer data)
{
  g_signal_emit (DIA_LINE_STYLE_SELECTOR (data),
                 dls_signals[DLS_VALUE_CHANGED],
                 0);
}


static void
dia_line_style_selector_init (DiaLineStyleSelector *fs)
{
  GtkWidget *label;
  GtkWidget *length;
  GtkWidget *box;
  GtkAdjustment *adj;
  GtkTreeIter iter;
  GtkCellRenderer *renderer;

  gtk_orientable_set_orientation (GTK_ORIENTABLE(fs), GTK_ORIENTATION_VERTICAL);
  fs->line_store = gtk_list_store_new (N_COL, DIA_TYPE_LINE_STYLE);

  for (int i = 0; i <= DIA_LINE_STYLE_DOTTED; i++) {
    gtk_list_store_append (fs->line_store, &iter);
    gtk_list_store_set (fs->line_store,
                        &iter,
                        COL_LINE, i,
                        -1);
  }

  fs->combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (fs->line_store));
  g_signal_connect (fs->combo,
                    "changed",
                    G_CALLBACK (linestyle_type_change_callback),
                    fs);

  renderer = dia_line_cell_renderer_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (fs->combo), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (fs->combo),
                                  renderer,
                                  "line", COL_LINE,
                                  NULL);

  gtk_box_pack_start (GTK_BOX (fs), fs->combo, FALSE, TRUE, 0);
  gtk_widget_show (fs->combo);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  /*  fs->sizebox = GTK_HBOX(box); */

  label = gtk_label_new(_("Dash length: "));
  fs->lengthlabel = GTK_LABEL (label);
  gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  adj = GTK_ADJUSTMENT (gtk_adjustment_new (0.1, 0.00, 10.0, 0.1, 1.0, 0));
  length = gtk_spin_button_new (adj, DEFAULT_LINESTYLE_DASHLEN, 2);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON(length), TRUE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON(length), TRUE);
  fs->dashlength = GTK_SPIN_BUTTON (length);
  gtk_box_pack_start (GTK_BOX (box), length, TRUE, TRUE, 0);
  gtk_widget_show (length);

  g_signal_connect (G_OBJECT (length),
                    "changed", G_CALLBACK (linestyle_dashlength_change_callback),
                    fs);

  set_linestyle_sensitivity (fs);
  gtk_box_pack_start (GTK_BOX (fs), box, TRUE, TRUE, 0);
  gtk_widget_show (box);
}


GtkWidget *
dia_line_style_selector_new (void)
{
  return g_object_new (DIA_TYPE_LINE_STYLE_SELECTOR, NULL);
}


void
dia_line_style_selector_get_linestyle (DiaLineStyleSelector *fs,
                                       DiaLineStyle         *ls,
                                       double               *dl)
{
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (fs->combo), &iter)) {
    gtk_tree_model_get (GTK_TREE_MODEL (fs->line_store),
                        &iter,
                        COL_LINE, ls,
                        -1);
  } else {
    *ls = DIA_LINE_STYLE_DEFAULT;
  }

  if (dl != NULL) {
    *dl = gtk_spin_button_get_value (fs->dashlength);
  }
}


static gboolean
set_style (GtkTreeModel *model,
           GtkTreePath  *path,
           GtkTreeIter  *iter,
           gpointer      data)
{
  DiaLineStyleSelector *self = DIA_LINE_STYLE_SELECTOR (data);
  DiaLineStyle line;
  gboolean res = FALSE;

  gtk_tree_model_get (model,
                      iter,
                      COL_LINE, &line,
                      -1);

  res = line == self->looking_for;
  if (res) {
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (self->combo), iter);
  }

  return res;
}


void
dia_line_style_selector_set_linestyle (DiaLineStyleSelector *as,
                                       DiaLineStyle          linestyle,
                                       double                dashlength)
{
  as->looking_for = linestyle;
  gtk_tree_model_foreach (GTK_TREE_MODEL (as->line_store), set_style, as);
  as->looking_for = DIA_LINE_STYLE_DEFAULT;

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (as->dashlength), dashlength);
}
