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

#include "properties.h"
#include "prop_text.h"
#include "diagram_tree_menu.h"
#include "diagram_tree_menu_callbacks.h"
#include "diagram_tree.h"

struct _DiagramTree {
  GtkCTree *tree;		/* the tree widget */
  GtkCTreeNode *last;		/* last clicked node */
  GtkMenu *dia_menu;		/* diagram popup menu */
  GtkMenu *object_menu;		/* object popup menu */
  GtkCListCompareFunc dia_cmp;	/* diagram ordering function */
  GtkCListCompareFunc obj_cmp;	/* object ordering function */
};


static void
update_object(DiagramTree *tree, GtkCTreeNode *node, Object *object);

static void
raise_display(Diagram *d)
{
  if (d) {
    GSList *dlist = d->displays;
    while (dlist) {
      DDisplay *dis = (DDisplay *)dlist->data;
      gdk_window_raise(dis->shell->window);
      gtk_widget_draw(dis->shell, NULL);
      dlist = g_slist_next(dlist);
    }
  }
}

static void
select_and_raise(DiagramTree *tree, GtkCTreeNode *node)
{
  Diagram *d = NULL;
  GtkCTreeNode *dnode =
    (GTK_CTREE_ROW(node)->is_leaf)? GTK_CTREE_ROW(node)->parent : node;
  d = (Diagram *)gtk_ctree_node_get_row_data(tree->tree, dnode);
  if (d) {
    if (GTK_CTREE_ROW(node)->is_leaf) {
      Object *o = (Object *)gtk_ctree_node_get_row_data(tree->tree, node);
      if (o) {
	update_object(tree, node, o);
	diagram_unselect_objects(d, d->data->selected);
	diagram_select(d, o);
      }
    }
    gtk_ctree_select(tree->tree, node);
    raise_display(d);
  }
}


/* signal handlers */
static void
select_tree_widget(GtkCTree *tree, GtkCTreeNode *node,
		   gint column, DiagramTree *data) 
{
  if (GTK_CTREE_ROW(node)->is_leaf) {
    Object *o = (Object *)gtk_ctree_node_get_row_data(tree, node);
    if (o) update_object(data, node, o);
  }
}

static gint
button_press_callback(GtkCTree *tree, GdkEventButton *event,
		      DiagramTree *dtree)
{
  gint row = -1;
  gint column = -1;
    
  gtk_clist_get_selection_info(GTK_CLIST(tree), event->x, event->y,
			       &row, &column);
  if (row != -1) dtree->last = gtk_ctree_node_nth(tree, row);
  else dtree->last = NULL;

  if (dtree->last && event->type == GDK_2BUTTON_PRESS) {
      select_and_raise(dtree, dtree->last);
  } else if (dtree->last && event->type == GDK_BUTTON_PRESS
	     && event->button == 3) {
    GtkMenu *menu = (GTK_CTREE_ROW(dtree->last)->is_leaf)?
      dtree->object_menu : dtree->dia_menu;
    gtk_menu_popup(menu, NULL, NULL, NULL, NULL, 3, event->time);
  }
  return TRUE;
}

/* private functions */
static void
sort_objects(DiagramTree *tree, GtkCTreeNode *node)
{
  if (tree->obj_cmp) {
    GtkCTreeNode *dnode =
      (GTK_CTREE_ROW(node)->is_leaf)? GTK_CTREE_ROW(node)->parent : node;
    gtk_clist_set_compare_func(GTK_CLIST(tree->tree), tree->obj_cmp);
    gtk_ctree_sort_node(tree->tree, dnode);
  }
}

static void
sort_diagrams(DiagramTree *tree)
{
  if (tree->dia_cmp) {
    gtk_clist_set_compare_func(GTK_CLIST(tree->tree), tree->dia_cmp);
    gtk_ctree_sort_node(tree->tree, NULL);
  }
}

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
update_object(DiagramTree *tree, GtkCTreeNode *node, Object *object)
{
  char *text = get_object_name(object);
  char *old = NULL;
  gtk_ctree_node_get_text(tree->tree, node, 0, &old);
  if (!old || (text && strcmp(text, old))) {
    GdkPixmap *pixmap;
    GdkBitmap *mask;
    create_object_pixmap(object, GTK_WIDGET(tree->tree), &pixmap, &mask);
    gtk_ctree_set_node_info(tree->tree, node, text, 3, pixmap, mask,
			    pixmap, mask, TRUE, TRUE);
    sort_objects(tree, node);
  }
}

