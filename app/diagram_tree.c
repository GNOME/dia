/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diagram_tree.c : a tree showing open diagrams
 * Copyright (C) 2001 Jose A Ortega Ruiz
 *
 * patch to center objects in drawing viewport when doing "Locate"
 * Copyright (C) 2003 Andrew Halper
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

#include <string.h>

#undef GTK_DISABLE_DEPRECATED /* GtkCTree */

#include "properties-dialog.h"
#include "diagram_tree_menu.h"
#include "diagram_tree_menu_callbacks.h"
#include "diagram_tree.h"
#include "persistence.h"
#include "dia-application.h"

/* delete a tree (the widget is destroyed, but not the diagrams */
static void diagram_tree_delete(GtkCTree *ctree, DiagramTree *tree);

struct _DiagramTree {
  GtkCTree *tree;		/* the tree widget */
  GtkCTreeNode *last;		/* last clicked node */
  DiagramTreeMenus *menus;	/* popup menus */
  GtkCListCompareFunc dia_cmp;	/* diagram ordering function */
  GtkCListCompareFunc obj_cmp;	/* object ordering function */
};

#define is_object_node(node) (GTK_CTREE_ROW(node)->is_leaf)

static gint
find_hidden_type(gconstpointer type, gconstpointer object_type)
{
    return !type
      || !object_type
      || strcmp((const gchar *)object_type, (const gchar *)type);
}

#define is_hidden_type(dtree, type) \
  g_list_find_custom(persistent_list_get_glist(HIDDEN_TYPES_NAME), \
                     (gpointer)type, find_hidden_type)

#define is_hidden_object(dtree, object)		\
  is_hidden_type(dtree, ((DiaObject *)object)->type->name)

static void
update_object(DiagramTree *tree, GtkCTreeNode *node, DiaObject *object);
static void
diagram_tree_add_object(DiagramTree *tree, Diagram *diagram, DiaObject *object);
static void
diagram_tree_remove_object(DiagramTree *tree, DiaObject *object);
static void
diagram_tree_remove_objects(DiagramTree *tree, GList *objects);
static void
diagram_tree_update_name(DiagramTree *tree, Diagram *diagram);
static void
diagram_tree_update_object(DiagramTree *tree, Diagram *diagram, DiaObject *object);

static void
select_node(DiagramTree *tree, GtkCTreeNode *node, gboolean raise)
{
  Diagram *d = NULL;
  GtkCTreeNode *dnode = (is_object_node(node)) ?
    GTK_CTREE_ROW(node)->parent : node;
  DiaObject *o = (DiaObject *)gtk_ctree_node_get_row_data(tree->tree, node);

 
  d = (Diagram *)gtk_ctree_node_get_row_data(tree->tree, dnode);
  if (d) {
    GSList *dlist = d->displays;
    if (is_object_node(node)) {
      if (o) {
	update_object(tree, node, o);
	diagram_remove_all_selected(d, FALSE);
	diagram_select(d, o);
      }
    }
    while (dlist) {
      DDisplay *ddisp = (DDisplay *)dlist->data;
      if (raise) {
	gdk_window_raise(ddisp->shell->window);
	/* if object exists */
	if (o) {
	  ddisplay_scroll_to_object(ddisp, o);
	}
      }
      gtk_widget_draw(ddisp->shell, NULL);
      dlist = g_slist_next(dlist);
    }
  }
}

static void
update_last_node(DiagramTree *tree) 
{
  if (is_object_node(tree->last)) {
    DiaObject *o = (DiaObject *)gtk_ctree_node_get_row_data(tree->tree, tree->last);
    if (o) update_object(tree, tree->last, o);
  }
}

/* signal handlers */
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

  if (dtree->last) update_last_node(dtree);

  /* if doubleclick */
  if (dtree->last && event->type == GDK_2BUTTON_PRESS) {
    /* equivalent of "Locate" */
    select_node(dtree, dtree->last, TRUE);
  } else if (dtree->last && event->type == GDK_BUTTON_PRESS) {
    if (event->button == 3) {
      DiagramTreeMenuType menu = (is_object_node(dtree->last))?
	DIA_MENU_OBJECT : DIA_MENU_DIAGRAM;
      diagram_tree_menus_popup_menu(dtree->menus, menu, event->time);
    } else if (event->button == 1) {
      select_node(dtree, dtree->last, FALSE);
      /* need to return FALSE to let gtk process it further */
      return FALSE;
    }
  }
  return TRUE;
}

