/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diagram_tree.c : a tree showing open diagrams
 * Copyright (C) 2001 Jose A Ortega Ruiz
 *
 * complete rewrite to get rid of deprecated widgets
 * Copyright (C) 2009 Hans Breuer
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
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>
#include "diagram.h"
#include "object.h"
#include "dia-application.h"
#include "dia-layer.h"
#include "diagram_tree_model.h"

/* accessing iter fields by name */
#if defined(_MSC_VER) && !defined(__clang__)
/* GCC does not like the pattern */
/* error: lvalue required as left operand of assignment */
#define NODE_DIAGRAM(it) ((DiagramData*)(it->user_data))
#define NODE_LAYER(it) ((Layer*)(it->user_data2))
#define NODE_OBJECT(it) ((DiaObject*)(it->user_data3))
#else
#define NODE_DIAGRAM(it) (it->user_data)
#define NODE_LAYER(it) (it->user_data2)
#define NODE_OBJECT(it) (it->user_data3)
#endif
typedef struct _DiagramTreeModelClass
{
  GObjectClass parent_class;
} DiagramTreeModelClass;
typedef struct _DiagramTreeModel
{
  GObject parent;
  /* no need to store anything */
} DiagramTreeModel;

static GType _dtm_get_type (void);
static void _dtm_finalize (GObject *object);

static void
_dtm_class_init (DiagramTreeModelClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = _dtm_finalize;
}

#define DIA_TYPE_DIAGRAM_TREE_MODEL (_dtm_get_type ())
#define DIAGRAM_TREE_MODEL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_DIAGRAM_TREE_MODEL, DiagramTreeModel))
static void _dtm_iface_init (GtkTreeModelIface *iface);

G_DEFINE_TYPE_WITH_CODE (DiagramTreeModel, _dtm, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
						_dtm_iface_init))

static GtkTreeModelFlags
_dtm_get_flags (GtkTreeModel *tree_model)
{
  return GTK_TREE_MODEL_ITERS_PERSIST; /* NOT: ; */
}


static  int
_dtm_get_n_columns (GtkTreeModel *tree_model)
{
  return NUM_COLUMNS;
}


static GType
_dtm_get_column_type (GtkTreeModel *tree_model,
                      int           index)
{
  g_return_val_if_fail (index >= DIAGRAM_COLUMN || index < NUM_COLUMNS, G_TYPE_NONE);

  switch (index) {
  case DIAGRAM_COLUMN :
    return DIA_TYPE_DIAGRAM;
  case NAME_COLUMN :
    return G_TYPE_STRING;
  case LAYER_COLUMN :
  case OBJECT_COLUMN :
    /* TODO: these should be ported to GObject ... */
    return G_TYPE_POINTER;
  default :
    g_assert_not_reached ();
    return G_TYPE_NONE;
  }
}


static gboolean
_dtm_get_iter (GtkTreeModel *tree_model,
               GtkTreeIter  *iter,
               GtkTreePath  *path)
{
  /* copy&paste from Gtk */
  GtkTreeIter parent;
  int *indices;
  int depth, i;

  indices = gtk_tree_path_get_indices (path);
  depth = gtk_tree_path_get_depth (path);

  g_return_val_if_fail (depth > 0, FALSE);

  if (!gtk_tree_model_iter_nth_child (tree_model, iter, NULL, indices[0]))
    return FALSE;

  for (i = 1; i < depth; i++)
    {
      parent = *iter;
      if (!gtk_tree_model_iter_nth_child (tree_model, iter, &parent, indices[i]))
	return FALSE;
    }

  return TRUE;
}
static GtkTreePath *
_dtm_get_path (GtkTreeModel *tree_model,
	       GtkTreeIter  *iter)
{
  GtkTreePath *result;
  int index = 0;

  if (!NODE_DIAGRAM(iter) && !NODE_LAYER(iter) && !NODE_OBJECT(iter)) {
    /* the root path */
    return gtk_tree_path_new_first ();
  }

  result = gtk_tree_path_new ();

  if (NODE_DIAGRAM(iter)) {
    GList *list = dia_open_diagrams();
    gtk_tree_path_append_index (result, g_list_index (list, NODE_DIAGRAM(iter)));
  }
  if (NODE_LAYER(iter)) {
    g_return_val_if_fail (NODE_DIAGRAM(iter) == dia_layer_get_parent_diagram (NODE_LAYER(iter)), NULL);
    index = data_layer_get_index (NODE_DIAGRAM(iter), NODE_LAYER(iter));
    if (index >= 0)
      gtk_tree_path_append_index (result, index);
  }
  if (index >= 0 && NODE_OBJECT(iter)) {
    g_return_val_if_fail (NODE_LAYER(iter) == dia_object_get_parent_layer (NODE_OBJECT(iter)), NULL);
    index = dia_layer_object_get_index (NODE_LAYER(iter), NODE_OBJECT(iter));
    if (index >= 0)
      gtk_tree_path_append_index (result, index);
  }

  if (index < 0) {
    gtk_tree_path_free (result);
    return NULL;
  }
  return result;
}


