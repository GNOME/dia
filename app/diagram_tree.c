/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diagram_tree.c : a tree showing open diagrams
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

#include "prop_text.h"
#include "diagram_tree.h"

struct _DiagramTree {
  GtkCTree *tree;		/* the tree widget */
};


static void
update_object(GtkCTree *tree, GtkCTreeNode *node, Object *object);

/* signal handlers */
static void
select_tree_widget(GtkCTree *tree, GtkCTreeNode *node,
		   gint column, gpointer *data) 
{
  Diagram *d = NULL;
  GtkCTreeNode *dnode =
    (GTK_CTREE_ROW(node)->is_leaf)? GTK_CTREE_ROW(node)->parent : node;
  d = (Diagram *)gtk_ctree_node_get_row_data(tree, dnode);
  if (d) {
    GSList *dlist = d->displays;
    if (GTK_CTREE_ROW(node)->is_leaf) {
      Object *o = (Object *)gtk_ctree_node_get_row_data(tree, node);
      if (o) {
	update_object(tree, node, o);
	diagram_unselect_objects(d, d->data->selected);
	diagram_select(d, o);
      }
    }
    while (dlist) {
      DDisplay *dis = (DDisplay *)dlist->data;
      gdk_window_raise(dis->shell->window);
      gtk_widget_draw(dis->shell, NULL);
      dlist = g_slist_next(dlist);
    }
  }
}

/* private functions */
static GList*
get_diagram_objects(Diagram *diagram)
{
  GList *result = NULL;
  GPtrArray *layers = diagram->data->layers;
  if (layers) {
    int k = 0;
    Layer *lay;
    for (k = 0; k < layers->len; k++) {
      lay = (Layer *)g_ptr_array_index(layers, k);
      result = g_list_concat(result, g_list_copy(lay->objects));
    }
  }
  return result;
}

static void
create_object_pixmap(Object *object, GtkWidget *parent,
		     GdkPixmap **pixmap, GdkBitmap **mask)
{
  GtkStyle *style;

  g_assert(object);
  g_assert(pixmap);
  g_assert(mask);
  
  style = gtk_widget_get_style(parent);
  
  if (object->type->pixmap != NULL) {
    *pixmap =
      gdk_pixmap_colormap_create_from_xpm_d(NULL,
					    gtk_widget_get_colormap(parent),
					    mask, 
					    &style->bg[GTK_STATE_NORMAL],
					    object->type->pixmap);
  } else if (object->type->pixmap_file != NULL) {
    *pixmap =
      gdk_pixmap_colormap_create_from_xpm(NULL,
					  gtk_widget_get_colormap(parent),
					  mask,
					  &style->bg[GTK_STATE_NORMAL],
					  object->type->pixmap_file);
  } else {
    *pixmap = NULL;
    *mask = NULL;
  }
}

static gchar * 
get_object_name(Object *object)
{
  enum {SIZE = 31};
  static gchar BUFFER[SIZE];
  
  Property *prop = NULL;
  gchar *result = NULL;
  g_assert(object);
  prop = object_prop_by_name(object, "name");
  if (prop) result = ((StringProperty *)prop)->string_data;
  else {
    prop = object_prop_by_name(object, "text");
    if (prop) result = ((TextProperty *)prop)->text_data;
  }
  if (!result) result = object->type->name;

  g_snprintf(BUFFER, SIZE, " %s", result);
  (void)g_strdelimit(BUFFER, "\n", ' ');
  return BUFFER;
}

static void
update_object(GtkCTree *tree, GtkCTreeNode *node, Object *object)
{
  char *text = get_object_name(object);
  char *old = NULL;
  gtk_ctree_node_get_text(tree, node, 0, &old);
  if (!old || (text && strcmp(text, old))) {
    GdkPixmap *pixmap;
    GdkBitmap *mask;
    create_object_pixmap(object, GTK_WIDGET(tree), &pixmap, &mask);
    gtk_ctree_set_node_info(tree, node, text, 3, pixmap, mask,
			    pixmap, mask, TRUE, TRUE);
  }
}

static void
create_object_node(GtkCTree *tree, GtkCTreeNode *dnode, Object *obj)
{
  gboolean expanded = GTK_CTREE_ROW(dnode)->expanded;
  char *text[] = {NULL};
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkCTreeNode *node;
  text[0] = get_object_name(obj);
  create_object_pixmap(obj, GTK_WIDGET(tree), &pixmap, &mask);
  node =  gtk_ctree_insert_node(tree, dnode, NULL, text, 3,
				pixmap, mask, NULL, NULL, TRUE, FALSE);
  gtk_ctree_node_set_row_data(tree, node, (gpointer)obj);
  if (expanded) gtk_ctree_expand(tree, dnode);
}

static void
create_diagram_children(GtkCTree *tree, GtkCTreeNode *node,
			Diagram *diagram)
{
  GList *objects = get_diagram_objects(diagram);
  while (objects) { 
    create_object_node(tree, node, (Object *)objects->data);
    objects = g_list_next(objects);
  }
  g_list_free(objects);
}

