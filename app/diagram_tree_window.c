/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diagram_tree_window.c : a window showing open diagrams
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
#include <config.h>
#endif

#include "intl.h"
#include "diagram.h"
#include "diagram_tree.h"
#include "diagram_tree_window.h"

GtkWidget *diagwindow_ = NULL;
GtkCheckMenuItem *menu_item_ = NULL;
DiagramTree *diagtree_ = NULL;

/* diagtree window hide callback */
static void
diagram_tree_window_hide(GtkWidget *window)
{
  if (menu_item_) menu_item_->active = FALSE;
  gtk_widget_hide(window);
}

/* create a diagram_tree_window window */
static GtkWidget*
diagram_tree_window_new(void) 
{
  GtkWidget *window;
  GtkWidget *scroll;
  GtkWidget *tree;
  
  /* the diagtree */
  if (!diagtree_) diagtree_ = diagram_tree_new(open_diagrams);
  tree = diagram_tree_widget(diagtree_);

  /* Create a new window */
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  g_return_val_if_fail(window, NULL);
  
  gtk_window_set_title(GTK_WINDOW(window), N_("Diagram tree"));
  gtk_widget_set_usize(window, 200, 250);

  /* simply hide the window when it is closed */
  gtk_signal_connect(GTK_OBJECT(window), "destroy",
		     GTK_SIGNAL_FUNC(diagram_tree_window_hide), NULL);

  gtk_signal_connect(GTK_OBJECT(window), "delete_event",
		     GTK_SIGNAL_FUNC(diagram_tree_window_hide), NULL);

  /* put the tree in a scrolled window */
  scroll = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(scroll), tree);
  gtk_container_add(GTK_CONTAINER(window), scroll);

  gtk_widget_show(tree);
  gtk_widget_show(scroll);
  gtk_widget_realize(window);

  return window;
}

/* ---------------------- external interface ---------------------------- */
DiagramTree *
diagram_tree(void)
{
  if (!diagtree_) diagtree_ = diagram_tree_new(NULL);
  return diagtree_;
}

GtkWidget *
diagram_tree_window(void)
{
  if (!diagwindow_) diagwindow_ = diagram_tree_window_new();
  return diagwindow_;
}

/* menu callbacks */
void
diagtree_show_callback(gpointer data, guint action, GtkWidget *widget)
{
  if (!diagwindow_) {
    diagwindow_ = diagram_tree_window_new();
  } else {
    GList *open = open_diagrams;
    while (open) {
      diagram_tree_add(diagtree_, (Diagram *)open->data);
      open = g_list_next(open);
    }
  }
  if (!menu_item_) menu_item_ = GTK_CHECK_MENU_ITEM(widget);
  if (menu_item_->active)
    gtk_widget_show(diagwindow_);
  else
    gtk_widget_hide(diagwindow_);
}