static int
_dtm_iter_n_children (GtkTreeModel *tree_model,
                      GtkTreeIter  *iter)
{
  if (!iter)
    return g_list_length (dia_open_diagrams());

  if (NODE_OBJECT(iter)) {
    /* TODO: dive into objects (groups?)? */
    return 0;
  } else if (NODE_LAYER(iter)) {
    if (!NODE_LAYER(iter))
      return 0;
    return dia_layer_object_count (NODE_LAYER(iter));
  } else if (NODE_DIAGRAM(iter)) {
     if (!NODE_DIAGRAM(iter))
       return 0;
     return data_layer_count (NODE_DIAGRAM(iter));
  }
  return 0;
}


static void
_dtm_get_value (GtkTreeModel *tree_model,
                GtkTreeIter  *iter,
                int           column,
                GValue       *value)
{

  switch (column) {
  case DIAGRAM_COLUMN :
    g_value_init (value, G_TYPE_OBJECT);
    g_value_set_object (value, NODE_DIAGRAM(iter));
    break;
  case LAYER_COLUMN :
    g_value_init (value, G_TYPE_POINTER);
    g_value_set_pointer (value, NODE_LAYER(iter));
    break;
  case OBJECT_COLUMN :
    g_value_init (value, G_TYPE_POINTER);
    g_value_set_pointer (value, NODE_OBJECT(iter));
    break;
  case NAME_COLUMN :
    g_value_init (value, G_TYPE_STRING);
    /* deduce the requested name from the iter */
    if (NODE_OBJECT(iter))
      g_value_set_string (value, object_get_displayname (NODE_OBJECT (iter)));
    else if (NODE_LAYER(iter))
      g_value_set_string (value, dia_layer_get_name (NODE_LAYER (iter)));
    else if (NODE_DIAGRAM(iter))
      g_value_set_string (value, diagram_get_name (DIA_DIAGRAM(NODE_DIAGRAM(iter))));
    else /* warn on it? */
      g_value_set_string (value, NULL);
    break;
  default :
    g_assert_not_reached ();
  }
}
static gboolean
_dtm_iter_next (GtkTreeModel *tree_model,
		GtkTreeIter  *iter)
{
  int i;
  if (NODE_OBJECT(iter)) {
    if (!NODE_LAYER(iter))
      return FALSE;
    i = dia_layer_object_get_index (NODE_LAYER(iter), NODE_OBJECT(iter));
    ++i;
    NODE_OBJECT(iter) = dia_layer_object_get_nth (NODE_LAYER (iter), i);
    return NODE_OBJECT(iter) != NULL;
  } else if (NODE_LAYER(iter)) {
    if (!NODE_DIAGRAM(iter))
      return FALSE;
    i = data_layer_get_index (NODE_DIAGRAM(iter), NODE_LAYER(iter));
    ++i;
    NODE_LAYER(iter) = data_layer_get_nth (NODE_DIAGRAM(iter), i);
    return NODE_LAYER(iter) != NULL;
  } else if (NODE_DIAGRAM(iter)) {
    GList *list = dia_open_diagrams();
    i = g_list_index (list, NODE_DIAGRAM(iter));
    ++i;
    list = g_list_nth (list, i);
    NODE_DIAGRAM(iter) = list ? list->data : NULL;
    return NODE_DIAGRAM(iter) != NULL;
  } else {
    /* empty iter? */
    GList *list = dia_open_diagrams();
    NODE_DIAGRAM(iter) = list ? list->data : NULL;
    return NODE_DIAGRAM(iter) != NULL;
  }
  return FALSE;
}

