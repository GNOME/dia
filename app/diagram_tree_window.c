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

#include <string.h>

#include "intl.h"
#include "preferences.h"
#include "diagram.h"
#include "diagram_tree.h"
#include "diagram_tree_window.h"
#include "diagram_tree_menu.h"


gchar *DIA_TREE_DEFAULT_HIDDEN = "DefaultDiaTreeTypeGuardDontChange!";

static GtkWidget *diagwindow_ = NULL;
static GtkCheckMenuItem *menu_item_ = NULL;
static DiagramTree *diagtree_ = NULL;
static DiagramTreeConfig *config_ = NULL;

/* diagtree window hide callback */
static void
diagram_tree_window_hide(GtkWidget *window)
{
  if (menu_item_) menu_item_->active = FALSE;
  gtk_widget_hide(window);
}

/* diagtree resize window callback */
static void
diagram_tree_window_size_request(GtkWidget *window, GtkAllocation *req,
				 gpointer data) 
{
  if (config_ && config_->save_size) {
    config_->width = req->width;
    config_->height = req->height;
    prefs_save();
  }
}

/* create a diagram_tree_window window */
static GtkWidget*
diagram_tree_window_new(DiagramTreeConfig *config) 
{
  GtkWidget *window;
  GtkWidget *scroll;
  GtkWidget *tree;
  
  /* Create a new window */
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  g_return_val_if_fail(window, NULL);
  
  gtk_window_set_title(GTK_WINDOW(window), N_("Diagram tree"));
  gtk_window_set_default_size(GTK_WINDOW(window),
			      config->width, config->height);

  /* simply hide the window when it is closed */
  gtk_signal_connect(GTK_OBJECT(window), "destroy",
		     GTK_SIGNAL_FUNC(diagram_tree_window_hide), NULL);

  gtk_signal_connect(GTK_OBJECT(window), "delete_event",
		     GTK_SIGNAL_FUNC(diagram_tree_window_hide), NULL);

  gtk_signal_connect(GTK_OBJECT(window), "size-allocate",
		     GTK_SIGNAL_FUNC(diagram_tree_window_size_request), NULL);

  /* the diagtree */
  if (!diagtree_)
    {
      diagtree_ = diagram_tree_new(open_diagrams, GTK_WINDOW(window),
				   config->dia_sort, config->obj_sort);
      if (config->save_hidden) {
	GList *hidden = diagram_tree_config_get_hidden_types(config);
	GList *ohidden = hidden;
	while (hidden) {
	  if (hidden->data && strlen(hidden->data))
	    diagram_tree_hide_explicit_type(diagtree_,
					    (const gchar *)hidden->data);
	  if (hidden->data) g_free(hidden->data);
	  hidden = hidden->next;
	}
	g_list_free(ohidden);
      }
    }
  tree = diagram_tree_widget(diagtree_);

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
  return diagtree_;
}

void
create_diagram_tree_window(DiagramTreeConfig *config, GtkWidget *menuitem)
{
  config_ = config;
  menu_item_ = GTK_CHECK_MENU_ITEM(menuitem);
  gtk_check_menu_item_set_active(menu_item_, config_->show_tree);
  if (!diagwindow_ && config_->show_tree) {
    diagwindow_ = diagram_tree_window_new(config_);
    gtk_widget_show(diagwindow_);
  }
}

/* menu callbacks */
void
diagtree_show_callback(gpointer data, guint action, GtkWidget *widget)
{
  if (!diagwindow_) {
    diagwindow_ = diagram_tree_window_new(config_);
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


static const gchar * const TYPE_DELIM_ = "^";

GList * /* destroyed by client */
diagram_tree_config_get_hidden_types(const DiagramTreeConfig *config)
{
  GList *result = NULL;
  g_return_val_if_fail(config, NULL);
  if (config->hidden == DIA_TREE_DEFAULT_HIDDEN)
    ((DiagramTreeConfig *)config)->hidden = g_strdup("");
  if (config->hidden && strlen(config->hidden)) {
    int k = 0;
    gchar **types = g_strsplit(config->hidden, TYPE_DELIM_, -1);
    while (types[k]) {
      result = g_list_prepend(result, (gpointer)types[k++]);
    }
  }
  return result;
}

void
diagram_tree_config_set_hidden_types(DiagramTreeConfig *config,
				     const GList *types)
{
  gchar *new_types = NULL;
  g_return_if_fail(config);
  if (types) {
    int k = 0;
    guint s = g_list_length((GList *)types);
    gchar **str_array = g_new(gchar *, s + 1);
    str_array[s] = NULL;
    while (types) {
      str_array[k++] = (gchar *)types->data;
      types = types->next;
    }
    new_types = g_strjoinv(TYPE_DELIM_, str_array);
    g_free(str_array);
  }
  if (config->hidden) g_free(config->hidden);
  config->hidden = new_types;
}

void
diagram_tree_config_add_hidden_type(DiagramTreeConfig *config,
				    const gchar *type)
{
  g_return_if_fail(config);
  if (type) {
    if (config->hidden) {
      gchar *new_types = g_strconcat(config->hidden, TYPE_DELIM_, type, NULL);
      g_free(config->hidden);
      config->hidden = new_types;
    } else {
      config->hidden = g_strdup(type);
    }
  }
}

void
diagram_tree_config_remove_hidden_type(DiagramTreeConfig *config,
				       const gchar *type)
{
  g_return_if_fail(config);
  if (type && config->hidden) {
    gchar *delim = g_strconcat(TYPE_DELIM_, type, NULL);
    gchar **parts = g_strsplit(config->hidden, delim, -1);
    gchar *new_types = g_strjoinv(NULL, parts);
    g_free(delim);
    g_strfreev(parts);
    g_free(config->hidden);
    config->hidden = new_types;
  }
}

    
