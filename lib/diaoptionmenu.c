/* Dia -- a diagram creation/manipulation program -*- c -*-
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 *
 * Copyright (C) 2010 Hans Breuer
 * Property type for affine transformation.
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

#include "diaoptionmenu.h"

enum {
  COL_NAME = 0,
  COL_VALUE,
  COL_N_COLS
};

/*!
 * GtkOptionMenu replacement specialized for Dia's use
 * of name (menu entry) to value (int)
 */
GtkWidget *
dia_option_menu_new (void)
{
  GtkWidget *combo_box;
  GtkListStore *model = gtk_list_store_new (COL_N_COLS, G_TYPE_STRING, G_TYPE_INT);
  GtkCellRenderer *cell;

  combo_box = gtk_combo_box_new_with_model (GTK_TREE_MODEL (model));

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), cell,
                                  "text", 0,
                                  NULL);

  return combo_box;
}

/*!
 * Convenient form of gtk_menu_append and more
 */
void
dia_option_menu_add_item (GtkWidget *widget, const char *name, int value)
{
  GtkComboBox  *combo_box = GTK_COMBO_BOX (widget);
  GtkTreeModel *model = gtk_combo_box_get_model (combo_box);
  GtkTreeIter iter;

  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      COL_NAME, name,
		      COL_VALUE, value,
		      -1);
}

/*!
 * drop in replacement gtk_option_menu_set_history
 */
void
dia_option_menu_set_active (GtkWidget *widget, int active)
{
  GtkComboBox  *combo_box = GTK_COMBO_BOX (widget);
  GtkTreeModel *model = gtk_combo_box_get_model (combo_box);
  GtkTreeIter iter;

  if (!gtk_tree_model_get_iter_first (model, &iter)) {
    g_warning ("Empty DiaOptionMenu?");
    return;
  }
  do {
    int value;
    gtk_tree_model_get (model, &iter, COL_VALUE, &value, -1);
    if (active == value) {
      gtk_combo_box_set_active_iter (combo_box, &iter);
      break;
    }
  } while (gtk_tree_model_iter_next (model, &iter));
}

/*!
 * drop in replacement gtk_option_menu_get_history
 */
int
dia_option_menu_get_active (GtkWidget *widget)
{
  GtkComboBox  *combo_box = GTK_COMBO_BOX (widget);
  GtkTreeModel *model = gtk_combo_box_get_model (combo_box);
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter  (combo_box, &iter)) {
    int value;

    gtk_tree_model_get (model, &iter, COL_VALUE, &value, -1);
    return value;
  }
  g_warning ("DiaOptionMenu: no selection");
  return 0;
}
