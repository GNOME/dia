/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diagram_tree_window.h : a window showing open diagrams
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


#ifndef DIAGRAM_TREE_WINDOW_H
#define DIAGRAM_TREE_WINDOW_H

#include <gtk/gtk.h>
#include "diagram_tree.h"

/* configuration parameters */
typedef struct _DiagramTreeConfig DiagramTreeConfig;

struct _DiagramTreeConfig {
  gboolean show_tree;		/* show dtree at startup */
  gboolean save_size;		/* save current size as default */
  guint width;			/* default width of the dtree window */
  guint height;			/* default height of the dtree window */
  DiagramTreeSortType dia_sort;	/* default sort mode for diagrams */
  DiagramTreeSortType obj_sort;	/* default sort mode for objects */
  gboolean save_hidden;		/* remember current hidden types on restart */
  gchar *hidden;		/* hidden object type list */
};

/* for use as default initialisation value */
extern gchar *DIA_TREE_DEFAULT_HIDDEN;

/* get the diagram tree and window*/
extern DiagramTree *
diagram_tree(void);

extern void
create_diagram_tree_window(DiagramTreeConfig *config, GtkWidget *menuitem);


/* show the diagtree window (menu callback) */
extern void
diagtree_show_callback(gpointer data, guint action,
		       GtkWidget *widget);


/* functions manipulating the config */
extern GList * /* destroyed by client */
diagram_tree_config_get_hidden_types(const DiagramTreeConfig *config);

extern void
diagram_tree_config_set_hidden_types(DiagramTreeConfig *config,
				     const GList *types);

extern void
diagram_tree_config_add_hidden_type(DiagramTreeConfig *config,
				    const gchar *type);

extern void
diagram_tree_config_remove_hidden_type(DiagramTreeConfig *config,
				       const gchar *type);


#endif /* DIAGRAM_TREE_WINDOW_H */