/* private functions */
static void
sort_objects(DiagramTree *tree, GtkCTreeNode *node)
{
  if (tree->obj_cmp) {
    GtkCTreeNode *dnode =
      (is_object_node(node))? GTK_CTREE_ROW(node)->parent : node;
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
create_object_pixmap(DiaObject *object, GtkWidget *parent,
		     GdkPixmap **pixmap, GdkBitmap **mask)
{
  GtkStyle *style;

  g_assert(object);
  g_assert(pixmap);
  g_assert(mask);
  
  style = gtk_widget_get_style(parent);
  
  if (object->type->pixmap != NULL) {
    if (strncmp((char *)object->type->pixmap, "GdkP", 4) == 0) {
      GdkPixbuf *p;
      p = gdk_pixbuf_new_from_inline(-1, (guint8*)object->type->pixmap, TRUE, NULL);
      gdk_pixbuf_render_pixmap_and_mask_for_colormap(p, gtk_widget_get_colormap(parent), pixmap, mask, 128);
      g_object_unref (p);
    } else {
      *pixmap =
	gdk_pixmap_colormap_create_from_xpm_d(NULL,
					      gtk_widget_get_colormap(parent),
					      mask, 
					      &style->bg[GTK_STATE_NORMAL],
					      object->type->pixmap);
    }
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
get_object_name(DiaObject *object)
{
  enum {SIZE = 31};
  static gchar BUFFER[SIZE];

  gchar *name = object_get_displayname (object);
  g_snprintf(BUFFER, SIZE, " %s", name);
  g_free(name);

  return BUFFER;
}

static void
update_object(DiagramTree *tree, GtkCTreeNode *node, DiaObject *object)
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

typedef struct _PixmapsAndMasks
{
  GdkPixmap *pixmap;
  GdkBitmap *mask;
} PixmapsAndMasks;

static GHashTable *_pixmaps_and_masks = NULL;

static void
create_object_node(DiagramTree *tree, GtkCTreeNode *dnode, DiaObject *obj)
{
  gboolean expanded = GTK_CTREE_ROW(dnode)->expanded;
  char *text[] = {NULL};
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkCTreeNode *node;
  PixmapsAndMasks *pnm;

  text[0] = get_object_name(obj);

  if (!_pixmaps_and_masks)
    _pixmaps_and_masks = g_hash_table_new_full(
			    g_str_hash, g_str_equal, NULL, g_free);

  pnm = g_hash_table_lookup (_pixmaps_and_masks, obj->type->name);
  if (!pnm) {
    pnm = g_new0 (PixmapsAndMasks, 1);
    create_object_pixmap(obj, GTK_WIDGET(tree->tree), &pnm->pixmap, &pnm->mask);
    g_hash_table_insert (_pixmaps_and_masks, obj->type->name, pnm);
  }
  node =  gtk_ctree_insert_node(tree->tree, dnode, NULL, text, 3,
				pnm->pixmap ? g_object_ref (pnm->pixmap) : NULL, 
				pnm->mask ? g_object_ref (pnm->mask) : NULL, 
				NULL, NULL, TRUE, FALSE);
  gtk_ctree_node_set_row_data(tree->tree, node, (gpointer)obj);
  if (expanded) gtk_ctree_expand(tree->tree, dnode);
  sort_objects(tree, dnode);
}


static void
create_diagram_children(DiagramTree *tree, GtkCTreeNode *node,
			Diagram *diagram)
{
  GList *objects = get_diagram_objects(diagram);
  GList *org = objects;
  while (objects) {
    if (!is_hidden_object(tree, objects->data)) {
      create_object_node(tree, node, (DiaObject *)objects->data);
    }
    objects = g_list_next(objects);
  }
  g_list_free(org);
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
      gpointer obj = gtk_ctree_node_get_row_data(tree->tree, current);
      child = GTK_CTREE_ROW(child)->sibling;
      if (!g_list_find(dobjects, obj) || is_hidden_object(tree, obj))
	gtk_ctree_remove_node(tree->tree, current);
    }
  }
  while (dobjects) {
    if (!is_hidden_object(tree, dobjects->data)
	&& !gtk_ctree_find_by_row_data(tree->tree, node, dobjects->data))
      create_object_node(tree, node, (DiaObject *)dobjects->data);
    dobjects = g_list_next(dobjects);
  }
  g_list_free(org);
  sort_objects(tree, node);
}

/* listen to diagram creation */
static void
_diagram_add (DiaApplication *app,
              Diagram        *dia,
	      DiagramTree    *tree)
{
  diagram_tree_add(tree, dia);
}
static void
_diagram_change (DiaApplication *app,
                 Diagram        *dia,
		 guint           flags,
		 gpointer        object,
	         DiagramTree    *tree)
{
  if (flags & DIAGRAM_CHANGE_NAME)
    diagram_tree_update_name (tree, dia);
  if (flags & DIAGRAM_CHANGE_OBJECT)
    diagram_tree_update_object (tree, dia, object);
}
static void
_diagram_remove (DiaApplication *app,
                 Diagram        *dia,
	         DiagramTree    *tree)
{
  diagram_tree_remove(tree, dia);
}
/* external interface */
DiagramTree*
diagram_tree_new(GList *diagrams, GtkWindow *window,
		 DiagramTreeSortType dia_sort,
		 DiagramTreeSortType obj_sort)
{
  GList *tmplist;
  DiagramTree *result = g_new(DiagramTree, 1);
  result->tree = GTK_CTREE(gtk_ctree_new(1, 0));
  result->last = NULL;
  result->dia_cmp = result->obj_cmp = NULL;

  g_signal_connect(GTK_OBJECT(result->tree),
		     "button_press_event",
		   G_CALLBACK(button_press_callback),
		     (gpointer)result);
  g_signal_connect (GTK_OBJECT (result->tree), "destroy",
		    G_CALLBACK (diagram_tree_delete),
		    result);
		     
  g_signal_connect (G_OBJECT (dia_application_get ()),
                    "diagram_add",
		    G_CALLBACK (_diagram_add),
		    result);
  g_signal_connect (G_OBJECT (dia_application_get ()),
                    "diagram_change",
		    G_CALLBACK (_diagram_change),
		    result);
  g_signal_connect (G_OBJECT(dia_application_get ()),
                    "diagram_remove",
		    G_CALLBACK (_diagram_remove),
		    result);

  while (diagrams) {
    diagram_tree_add(result, (Diagram *)diagrams->data);
    diagrams = g_list_next(diagrams);
  }
  diagram_tree_set_diagram_sort_type(result, dia_sort);
  diagram_tree_set_object_sort_type(result, obj_sort);
  result->menus = diagram_tree_menus_new(result, window);
  /* Set up menu items for the list of hidden types */
  tmplist = persistent_list_get_glist(HIDDEN_TYPES_NAME);
  for (; tmplist != NULL; tmplist = g_list_next(tmplist)) {
    diagram_tree_menus_add_hidden_type(result->menus, tmplist->data);
  }
  return result;
}

static void
diagram_tree_delete(GtkCTree *ctree, DiagramTree *tree) 
{
  g_return_if_fail(tree);

  g_signal_handlers_disconnect_by_func (G_OBJECT (dia_application_get ()), _diagram_add,    tree);
  g_signal_handlers_disconnect_by_func (G_OBJECT (dia_application_get ()), _diagram_change, tree);
  g_signal_handlers_disconnect_by_func (G_OBJECT (dia_application_get ()), _diagram_remove, tree);

  gtk_widget_destroy (ctree);
}

static void
_object_add (DiagramData *dia,
	     Layer       *layer,
             DiaObject   *obj,
	     DiagramTree *tree)
{
  g_return_if_fail (DIA_DIAGRAM(dia) != NULL);
  
  diagram_tree_add_object (tree, DIA_DIAGRAM(dia), obj);
}
static void
_object_remove(DiagramData *dia,
	       Layer       *layer,
               DiaObject   *obj,
	       DiagramTree *tree)
{
  g_return_if_fail (DIA_DIAGRAM(dia) != NULL);
  
  diagram_tree_remove_object (tree, obj);
}

void
diagram_tree_add(DiagramTree *tree, Diagram *diagram)
{
    if (tree) {
        if (diagram
            && !gtk_ctree_find_by_row_data(tree->tree, NULL, (gpointer)diagram))
        {
            char *text[] = {(char *)g_basename(diagram->filename)};
            GtkCTreeNode *node =
                gtk_ctree_insert_node(tree->tree, NULL, NULL, text, 1,
                                      NULL, NULL, NULL, NULL, FALSE, FALSE);
            gtk_ctree_node_set_row_data(tree->tree, node, (gpointer)diagram);
            create_diagram_children(tree, node, diagram);
            sort_diagrams(tree);
	    
	    g_signal_connect (G_OBJECT(diagram), "object_add", G_CALLBACK(_object_add), tree);
	    g_signal_connect (G_OBJECT(diagram), "object_remove", G_CALLBACK(_object_remove), tree);
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

      g_signal_handlers_disconnect_by_func (diagram, _object_add, tree);
      g_signal_handlers_disconnect_by_func (diagram, _object_remove, tree);
  }
}

void
diagram_tree_update(DiagramTree *tree, Diagram *diagram)
{
  if (tree) {
    if (diagram_is_modified(diagram)) {
      GtkCTreeNode *dnode =
	gtk_ctree_find_by_row_data(tree->tree, NULL, (gpointer)diagram);
      if (dnode) update_diagram_children(tree, dnode, diagram);
      else diagram_tree_add(tree, diagram);
    }
  }
}

void
diagram_tree_update_all(DiagramTree *tree)
{
  if (tree) {
    GtkCTreeNode *node = gtk_ctree_node_nth(tree->tree, 0);
    while (node) {
      Diagram *d = (Diagram *)gtk_ctree_node_get_row_data(tree->tree, node);
      if (d) update_diagram_children(tree, node, d);
      node = GTK_CTREE_ROW(node)->sibling;
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

static void
diagram_tree_add_object(DiagramTree *tree, Diagram *diagram, DiaObject *object)
{
  if (tree) {
    g_return_if_fail(diagram);
    if (object && !is_hidden_object(tree, object)) {
      GtkCTreeNode *dnode =
	gtk_ctree_find_by_row_data(tree->tree, NULL, (gpointer)diagram);
      if (!dnode) diagram_tree_add(tree, diagram);
      else if (!gtk_ctree_find_by_row_data(tree->tree, dnode, (gpointer)object))
	create_object_node(tree, dnode, object);
    }
  }
}

static void
diagram_tree_add_objects(DiagramTree *tree, Diagram *diagram, GList *objects)
{
  if (tree) {
    g_return_if_fail(diagram);
    while (objects) {
      diagram_tree_add_object(tree, diagram, (DiaObject *)objects->data);
      objects = g_list_next(objects);
    }
  }
}


void
diagram_tree_remove_object(DiagramTree *tree, DiaObject *object)
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
      diagram_tree_remove_object(tree, (DiaObject *)objects->data);
      objects = g_list_next(objects);
    }
  }
}

void
diagram_tree_update_object(DiagramTree *tree, Diagram *diagram,
			   DiaObject *object)
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
    select_node(tree, tree->last, TRUE);
  }
}

void
diagram_tree_show_properties(const DiagramTree *tree)
{
  if (tree && tree->last && is_object_node(tree->last)) {
    GtkCTreeNode *parent = GTK_CTREE_ROW(tree->last)->parent;
    if (parent) {
      Diagram *dia = (Diagram *)gtk_ctree_node_get_row_data(tree->tree, parent);
      DiaObject *obj =
	(DiaObject *)gtk_ctree_node_get_row_data(tree->tree, tree->last);
      object_properties_show(dia, obj);
    }
  }
}

const gchar *
diagram_tree_hide_type(DiagramTree *tree)
{
  if (tree && tree->last && is_object_node(tree->last)) {
    DiaObject *obj =
      (DiaObject *)gtk_ctree_node_get_row_data(tree->tree, tree->last);
    g_assert(!is_hidden_object(tree, obj));
    diagram_tree_hide_explicit_type(tree, obj->type->name);
    return obj->type->name;
  }
  return NULL;
}

void
diagram_tree_hide_explicit_type(DiagramTree *tree, const gchar *type)
{
  if (tree && type) {
    persistent_list_add(HIDDEN_TYPES_NAME, type);
    diagram_tree_menus_add_hidden_type(tree->menus, type);
    diagram_tree_update_all(tree);
  }
}

void
diagram_tree_unhide_type(DiagramTree *tree, const gchar *type)
{
  if (tree && type) {
    GList *t = is_hidden_type(tree, type);
    if (t) {
      persistent_list_remove(HIDDEN_TYPES_NAME, type);
      diagram_tree_update_all(tree);
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
  DiaObject *o1 = (DiaObject *)lhm->data;
  DiaObject *o2 = (DiaObject *)rhm->data;
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
    GtkCTreeNode *node = is_object_node(tree->last)?
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
    GtkCTreeNode *node = is_object_node(tree->last)?
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

GtkWidget*
diagram_tree_widget(const DiagramTree *tree)
{
  g_return_val_if_fail(tree, NULL);
  return GTK_WIDGET(tree->tree);
}