static void
create_object_node(DiagramTree *tree, GtkCTreeNode *dnode, Object *obj)
{
  gboolean expanded = GTK_CTREE_ROW(dnode)->expanded;
  char *text[] = {NULL};
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkCTreeNode *node;
  text[0] = get_object_name(obj);
  create_object_pixmap(obj, GTK_WIDGET(tree->tree), &pixmap, &mask);
  node =  gtk_ctree_insert_node(tree->tree, dnode, NULL, text, 3,
				pixmap, mask, NULL, NULL, TRUE, FALSE);
  gtk_ctree_node_set_row_data(tree->tree, node, (gpointer)obj);
  if (expanded) gtk_ctree_expand(tree->tree, dnode);
  sort_objects(tree, dnode);
}

static void
create_diagram_children(DiagramTree *tree, GtkCTreeNode *node,
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
update_diagram_children(DiagramTree *tree, GtkCTreeNode *node,
			Diagram *diagram)
{
  GList *dobjects = get_diagram_objects(diagram);
  GList *org = dobjects;
  GtkCTreeNode *child = GTK_CTREE_ROW(node)->children;
  if (dobjects) {
    while (child) {
      GtkCTreeNode *current = child;
      child = GTK_CTREE_ROW(child)->sibling;
      if (!g_list_find(dobjects, gtk_ctree_node_get_row_data(tree->tree,
							     current)))
	gtk_ctree_remove_node(tree->tree, current);
    }
  }
  while (dobjects) {
    if (!gtk_ctree_find_by_row_data(tree->tree, node, dobjects->data))
      create_object_node(tree, node, (Object *)dobjects->data);
    dobjects = g_list_next(dobjects);
  }
  g_list_free(org);
  sort_objects(tree, node);
}

/* external interface */
DiagramTree*
diagram_tree_new(GList *diagrams)
{
  DiagramTree *result = g_new(DiagramTree, 1);
  result->tree = GTK_CTREE(gtk_ctree_new(1, 0));
  result->last = NULL;
  result->dia_cmp = result->obj_cmp = NULL;
  gtk_signal_connect(GTK_OBJECT(result->tree), "tree-select-row",
		     GTK_SIGNAL_FUNC(select_tree_widget),
		     (gpointer)result);
  gtk_signal_connect(GTK_OBJECT(result->tree),
		     "button_press_event",
		     GTK_SIGNAL_FUNC(button_press_callback),
		     (gpointer)result);
  result->dia_menu = result->object_menu = NULL;
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
  if (tree) {
    if (diagram
	&& !gtk_ctree_find_by_row_data(tree->tree, NULL, (gpointer)diagram))
      {
	char *text[] = {g_basename(diagram->filename)};
	GtkCTreeNode *node =
	  gtk_ctree_insert_node(tree->tree, NULL, NULL, text, 1,
				NULL, NULL, NULL, NULL, FALSE, FALSE);
	gtk_ctree_node_set_row_data(tree->tree, node, (gpointer)diagram);
	create_diagram_children(tree, node, diagram);
	sort_diagrams(tree);
      }
  }
}

void
diagram_tree_remove(DiagramTree *tree, Diagram *diagram)
{
  if (tree) {
    GtkCTreeNode *node;
    if (diagram
	&& (node = gtk_ctree_find_by_row_data(tree->tree, NULL,
					      (gpointer)diagram)))
      gtk_ctree_remove_node(tree->tree, node);
  }
}

void
diagram_tree_update(DiagramTree *tree, Diagram *diagram)
{
  if (tree) {
    if (diagram->modified) {
      GtkCTreeNode *dnode =
	gtk_ctree_find_by_row_data(tree->tree, NULL, (gpointer)diagram);
      if (dnode) update_diagram_children(tree, dnode, diagram);
      else diagram_tree_add(tree, diagram);
    }
  }
}

void
diagram_tree_update_name(DiagramTree *tree, Diagram *diagram)
{
  if (tree) {
    GtkCTreeNode *node;
    g_return_if_fail(diagram);
    node = gtk_ctree_find_by_row_data(tree->tree, NULL, (gpointer)diagram);
    if (node) {
      gtk_ctree_node_set_text(tree->tree, node, 0,
			      g_basename(diagram->filename));
      sort_diagrams(tree);
    }
  }
}

void
diagram_tree_add_object(DiagramTree *tree, Diagram *diagram, Object *object)
{
  if (tree) {
    g_return_if_fail(diagram);
    if (object) {
      GtkCTreeNode *dnode =
	gtk_ctree_find_by_row_data(tree->tree, NULL, (gpointer)diagram);
      if (!dnode) diagram_tree_add(tree, diagram);
      else if (!gtk_ctree_find_by_row_data(tree->tree, dnode, (gpointer)object))
	create_object_node(tree, dnode, object);
    }
  }
}

void
diagram_tree_add_objects(DiagramTree *tree, Diagram *diagram, GList *objects)
{
  if (tree) {
    g_return_if_fail(diagram);
    while (objects) {
      diagram_tree_add_object(tree, diagram, (Object *)objects->data);
      objects = g_list_next(objects);
    }
  }
}


void
diagram_tree_remove_object(DiagramTree *tree, Object *object)
{
  if (tree) {
    if (object) {
      GtkCTreeNode *node =
	gtk_ctree_find_by_row_data(tree->tree, NULL, (gpointer)object);
      if (node) gtk_ctree_remove_node(tree->tree, node);
    }
  }
}

void
diagram_tree_remove_objects(DiagramTree *tree, GList *objects)
{
  if (tree) {
    while (objects) {
      diagram_tree_remove_object(tree, (Object *)objects->data);
      objects = g_list_next(objects);
    }
  }
}

void
diagram_tree_update_object(DiagramTree *tree, Diagram *diagram,
			   Object *object)
{
  if (tree) {
    g_return_if_fail(diagram);
    if (object) {
      GtkCTreeNode *node =
	gtk_ctree_find_by_row_data(tree->tree, NULL,(gpointer)object);
      if (node) {
	update_object(tree, node, object);
      }
    }
  }
}

void
diagram_tree_raise(DiagramTree *tree)
{
  if (tree && tree->last) {
    select_and_raise(tree, tree->last);
  }
}

void
diagram_tree_show_properties(const DiagramTree *tree)
{
  if (tree && tree->last && GTK_CTREE_ROW(tree->last)->is_leaf) {
    GtkCTreeNode *parent = GTK_CTREE_ROW(tree->last)->parent;
    if (parent) {
      Diagram *dia = (Diagram *)gtk_ctree_node_get_row_data(tree->tree, parent);
      Object *obj =
	(Object *)gtk_ctree_node_get_row_data(tree->tree, tree->last);
      properties_show(dia, obj);
    }
  }
}

/* sorting functions */
static gint
cmp_name_(GtkCList *tree, GtkCListRow *lhm, GtkCListRow *rhm)
{
  int k;
  gchar *name1 = lhm->cell->u.text; 
  gchar *name2 = rhm->cell->u.text; 
  if (name1 && !name2) k = -1;
  else if (!name1 && name2) k = 1;
  else if (!name1 && !name2) k = 0;
  else k = strcmp(name1, name2);
  if (k > 0) return 1;
  if (k < 0) return -1;
  return 0;
}

static gint
cmp_type_(GtkCList *tree, GtkCListRow *lhm, GtkCListRow *rhm)
{
  int k;
  Object *o1 = (Object *)lhm->data;
  Object *o2 = (Object *)rhm->data;
  k = strcmp(o1->type->name, o2->type->name);
  if (k > 0) return 1;
  if (k < 0) return -1;
  return 0;
}

static GtkCListCompareFunc cmp_funcs_[] = {
  (GtkCListCompareFunc)cmp_name_,
  (GtkCListCompareFunc)cmp_type_,
  NULL
};

void
diagram_tree_sort_objects(DiagramTree *tree, DiagramTreeSortType type)
{
  if (tree && type <= DIA_TREE_SORT_INSERT && tree->last) {
    GtkCTreeNode *node = GTK_CTREE_ROW(tree->last)->is_leaf?
      GTK_CTREE_ROW(tree->last)->parent : tree->last;
    gtk_clist_set_compare_func(GTK_CLIST(tree->tree), cmp_funcs_[type]);
    gtk_ctree_sort_node(tree->tree, node);
  }
}

void
diagram_tree_sort_all_objects(DiagramTree *tree, DiagramTreeSortType type)
{
  /* FIXME: should not depend on tree->last != NULL */
  if (tree && type <= DIA_TREE_SORT_INSERT && tree->last) {
    GtkCTreeNode *node = GTK_CTREE_ROW(tree->last)->is_leaf?
      GTK_CTREE_ROW(tree->last)->parent : tree->last;
    while (GTK_CTREE_NODE_PREV(node)) node = GTK_CTREE_NODE_PREV(node);
    while (node) {
      gtk_clist_set_compare_func(GTK_CLIST(tree->tree), cmp_funcs_[type]);
      gtk_ctree_sort_node(tree->tree, node);
      node = GTK_CTREE_ROW(node)->sibling;
    }
  }
}

void
diagram_tree_sort_diagrams(DiagramTree *tree, DiagramTreeSortType type)
{
  if (tree && type <= DIA_TREE_SORT_INSERT && type != DIA_TREE_SORT_TYPE) {
    gtk_clist_set_compare_func(GTK_CLIST(tree->tree), cmp_funcs_[type]);
    gtk_ctree_sort_node(tree->tree, NULL);
  }
}

void
diagram_tree_set_object_sort_type(DiagramTree *tree, DiagramTreeSortType type)
{
  if (tree && type <= DIA_TREE_SORT_INSERT) {
    tree->obj_cmp = cmp_funcs_[type];
    diagram_tree_sort_all_objects(tree, type);
  }
}

void
diagram_tree_set_diagram_sort_type(DiagramTree *tree, DiagramTreeSortType type)
{
  if (tree && type <= DIA_TREE_SORT_INSERT && type != DIA_TREE_SORT_TYPE) {
    tree->dia_cmp = cmp_funcs_[type];
    diagram_tree_sort_diagrams(tree, type);
  }
}

static DiagramTreeSortType
sort_type_lookup(GtkCListCompareFunc func)
{
  int k;
  for (k = 0; k <= DIA_TREE_SORT_INSERT; ++k) {
    if (cmp_funcs_[k] == func) return k;
  }
  g_assert_not_reached();
  return 0;
}

DiagramTreeSortType
diagram_tree_diagram_sort_type(const DiagramTree *tree)
{
  if (tree) {
    return sort_type_lookup(tree->dia_cmp);
  } else
    return DIA_TREE_SORT_INSERT;
}
  

DiagramTreeSortType
diagram_tree_object_sort_type(const DiagramTree *tree)
{
  if (tree) {
    return sort_type_lookup(tree->obj_cmp);
  } else
    return DIA_TREE_SORT_INSERT;
}

void
diagram_tree_attach_dia_menu(DiagramTree *tree, GtkWidget *menu)
{
  if (tree) {
    tree->dia_menu = GTK_MENU(menu);
  }
}

void
diagram_tree_attach_obj_menu(DiagramTree *tree, GtkWidget *menu)
{
  if (tree) {
    tree->object_menu = GTK_MENU(menu);
  }
}

GtkWidget*
diagram_tree_widget(const DiagramTree *tree)
{
  g_return_val_if_fail(tree, NULL);
  return GTK_WIDGET(tree->tree);
}
