/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diagram_tree_menu_callbacks.c : callbacks for the diagram tree menus.
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

#include <gtk/gtk.h>

#include "diagram_tree_menu_callbacks.h"
#include "diagram_tree_menu.h"
#include "diagram_tree_window.h"
#include "preferences.h"

void
on_locate_object_activate(gpointer user_data,
			  guint action, GtkMenuItem *menuitem)
			  
{
  diagram_tree_raise((DiagramTree *)user_data);
}

void
on_properties_activate(gpointer user_data,
		       guint action, GtkMenuItem *menuitem)
{
  diagram_tree_show_properties((DiagramTree *)user_data);
}

void
on_sort_objects_activate(gpointer user_data,
			 guint action, GtkMenuItem *menuitem)
{
  diagram_tree_sort_objects((DiagramTree *)user_data, action);
}

void
on_sort_all_objects_activate(gpointer user_data,
			     guint action, GtkMenuItem *menuitem)
{
  diagram_tree_sort_all_objects((DiagramTree *)user_data, action);
}

void
on_sort_def_activate(gpointer user_data,
		     guint action, GtkMenuItem *item)
{
  prefs.dia_tree.obj_sort = action;
  diagram_tree_set_object_sort_type((DiagramTree *)user_data, action);
}

void
on_delete_object_activate(gpointer user_data,
			  guint action, GtkMenuItem *menuitem)
{

}

void
on_locate_dia_activate(gpointer user_data,
		       guint action, GtkMenuItem *menuitem)
{
  diagram_tree_raise((DiagramTree *)user_data);
}


void
on_sort_diagrams_activate(gpointer user_data,
			  guint action, GtkMenuItem *menuitem)
{
  diagram_tree_sort_diagrams((DiagramTree *)user_data, action);
}


void
on_sort_dia_def_activate(gpointer user_data,
			 guint action, GtkMenuItem *item)
{
  prefs.dia_tree.dia_sort = action;
  diagram_tree_set_diagram_sort_type((DiagramTree *)user_data, action);
}

void
on_hide_object_activate(gpointer user_data,
			guint action, GtkMenuItem *menuitem)
{
  const gchar *type = diagram_tree_hide_type((DiagramTree *)user_data);
}