static gboolean
_dtm_iter_children (GtkTreeModel *tree_model,
                    GtkTreeIter  *iter,
                    GtkTreeIter  *parent)
{
  if (parent) {
    if (NODE_OBJECT(parent))
      return FALSE;
    else if (NODE_LAYER(parent)) {
      NODE_OBJECT(iter) = dia_layer_object_get_nth (NODE_LAYER(parent), 0);
      if (NODE_OBJECT(iter)) {
        NODE_LAYER(iter) = dia_object_get_parent_layer(NODE_OBJECT(iter));
        NODE_DIAGRAM(iter) = dia_layer_get_parent_diagram (NODE_LAYER(iter));
        return TRUE;
      }
    } else if (NODE_DIAGRAM(parent)) {
      NODE_LAYER(iter) = data_layer_get_nth(NODE_DIAGRAM(parent), 0);
      if (NODE_LAYER(iter)) {
        NODE_DIAGRAM(iter) = dia_layer_get_parent_diagram (NODE_LAYER(iter));
        NODE_OBJECT(iter) = NULL;
        return TRUE;
      }
    } else {
      /* deliver root's children */
      parent = NULL;
    }
  }
  if (!parent) {
    /* the first diagram */
    GList *list = dia_open_diagrams();
    NODE_DIAGRAM(iter) = list ? list->data : NULL;
    NODE_LAYER(iter) = NULL;
    NODE_OBJECT(iter) = NULL;
    return NODE_DIAGRAM(iter) != NULL;
  }
  return FALSE;
}
static gboolean
_dtm_iter_has_child (GtkTreeModel *tree_model,
		     GtkTreeIter  *iter)
{
  return _dtm_iter_n_children (tree_model, iter) > 0;
}


static gboolean
_dtm_iter_nth_child (GtkTreeModel *tree_model,
                     GtkTreeIter  *iter,
                     GtkTreeIter  *parent,
                     int           n)
{
  if (parent) {
    *iter = *parent;
    if (NODE_OBJECT(parent)) {
      return FALSE;
    } else if (NODE_LAYER(parent)) {
      NODE_OBJECT(iter) = dia_layer_object_get_nth (NODE_LAYER(iter), n);
      return NODE_OBJECT(iter) != NULL;
    } else if (NODE_DIAGRAM(parent)) {
      NODE_LAYER(iter) = data_layer_get_nth(NODE_DIAGRAM(iter), n);
      return NODE_LAYER(iter) != NULL;
    }
  }
  if (!parent || !NODE_DIAGRAM(parent)) {
    /* the nth diagram */
    GList *list = dia_open_diagrams();
    list = g_list_nth(list, n);
    NODE_DIAGRAM(iter) = list ? list->data : NULL;
    NODE_LAYER(iter) = NULL;
    NODE_OBJECT(iter) = NULL;
    return NODE_DIAGRAM(iter) != NULL;
  }
  return FALSE;
}
static gboolean
_dtm_iter_parent (GtkTreeModel *tree_model,
		  GtkTreeIter  *iter,
		  GtkTreeIter  *child)
{
  *iter = *child;
  if (!NODE_DIAGRAM(child))
    return FALSE;
  if (NODE_OBJECT(child))
    NODE_OBJECT(iter) = NULL;
  else if (NODE_LAYER(child))
    NODE_LAYER(iter) = NULL;
  else if (NODE_DIAGRAM(child))
    NODE_DIAGRAM(iter) = NULL;
  else
    return FALSE;

  return TRUE;
}

