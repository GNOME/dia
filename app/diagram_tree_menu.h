/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diagram_tree_menu.h : menus for the diagram tree.
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

#ifndef DIAGRAM_TREE_MENU_H
#define DIAGRAM_TREE_MENU_H

#include "diagram_tree.h"

typedef struct _DiagramTreeMenus DiagramTreeMenus;
typedef enum {
  DIA_MENU_DIAGRAM,
  DIA_MENU_OBJECT
} DiagramTreeMenuType;

extern DiagramTreeMenus *
diagram_tree_menus_new(DiagramTree *tree, GtkWindow *window);


extern GtkWidget *
diagram_tree_menus_get_menu(const DiagramTreeMenus *menus,
			    DiagramTreeMenuType type);

extern void
diagram_tree_menus_popup_menu(const DiagramTreeMenus *menus,
			      DiagramTreeMenuType type, gint time);


extern void
diagram_tree_menus_add_hidden_type(DiagramTreeMenus *menus,
				   const gchar *type);

extern void
diagram_tree_menus_remove_hidden_type(DiagramTreeMenus *menus,
				      const gchar *type);

#endif /* DIAGRAM_TREE_MENU_H */
