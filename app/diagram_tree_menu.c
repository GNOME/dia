/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diagram_tree_menu.c : menus for the diagram tree.
 * Copyright (C) 2001 Jose A Ortega Ruiz
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
 *  
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "diagram_tree_menu_callbacks.h"
#include "diagram_tree_menu.h"

static GtkItemFactoryEntry common_items_[] = {
  { "/sep0", NULL, NULL, 0, "<Separator>" },
  { N_("/_Sort objects"), NULL, NULL, 0, "<Branch>" },
  { N_("/Sort objects/by _name"), NULL, on_sort_objects_activate,
    DIA_TREE_SORT_NAME, "" },
  { N_("/Sort objects/by _type"), NULL, on_sort_objects_activate,
    DIA_TREE_SORT_TYPE, "" },
  { N_("/Sort objects/as _inserted"), NULL, on_sort_objects_activate,
    DIA_TREE_SORT_INSERT, "" },
  { "/Sort objects/sep0", NULL, NULL, 0, "<Separator>" },
  { N_("/Sort objects/All by name"), NULL, on_sort_all_objects_activate,
    DIA_TREE_SORT_NAME, "" },
  { N_("/Sort objects/All by type"), NULL, on_sort_all_objects_activate,
    DIA_TREE_SORT_TYPE, "" },
  { N_("/Sort objects/All as inserted"), NULL, on_sort_all_objects_activate,
    DIA_TREE_SORT_INSERT, "" },
  { N_("/Sort objects/_Default"), NULL, NULL, 0, "<Branch>"},
  { N_("/Sort objects/Default/by _name"), NULL, on_sort_def_activate,
    DIA_TREE_SORT_NAME, "<RadioItem>" },
  { N_("/Sort objects/Default/by _type"), NULL, on_sort_def_activate,
    DIA_TREE_SORT_TYPE, "/Sort objects/Default/by name" },
  { N_("/Sort objects/Default/as _inserted"), NULL, on_sort_def_activate,
    DIA_TREE_SORT_INSERT, "/Sort objects/Default/by name" },
  { N_("/Sort _diagrams"), NULL, NULL, 0, "<Branch>" },
  { N_("/Sort _diagrams/by _name"), NULL, on_sort_diagrams_activate,
    DIA_TREE_SORT_NAME, "" },
  { N_("/Sort _diagrams/as _inserted"), NULL, on_sort_diagrams_activate,
    DIA_TREE_SORT_INSERT, "" },
  { N_("/Sort diagrams/_Default"), NULL, NULL, 0, "<Branch>"},
  { N_("/Sort diagrams/Default/by _name"), NULL, on_sort_dia_def_activate,
    DIA_TREE_SORT_NAME, "<RadioItem>" },
  { N_("/Sort diagrams/Default/as _inserted"), NULL, on_sort_dia_def_activate,
    DIA_TREE_SORT_INSERT, "/Sort diagrams/Default/by name" },
};

static const gint COMMON_ITEMS_SIZE_ =
sizeof (common_items_) / sizeof (common_items_[0]);

static GtkItemFactoryEntry object_items_[] = {
  { N_("/_Locate"), NULL, on_locate_object_activate, 0, "" },
  { N_("/_Properties"), NULL, on_properties_activate, 0, "" },
  /*  { "/sep1", NULL, NULL, 0, "<Separator>" },
      { N_("/_Delete"), NULL, on_delete_object_activate, 0, "" }, */
};

static const gint OBJ_ITEMS_SIZE_ =
sizeof (object_items_) / sizeof (object_items_[0]);

static GtkItemFactoryEntry dia_items_[] = {
  { N_("/_Locate"), NULL, on_locate_dia_activate, 0, "" },
};

static const gint DIA_ITEMS_SIZE_ =
sizeof (dia_items_) / sizeof (dia_items_[0]);

static GtkWidget*
create_dmenu(DiagramTree *tree, GtkWindow *window, gint no,
	    GtkItemFactoryEntry entries[], const gchar *path)
{
  GtkItemFactory *factory;
  GtkAccelGroup *accel = gtk_accel_group_new();
  gchar *item_path = NULL;
  GtkWidget *menu;
  
  factory = gtk_item_factory_new(GTK_TYPE_MENU, path, accel);
  gtk_item_factory_create_items(factory, no, entries,tree);
  gtk_item_factory_create_items(factory, COMMON_ITEMS_SIZE_, common_items_,
				tree);
  gtk_window_add_accel_group(window, accel);

  switch (diagram_tree_object_sort_type(tree)) {
  case DIA_TREE_SORT_NAME:
    item_path = g_strconcat(path, "/Sort objects/Default/by name", NULL);
    break;
  case DIA_TREE_SORT_TYPE:
    item_path = g_strconcat(path, "/Sort objects/Default/by type", NULL);
    break;
  default:
  case DIA_TREE_SORT_INSERT:
    item_path = g_strconcat(path, "/Sort objects/Default/as inserted", NULL);
    break;
  }

  menu = gtk_item_factory_get_widget(factory, item_path);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), TRUE);
  g_free(item_path);
  
  switch (diagram_tree_diagram_sort_type(tree)) {
  case DIA_TREE_SORT_NAME:
    item_path = g_strconcat(path, "/Sort diagrams/Default/by name", NULL);
    break;
  default:
  case DIA_TREE_SORT_INSERT:
    item_path = g_strconcat(path, "/Sort diagrams/Default/as inserted", NULL);
    break;
  }

  menu = gtk_item_factory_get_widget(factory, item_path);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), TRUE);
  g_free(item_path);

  return gtk_item_factory_get_widget(factory, path);
}

void
create_dtree_object_menu(DiagramTree *tree, GtkWindow *window)
{
  GtkWidget *menu = create_dmenu(tree, window, OBJ_ITEMS_SIZE_, object_items_,
				 "<DiagramTreeObj>");
  diagram_tree_attach_obj_menu(tree, menu);
}

void
create_dtree_dia_menu(DiagramTree *tree, GtkWindow *window)
{
  GtkWidget *menu = create_dmenu(tree, window, DIA_ITEMS_SIZE_, dia_items_,
				 "<DiagramTreeDia>");
  diagram_tree_attach_dia_menu(tree, menu);
}