static void
_dtm_iface_init (GtkTreeModelIface *iface)
{
  iface->get_flags = _dtm_get_flags;
  iface->get_n_columns = _dtm_get_n_columns;
  iface->get_column_type = _dtm_get_column_type;
  iface->get_iter = _dtm_get_iter;
  iface->get_path = _dtm_get_path;
  iface->get_value = _dtm_get_value;
  iface->iter_next = _dtm_iter_next;
  iface->iter_children = _dtm_iter_children;
  iface->iter_has_child = _dtm_iter_has_child;
  iface->iter_n_children = _dtm_iter_n_children;
  iface->iter_nth_child = _dtm_iter_nth_child;
  iface->iter_parent = _dtm_iter_parent;

  /*todo?*/
#if 0
  row_changed;
  row_inserted;
  row_has_child_toggled;
  row_deleted;
  rows_reordered;
#endif
}

/*
 * MODELCHANGES
 *
 * No matter if we are wrapped in the SortModel change signals are always
 * sent to this model. In the sortable case the GtkTreeModelSort translates
 * them to the right coordinates for the tree view.
 */
static gboolean
_recurse_row_inserted (GtkTreeModel *model, GtkTreeIter *parent)
{
  GtkTreeIter iter;
  int n = 0;
  while (_dtm_iter_nth_child (model, &iter, parent, n)) {
    GtkTreePath *path = _dtm_get_path (model, &iter);
    gtk_tree_model_row_inserted (model, path, &iter);
    /* gtk_tree_model_row_has_child_toggled
     * ... emitted when a row has gotten the first child row ...
     * So no need to do it here. Or maybe not for the sorted model?
     */
    if (_recurse_row_inserted (model, &iter))
      gtk_tree_model_row_has_child_toggled (model, path, &iter);
    gtk_tree_path_free (path);
    ++n;
  }
  return (n > 0);
}

/* listen on the diagram for object add/remove */
static void
_object_add (DiagramData      *dia,
             DiaLayer         *layer,
             DiaObject        *obj,
             DiagramTreeModel *dtm)
{
  GtkTreePath *path;
  GtkTreeIter _iter;
  GtkTreeIter *iter = &_iter;
  /* a bit backward: the first child is in the diagram, but not the tree yet */
  gboolean had_child = dia_layer_object_count (layer) > 1;

  g_return_if_fail (DIA_DIAGRAM(dia) != NULL);

  NODE_DIAGRAM(iter) = dia;
  NODE_LAYER(iter)   = layer;
  NODE_OBJECT(iter)  = obj;


  /* FIXME: this may get called before the layer is added to the tree */
  path = _dtm_get_path (GTK_TREE_MODEL (dtm), iter);
  if (path) {
    gtk_tree_model_row_inserted (GTK_TREE_MODEL (dtm), path, iter);
    /* enforce update */
    if (!had_child && gtk_tree_path_up (path))
      gtk_tree_model_row_has_child_toggled (GTK_TREE_MODEL (dtm), path, &_iter);
    gtk_tree_path_free (path);
  }
}

