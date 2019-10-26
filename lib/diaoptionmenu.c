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

/**
 * SECTION:diaoptionmenu
 * @title: DiaOptionMenu
 * @short_description: name -> value selector
 *
 * GtkOptionMenu replacement specialized for Dia's use
 * of name (menu entry) to value (int)
 *
 * #GtkComboBox is an implementation detail, use of anything other than the
 * #GtkComboBox:changed signal is undefined behaviour
 */


typedef struct _DiaOptionMenuPrivate DiaOptionMenuPrivate;
struct _DiaOptionMenuPrivate {
  GtkListStore *model;
};

G_DEFINE_TYPE_WITH_PRIVATE (DiaOptionMenu, dia_option_menu, GTK_TYPE_COMBO_BOX)

enum {
  COL_NAME = 0,
  COL_VALUE,
  COL_N_COLS
};


static void
dia_option_menu_class_init (DiaOptionMenuClass *klass)
{

}


static void
dia_option_menu_init (DiaOptionMenu *self)
{
  DiaOptionMenuPrivate *priv = dia_option_menu_get_instance_private (self);
  GtkCellRenderer *cell;

  priv->model = gtk_list_store_new (COL_N_COLS, G_TYPE_STRING, G_TYPE_INT);

  gtk_combo_box_set_model (GTK_COMBO_BOX (self), GTK_TREE_MODEL (priv->model));

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (self), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (self), cell,
                                  "text", COL_NAME,
                                  NULL);
}


GtkWidget *
dia_option_menu_new (void)
{
  return g_object_new (DIA_TYPE_OPTION_MENU, NULL);
}


/**
 * dia_option_menu_add_item:
 * @self: the #DiaOptionMenu
 * @name: the item name
 * @value: the item value
 *
 * Convenient form of gtk_menu_append and more
 */
void
dia_option_menu_add_item (DiaOptionMenu *self,
                          const char    *name,
                          int            value)
{
  DiaOptionMenuPrivate *priv;
  GtkTreeIter iter;

  g_return_if_fail (DIA_IS_OPTION_MENU (self));

  priv = dia_option_menu_get_instance_private (self);

  gtk_list_store_append (priv->model, &iter);
  gtk_list_store_set (priv->model, &iter,
                      COL_NAME, name,
                      COL_VALUE, value,
                      -1);
}


/**
 * dia_option_menu_set_active:
 * @self: the #DiaOptionMenu
 * @active: the new value
 *
 * drop in replacement gtk_option_menu_set_history
 */
void
dia_option_menu_set_active (DiaOptionMenu *self, int active)
{
  DiaOptionMenuPrivate *priv;
  GtkTreeIter iter;

  g_return_if_fail (DIA_IS_OPTION_MENU (self));

  priv = dia_option_menu_get_instance_private (self);

  g_return_if_fail (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->model), &iter));

  do {
    int value;
    gtk_tree_model_get (GTK_TREE_MODEL (priv->model),
                        &iter,
                        COL_VALUE, &value,
                        -1);
    if (active == value) {
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (self), &iter);
      break;
    }
  } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->model), &iter));
}


/**
 * dia_option_menu_get_active
 * @self: the #DiaOptionMenu
 *
 * drop in replacement gtk_option_menu_get_history
 */
int
dia_option_menu_get_active (DiaOptionMenu *self)
{
  DiaOptionMenuPrivate *priv;
  GtkTreeIter iter;

  g_return_val_if_fail (DIA_IS_OPTION_MENU (self), -1);

  priv = dia_option_menu_get_instance_private (self);

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (self), &iter)) {
    int value;

    gtk_tree_model_get (GTK_TREE_MODEL (priv->model),
                        &iter,
                        COL_VALUE, &value,
                        -1);

    return value;
  }

  g_return_val_if_reached (-1);
}