static void
update_diagram_children(GtkCTree *tree, GtkCTreeNode *node,
			Diagram *diagram)
{
  GList *dobjects = get_diagram_objects(diagram);
  GList *org = dobjects;
  GtkCTreeNode *child = GTK_CTREE_ROW(node)->children;
  if (dobjects) {
    while (child) {
      GtkCTreeNode *current = child;
      child = GTK_CTREE_ROW(child)->sibling;
      if (!g_list_find(dobjects, gtk_ctree_node_get_row_data(tree, current)))
	gtk_ctree_remove_node(tree, current);
    }
  }
  while (dobjects) {
    if (!gtk_ctree_find_by_row_data(tree, node, dobjects->data))
      create_object_node(tree, node, (Object *)dobjects->data);
    dobjects = g_list_next(dobjects);
  }
  g_list_free(org);
}

/* external interface */
DiagramTree*
diagram_tree_new(GList *diagrams)
{
  DiagramTree *result = g_new(DiagramTree, 1);
  result->tree = GTK_CTREE(gtk_ctree_new(1, 0));
  gtk_signal_connect(GTK_OBJECT(result->tree), "tree-select-row",
		     GTK_SIGNAL_FUNC(select_tree_widget),
		     NULL);
  while (diagrams) {
    diagram_tree_add(result, (Diagram *)diagrams->data);
    diagrams = g_list_next(diagrams);
  }
  return result;
}

void
diagram_tree_delete(DiagramTree *tree) 
{
  g_return_if_fail(tree);
  gtk_widget_destroy(GTK_WIDGET(tree->tree));
}

void
diagram_tree_add(DiagramTree *tree, Diagram *diagram)
{
  g_return_if_fail(tree);
  if (diagram
      && !gtk_ctree_find_by_row_data(tree->tree, NULL, (gpointer)diagram))
    {
      char *text[] = {g_basename(diagram->filename)};
      GtkCTreeNode *node =
	gtk_ctree_insert_node(tree->tree, NULL, NULL, text, 1,
			      NULL, NULL, NULL, NULL, FALSE, FALSE);
      gtk_ctree_node_set_row_data(tree->tree, node, (gpointer)diagram);
      create_diagram_children(tree->tree, node, diagram);
    }
}

void
diagram_tree_remove(DiagramTree *tree, Diagram *diagram)
{
  GtkCTreeNode *node;
  g_return_if_fail(tree);
  if (diagram
      && (node = gtk_ctree_find_by_row_data(tree->tree, NULL,
					    (gpointer)diagram)))
    gtk_ctree_remove_node(tree->tree, node);
}

void
diagram_tree_update(DiagramTree *tree, Diagram *diagram)
{
  g_return_if_fail(tree);
  if (diagram->modified) {
    GtkCTreeNode *dnode =
      gtk_ctree_find_by_row_data(tree->tree, NULL, (gpointer)diagram);
    if (dnode) update_diagram_children(tree->tree, dnode, diagram);
    else diagram_tree_add(tree, diagram);
  }
}

void
diagram_tree_update_name(DiagramTree *tree, Diagram *diagram)
{
  GtkCTreeNode *node;
  g_return_if_fail(tree);
  g_return_if_fail(diagram);
  node = gtk_ctree_find_by_row_data(tree->tree, NULL, (gpointer)diagram);
  if (node)
    gtk_ctree_node_set_text(tree->tree, node, 0,
			    g_basename(diagram->filename));
}

void
diagram_tree_add_object(DiagramTree *tree, Diagram *diagram, Object *object)
{
  g_return_if_fail(tree);
  g_return_if_fail(diagram);
  if (object) {
    GtkCTreeNode *dnode =
      gtk_ctree_find_by_row_data(tree->tree, NULL, (gpointer)diagram);
    if (!dnode) diagram_tree_add(tree, diagram);
    else if (!gtk_ctree_find_by_row_data(tree->tree, dnode, (gpointer)object))
      create_object_node(tree->tree, dnode, object);
  }
}

void
diagram_tree_add_objects(DiagramTree *tree, Diagram *diagram, GList *objects)
{
  g_return_if_fail(tree);
  g_return_if_fail(diagram);
  while (objects) {
    diagram_tree_add_object(tree, diagram, (Object *)objects->data);
    objects = g_list_next(objects);
  }
}


void
diagram_tree_remove_object(DiagramTree *tree, Object *object)
{
  g_return_if_fail(tree);
  if (object) {
    GtkCTreeNode *node =
      gtk_ctree_find_by_row_data(tree->tree, NULL, (gpointer)object);
      if (node) gtk_ctree_remove_node(tree->tree, node);
  }
}

void
diagram_tree_remove_objects(DiagramTree *tree, GList *objects)
{
  g_return_if_fail(tree);
  while (objects) {
    diagram_tree_remove_object(tree, (Object *)objects->data);
    objects = g_list_next(objects);
  }
}

void
diagram_tree_update_object(DiagramTree *tree, Diagram *diagram,
			   Object *object)
{
  g_return_if_fail(tree);
  g_return_if_fail(diagram);
  if (object) {
    GtkCTreeNode *node =
      gtk_ctree_find_by_row_data(tree->tree, NULL,(gpointer)object);
    if (node) {
      update_object(tree->tree, node, object);
    }
  }
}

GtkWidget*
diagram_tree_widget(const DiagramTree *tree)
{
  g_return_val_if_fail(tree, NULL);
  return GTK_WIDGET(tree->tree);
}