static void
_object_remove (DiagramData      *dia,
                DiaLayer         *layer,
                DiaObject        *obj,
                DiagramTreeModel *dtm)
{
  GtkTreePath *path;
  GtkTreeIter _iter;
  GtkTreeIter *iter = &_iter;
  gboolean last_child = dia_layer_object_count (layer) == 0;

  g_return_if_fail (DIA_DIAGRAM(dia) != NULL);

  NODE_DIAGRAM(iter) = dia;
  NODE_LAYER(iter)   = layer;
  NODE_OBJECT(iter)  = obj;

  path = _dtm_get_path (GTK_TREE_MODEL (dtm), iter);
  gtk_tree_model_row_deleted (GTK_TREE_MODEL (dtm), path);
  gtk_tree_path_free (path);
  /* enforce update - but the arrow on layer does not vanish */
  if (last_child) {
    NODE_OBJECT(iter)  = NULL;
    path = _dtm_get_path (GTK_TREE_MODEL (dtm), iter);
    gtk_tree_model_row_has_child_toggled (GTK_TREE_MODEL (dtm), path, iter);
    gtk_tree_path_free (path);
  }
}
/* start listening for diagram specific object changes */
static void
_dtm_listen_on_diagram (DiagramTreeModel *dtm,
			Diagram          *dia)
{
  g_signal_connect (G_OBJECT(dia), "object_add", G_CALLBACK(_object_add), dtm);
  g_signal_connect (G_OBJECT(dia), "object_remove", G_CALLBACK(_object_remove), dtm);
}
/* listen to diagram creation */
static void
_diagram_add (DiaApplication   *app,
              Diagram          *dia,
	      DiagramTreeModel *dtm)
{
  GtkTreePath *path;
  GtkTreeIter _iter = {0,}; /* all null is our root */
  GtkTreeIter *iter = &_iter;

  NODE_DIAGRAM(iter) = DIA_DIAGRAM_DATA (dia);
  path = _dtm_get_path (GTK_TREE_MODEL (dtm), iter);
  /* we always get a path, but it may not be a valid one */
  if (path) {
    /* gtk_tree_model_row_changed is not 'strong' enough, lets try to re-add the root */
    gtk_tree_model_row_inserted (GTK_TREE_MODEL (dtm), path, iter);
    /* looks like the GtkTreeView is somewhat out of service */
    if (_recurse_row_inserted (GTK_TREE_MODEL (dtm), iter))
      gtk_tree_model_row_has_child_toggled (GTK_TREE_MODEL (dtm), path, iter);
    gtk_tree_path_free (path);
  }
  _dtm_listen_on_diagram (dtm, dia);
}
static void
_diagram_change (DiaApplication   *app,
                 Diagram          *dia,
		 guint             flags,
		 gpointer          object,
	         DiagramTreeModel *dtm)
{
  GtkTreePath *path;
  GtkTreeIter _iter = {0,};
  GtkTreeIter *iter = &_iter;

  NODE_DIAGRAM(iter) = DIA_DIAGRAM_DATA(dia);

  if (flags & DIAGRAM_CHANGE_NAME)
    /* nothing special */;
  if (flags & DIAGRAM_CHANGE_LAYER)
    NODE_LAYER(iter) = object;
  if (flags & DIAGRAM_CHANGE_OBJECT) {
    NODE_OBJECT(iter) = object;
    NODE_LAYER(iter) = dia_object_get_parent_layer (object);
  }

  path = _dtm_get_path (GTK_TREE_MODEL (dtm), iter);
  gtk_tree_model_row_changed (GTK_TREE_MODEL (dtm), path, iter);
  gtk_tree_path_free (path);
}
static void
_diagram_remove (DiaApplication   *app,
                 Diagram          *dia,
	         DiagramTreeModel *dtm)
{
  GtkTreePath *path;
  GtkTreeIter _iter = {0,};
  GtkTreeIter *iter = &_iter;

  NODE_DIAGRAM(iter) = DIA_DIAGRAM_DATA(dia);
  NODE_LAYER(iter)   = NULL;
  NODE_OBJECT(iter)  = NULL;

  path = _dtm_get_path (GTK_TREE_MODEL (dtm), iter);
  gtk_tree_model_row_deleted (GTK_TREE_MODEL (dtm), path);
  gtk_tree_path_free (path);

  /* stop listening on this diagram */
  g_signal_handlers_disconnect_by_func (dia, _object_add, dtm);
  g_signal_handlers_disconnect_by_func (dia, _object_remove, dtm);
}

static void
_dtm_init (DiagramTreeModel *dtm)
{
  /* connect to interesting state changes */
  g_signal_connect (G_OBJECT (dia_application_get_default ()),
                    "diagram_add", G_CALLBACK (_diagram_add), dtm);
  g_signal_connect (G_OBJECT (dia_application_get_default ()),
                    "diagram_change", G_CALLBACK (_diagram_change), dtm);
  g_signal_connect (G_OBJECT(dia_application_get_default ()),
                    "diagram_remove", G_CALLBACK (_diagram_remove), dtm);
  /* also connect to every already existing diagram */
  {
    GList *list =  dia_open_diagrams();

    while (list) {
      _dtm_listen_on_diagram (dtm, DIA_DIAGRAM(list->data));
      list = g_list_next (list);
    }
  }
}

