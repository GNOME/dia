/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diagram_tree.h : a tree showing open diagrams
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


#ifndef DIAGRAM_TREE_H
#define DIAGRAM_TREE_H

#include <gtk/gtk.h>
#include "diagram.h"

/* the diagram tree adt */
typedef struct _DiagramTree DiagramTree;


/* sort types */
typedef enum {
  DIA_TREE_SORT_NAME,
  DIA_TREE_SORT_TYPE,
  DIA_TREE_SORT_INSERT
} DiagramTreeSortType;



/* create a new tree with a list of already open diagrams */
extern DiagramTree*
diagram_tree_new(GList *diagrams);

/* delete a tree (the widget is destroyed, but not the diagrams */
extern void
diagram_tree_delete(DiagramTree *tree);

/* add a diagram to the tree */
extern void
diagram_tree_add(DiagramTree *tree, Diagram *diagram);

/* remove a diagram from the tree */
extern void
diagram_tree_remove(DiagramTree *tree, Diagram *diagram);

/* update a diagram in the tree */
extern void
diagram_tree_update(DiagramTree *tree, Diagram *diagram);

extern void
diagram_tree_update_name(DiagramTree *tree, Diagram *diagram);

/* add/remove an object in a diagram already contained in the tree */
extern void
diagram_tree_add_object(DiagramTree *tree, Diagram *diagram, Object *object);

extern void
diagram_tree_add_objects(DiagramTree *tree, Diagram *diagram, GList *objects);

extern void
diagram_tree_remove_object(DiagramTree *tree, Object *object);

extern void
diagram_tree_remove_objects(DiagramTree *tree, GList *objects);

extern void
diagram_tree_update_object(DiagramTree *tree, Diagram *diagram,
			   Object *object);

/* operations on the last clicked node */
extern void
diagram_tree_raise(DiagramTree *tree);

extern void
diagram_tree_show_properties(const DiagramTree *tree);

extern void
diagram_tree_sort_objects(DiagramTree *tree, DiagramTreeSortType type);

extern void
diagram_tree_sort_all_objects(DiagramTree *tree, DiagramTreeSortType type);

extern void
diagram_tree_sort_diagrams(DiagramTree *tree, DiagramTreeSortType type);

extern void
diagram_tree_set_object_sort_type(DiagramTree *tree, DiagramTreeSortType type);

extern void
diagram_tree_set_diagram_sort_type(DiagramTree *tree, DiagramTreeSortType type);

extern DiagramTreeSortType
diagram_tree_diagram_sort_type(const DiagramTree *tree);

extern DiagramTreeSortType
diagram_tree_object_sort_type(const DiagramTree *tree);

/* attach menus */
extern void
diagram_tree_attach_dia_menu(DiagramTree *tree, GtkWidget *menu);

extern void
diagram_tree_attach_obj_menu(DiagramTree *tree, GtkWidget *menu);


/* get the tree widget */
extern GtkWidget*
diagram_tree_widget(const DiagramTree *tree);


#endif /* DIAGRAM_TREE_H */