static void
_dtm_finalize (GObject *object)
{
  DiagramTreeModel *dtm = (DiagramTreeModel *)object;

  g_signal_handlers_disconnect_by_func (G_OBJECT (dia_application_get_default ()), _diagram_add, dtm);
  g_signal_handlers_disconnect_by_func (G_OBJECT (dia_application_get_default ()), _diagram_change, dtm);
  g_signal_handlers_disconnect_by_func (G_OBJECT (dia_application_get_default ()), _diagram_remove, dtm);

  G_OBJECT_CLASS(_dtm_parent_class)->finalize (object);
}


/* SORTABLE
 * Wrapper around the original model to allow sorting by various columns IDs
 */
static int
cmp_diagram (GtkTreeIter *a,
             GtkTreeIter *b)
{
  DiagramData *pa = NODE_DIAGRAM (a), *pb = NODE_DIAGRAM (b);
  char *na, *nb;
  int ret;

  if (pa == pb) {
    return 0;
  }

  na = diagram_get_name (DIA_DIAGRAM (pa));
  nb = diagram_get_name (DIA_DIAGRAM (pa));

  if (!na || !nb) {
    return (na > nb) ? -1 : 1;
  }

  ret = strcmp (na, nb);

  g_clear_pointer (&na, g_free);
  g_clear_pointer (&nb, g_free);

  return ret;
}


static int
cmp_layer (GtkTreeIter  *a,
           GtkTreeIter  *b)
{
  DiaLayer *pa = NODE_LAYER(a), *pb = NODE_LAYER(b);
  const char *na, *nb;
  int ret;
  if (pa == pb)
    return 0;
  na = dia_layer_get_name (pa);
  nb = dia_layer_get_name (pb);
  if (!na || !nb)
    return (na > nb) ? -1 : 1;
  ret = strcmp (na, nb);

  return ret;
}


static int
name_sort_func (GtkTreeModel *model,
                GtkTreeIter  *a,
                GtkTreeIter  *b,
                gpointer      user_data)
{
  DiaObject *pa = NODE_OBJECT (a), *pb = NODE_OBJECT (b);
  char *na, *nb;
  int ret = cmp_diagram (a, b);

  if (ret) {
    return ret;
  }

  ret = cmp_layer (a, b);

  if (ret) {
    return ret;
  }

  if (pa == pb) {
    return 0;
  } else if (!pa || !pb) {
    return (pa > pb) ? -1 : 1;
  }

  na = object_get_displayname (pa);
  nb = object_get_displayname (pb);

  if (!na || !nb) {
    return (na > nb) ? -1 : 1;
  }

  ret = strcmp (na, nb);

  g_clear_pointer (&na, g_free);
  g_clear_pointer (&nb, g_free);

  return ret;
}


static int
type_sort_func (GtkTreeModel *model,
                GtkTreeIter  *a,
                GtkTreeIter  *b,
                gpointer      user_data)
{
  DiaObject *pa = NODE_OBJECT(a), *pb = NODE_OBJECT(b);
  int ret = cmp_diagram (a, b);
  if (ret)
    return ret;
  ret = cmp_layer (a, b);
  if (ret)
    return ret;
  if (pa == pb)
    return 0;
  else if (!pa || !pb)
    return (pa > pb) ? -1 : 1;
  return strcmp (pa->type->name, pb->type->name);
}


static GtkTreeModel *
wrap_as_sortable_model (GtkTreeModel *model)
{
  GtkTreeModel *sort_model = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (model));

  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (sort_model), NAME_COLUMN, name_sort_func, model, NULL);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (sort_model), OBJECT_COLUMN, type_sort_func, model, NULL);

  return sort_model;
}

GtkTreeModel *
diagram_tree_model_new (void)
{
  GtkTreeModel *model = g_object_new (DIA_TYPE_DIAGRAM_TREE_MODEL, NULL);
  model = wrap_as_sortable_model (model);
  return model;
}
