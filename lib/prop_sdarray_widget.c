/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 *
 * Properties List Widget
 * Copyright (C) 2007, 2013  Hans Breuer
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

/*
 The basic idea is to register *every* Dia property within the GType system.
 Thus we could just put these into e.g. a list-store, but some range checking
 may also be possible for load/bindings/etc.

 The registered type could replace the type quark as it would give a fast
 unique number as well.

 Prop names with namespces like UML? No. they would by nothing new, a simple
 'dia__' should be enough to make them unique. Also the basic idea of Dia's
 property type system always was: same name, same type.

 Stuff like 'line_width' should mean the same everywhere ...
 *BUT* does the same assumption hold for 'attributes (UML::Class|Database::Table)' ??

 First iteration:
   - dia prop type (PROP_TYPE_*) plus member name ('line_width') give a unique GType
   + PROP_TYPE_INT also may include a range
   + PROP_TYPE_ENUM
   -

 WHEN TO REGISTER?
  - during prop_desc_list_calculate_quarks() that is on firts access of *_describe_props()
  - (eralier?) during object_register_type() -

 SO MUCH FOR THE ORIGINAL IDEA.

 what's been implemented below is something a lot simpler, just enough to
 support exisiting ArrayProp usage ;)
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include "properties.h"
#include "propinternals.h"
#include "diacellrendererenum.h"
#include "prop_sdarray_widget.h"

/** A small wrapper to connect to the model */
static GtkCellRenderer *
_cell_renderer_enum_new (const Property *p, GtkTreeView *tree_view)
{
  const EnumProperty *prop = (const EnumProperty *)p;
  PropEnumData *enumdata = prop->common.descr->extra_data;
  GtkCellRenderer *cren = dia_cell_renderer_enum_new (enumdata, tree_view);

  return cren;
}

static GtkCellRenderer *
_cell_renderer_spin_new (const Property *p, GtkTreeView *tree_view)
{
  return gtk_cell_renderer_spin_new ();
}

/** Wrapper to setup ranges */
static GtkCellRenderer *
_cell_renderer_real_new (const Property *p, GtkTreeView *tree_view)
{
  const RealProperty *prop = (RealProperty *)p;
  GtkCellRenderer *cren = gtk_cell_renderer_spin_new ();
  PropNumData *numdata = prop->common.descr->extra_data;
  GtkAdjustment *adj;

  /* must be non NULL to make it editable */
  adj = GTK_ADJUSTMENT (gtk_adjustment_new (prop->real_data,
			    numdata->min, numdata->max,
			    numdata->step,
			    10.0 * numdata->step, 0));

  g_object_set (G_OBJECT (cren), "adjustment", adj, NULL);

  return cren;
}
/** Make it editable, connect signals */
static void
_toggle_callback (GtkCellRendererToggle *renderer,
                  gchar                 *path_string,
                  GtkTreeView           *tree_view)
{
  GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
  GtkTreePath *path;
  GtkTreeIter iter;
  gboolean value;
  int column;

  path = gtk_tree_path_new_from_string (path_string);
  if (!gtk_tree_model_get_iter (model, &iter, path))
    {
      g_warning ("%s: bad path?", G_STRLOC);
      return;
    }
  gtk_tree_path_free (path);

  column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (renderer), COLUMN_KEY));

  gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
                      column, &value, -1);
  gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
                      column, !value, -1);
  g_object_set_data (G_OBJECT (model), "modified", GINT_TO_POINTER (1));
}
static GtkCellRenderer *
_cell_renderer_toggle_new (const Property *p, GtkTreeView *view)
{
  GtkCellRenderer *cren = gtk_cell_renderer_toggle_new ();

  g_object_set (G_OBJECT (cren),
		"mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE,
		"activatable", TRUE,
		NULL);
  g_signal_connect(G_OBJECT (cren), "toggled",
		   G_CALLBACK (_toggle_callback), view);

  return cren;
}
static void
_text_edited (GtkCellRenderer *renderer,
	      gchar           *path_string,
	      gchar           *new_text,
	      GtkTreeView     *tree_view)
{
  GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
  GtkTreePath *path;
  GtkTreeIter iter;
  gchar *value;
  int column;

  path = gtk_tree_path_new_from_string (path_string);
  if (!gtk_tree_model_get_iter (model, &iter, path))
    {
      g_warning ("%s: bad path?", G_STRLOC);
      return;
    }
  gtk_tree_path_free (path);

  column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (renderer), COLUMN_KEY));

  gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
                      column, &value, -1);
  g_clear_pointer (&value, g_free);
  gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
                      column, g_strdup (new_text), -1);
  g_object_set_data (G_OBJECT (model), "modified", GINT_TO_POINTER (1));
}
static GtkCellRenderer *
_cell_renderer_text_new (const Property *p, GtkTreeView *tree_view)
{
  GtkCellRenderer *cren = gtk_cell_renderer_text_new ();

  g_signal_connect (G_OBJECT (cren), "edited",
		    G_CALLBACK (_text_edited), tree_view);
  g_object_set (G_OBJECT (cren), "editable", TRUE, NULL);

  return cren;
}
static GtkCellRenderer *
_cell_renderer_multitext_new (const Property *p, GtkTreeView *tree_view)
{
  GtkCellRenderer *cren = gtk_cell_renderer_text_new ();

  g_signal_connect (G_OBJECT (cren), "edited",
		    G_CALLBACK (_text_edited), tree_view);
  g_object_set (G_OBJECT (cren), "editable", TRUE, NULL);
  g_object_set (G_OBJECT (cren), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  g_object_set (G_OBJECT (cren), "wrap-width", 140, NULL);

  return cren;
}

typedef void (*DataFunc) (GtkTreeViewColumn *tree_column,
			  GtkCellRenderer   *cell,
			  GtkTreeModel      *model,
			  GtkTreeIter       *iter,
			  gpointer           user_data);

static struct {
  const char *type;  /* the type sting */
  GQuark      type_quark; /* it's calculated quark */
  GType       gtype;
  GtkCellRenderer *(*create_renderer) (const Property *prop, GtkTreeView *tree_view);
  const char *bind;
  DataFunc    data_func;
} _dia_gtk_type_map[] = {
  { PROP_TYPE_DARRAY, 0, G_TYPE_POINTER, /* child data model */ },
  { PROP_TYPE_BOOL, 0, G_TYPE_BOOLEAN, _cell_renderer_toggle_new, "active" },
  { PROP_TYPE_INT, 0, G_TYPE_INT, _cell_renderer_spin_new },
  { PROP_TYPE_ENUM, 0, G_TYPE_INT, _cell_renderer_enum_new, "text" },
  { PROP_TYPE_REAL, 0, G_TYPE_DOUBLE, _cell_renderer_real_new },
  { PROP_TYPE_STRING, 0, G_TYPE_STRING, _cell_renderer_text_new, "text" },
  { PROP_TYPE_MULTISTRING, 0, G_TYPE_STRING, _cell_renderer_multitext_new, "text" },
  { NULL, 0 }
};

static int
_find_type (const Property *prop)
{
  int i;

  /* calculate quarks first time called */
  if (_dia_gtk_type_map[0].type_quark == 0) {
    /* dynamic to avoid: error C2099: initializer is not a constant */
    _dia_gtk_type_map[0].gtype = GTK_TYPE_TREE_MODEL;

    for (i = 0; _dia_gtk_type_map[i].type != NULL; ++i) {
      _dia_gtk_type_map[i].type_quark = g_quark_from_static_string (_dia_gtk_type_map[i].type);
    }
  }

  for (i = 0; _dia_gtk_type_map[i].type != NULL; ++i) {
    if (prop->type_quark == _dia_gtk_type_map[i].type_quark)
      return i;
  }
  return -1;
}


/**
 * create_sdarray_model:
 * @prop: the #ArrayProperty to model
 *
 * Create an empty model (list store) with Dia types mapped to #GType
 *
 * Since: dawn-of-time
 */
static GtkTreeStore *
create_sdarray_model (ArrayProperty *prop)
{
  int idx, i, columns = prop->ex_props->len;
  GtkTreeStore *model;
  GType *types = g_alloca (sizeof(GType) * columns);
  int branch_column = -1;
  ArrayProperty *branch_prop = NULL;

  for (i = 0; i < columns; i++) {
    Property *p = g_ptr_array_index(prop->ex_props, i);

    /* map Dia's property types to gtk-tree_model */
    idx = _find_type (p);
    if (idx >= 0) {
      types[i] = _dia_gtk_type_map[idx].gtype;
      if (g_quark_from_static_string (PROP_TYPE_DARRAY) == p->type_quark) {
	branch_column = i;
	branch_prop = (ArrayProperty *)p;
      }
    } else {
      types[i] = G_TYPE_POINTER;
      g_warning (G_STRLOC "No model type for '%s'\n", p->descr->name);
    }
  }
  model = gtk_tree_store_newv (columns, types);
  g_object_set_data (G_OBJECT (model), "branch-column", GINT_TO_POINTER (branch_column));
  /* we need to remember the branch property to create a model from scratch */
  if (branch_prop)
    g_object_set_data (G_OBJECT (model), "branch-prop", branch_prop);

  return model;
}

static gboolean
_get_active_iter (GtkTreeView *tree_view,
	          GtkTreeIter *iter)
{
  GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));

  /* will not work with GTK_SELECTION_MULTIPLE */
  if (!gtk_tree_selection_get_selected (selection, NULL, iter)) {
    /* nothing selected yet, just use the start */
    return gtk_tree_model_get_iter_first (model, iter);
  } else {
    /* done with it */
    return TRUE;
  }
}
/* Working on the model, not the properties */
static void
_insert_row_callback (GtkWidget   *button,
		      GtkTreeView *tree_view)
{
  GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
  GtkTreeIter iter;

  if (!_get_active_iter (tree_view, &iter))
    gtk_tree_store_insert_after (GTK_TREE_STORE (model), &iter, NULL, NULL);
  else
    gtk_tree_store_insert_after (GTK_TREE_STORE (model), &iter, NULL, &iter);
  gtk_tree_selection_select_iter (gtk_tree_view_get_selection (tree_view), &iter);

}
static void
_remove_row_callback (GtkWidget   *button,
		      GtkTreeView *tree_view)
{
  GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
  GtkTreeIter iter;

  if (_get_active_iter (tree_view, &iter)) {
    GtkTreeIter next = iter;
    if (gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &next))
      gtk_tree_selection_select_iter (gtk_tree_view_get_selection (tree_view), &next);
    gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
  }
}
static void
_upper_row_callback (GtkWidget   *button,
		     GtkTreeView *tree_view)
{
  GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
  GtkTreeIter iter;

  if (_get_active_iter (tree_view, &iter)) {
    /* There is no gtk_tree_model_iter_prev, so we have to resort to pathes */
    GtkTreePath *path = gtk_tree_model_get_path (model, &iter);
    GtkTreeIter prev;

    if (   path != NULL
        && gtk_tree_path_prev (path)
        && gtk_tree_model_get_iter (model, &prev, path))
      gtk_tree_store_move_before (GTK_TREE_STORE (model), &iter, &prev);
    else
      gtk_tree_store_move_before (GTK_TREE_STORE (model), &iter, NULL);
    gtk_tree_path_free (path);
  }
}
static void
_lower_row_callback (GtkWidget   *button,
		     GtkTreeView *tree_view)
{
  GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
  GtkTreeIter iter;

  if (_get_active_iter (tree_view, &iter)) {
    GtkTreeIter pos = iter;
    if (gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &pos))
      gtk_tree_store_move_after (GTK_TREE_STORE (model), &iter, &pos);
    else
      gtk_tree_store_move_after (GTK_TREE_STORE (model), &iter, NULL);
  }
}

/* react to the row-selected signal of the main tree view */
static void
_update_branch (GtkTreeSelection *selection,
	        GtkTreeView      *tree_view)
{
  GtkTreeIter   iter;
  GtkTreeView  *branch_view = g_object_get_data (G_OBJECT (tree_view), "branch-view");
  GtkTreeModel *model = gtk_tree_view_get_model (tree_view);

  if (!branch_view)
    return;

  if (gtk_tree_selection_get_selected(selection, NULL, &iter)) {
    GtkTreeModel *branch_model = NULL;
    int column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (model), "branch-column"));

    gtk_tree_model_get (model, &iter, column, &branch_model, -1);
    if (!branch_model) {
      branch_model = GTK_TREE_MODEL (create_sdarray_model (
	(ArrayProperty *)g_object_get_data (G_OBJECT (model), "branch-prop")));
      /* remember it for later reference */
      gtk_tree_store_set (GTK_TREE_STORE (model), &iter, column, branch_model, -1);
    }
    gtk_tree_view_set_model (branch_view, branch_model);
    g_clear_object (&branch_model);
  } else {
    gtk_tree_view_set_model (branch_view, NULL);
  }
}

static void
_build_tree_view_columns (GtkTreeView    *view,
			  ArrayProperty  *prop,
			  ArrayProperty **branch_prop)
{
  int idx, i, cols;

  /* to keep add/remove simple */
  gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (view)),
			       GTK_SELECTION_SINGLE);
  cols = prop->ex_props->len;
  for (i = 0; i < cols; i++) {
    /* for every property type we need a cell renderer and view */
    Property *p = g_ptr_array_index(prop->ex_props, i);

    idx = _find_type (p);
    if (p->type_quark == g_quark_from_static_string (PROP_TYPE_DARRAY)) {
      g_return_if_fail (idx == 0 && GTK_TYPE_TREE_MODEL == _dia_gtk_type_map[idx].gtype);
      g_return_if_fail (*branch_prop == NULL); /* only one branch per row */
      *branch_prop = (ArrayProperty*)p;
    }
    if (idx >= 0) {
      GtkCellRenderer *renderer;
      GtkTreeViewColumn *col;

      if (!_dia_gtk_type_map[idx].create_renderer)
	continue;

      renderer = (_dia_gtk_type_map[idx].create_renderer) (p, view);
      g_object_set_data (G_OBJECT (renderer), COLUMN_KEY, GINT_TO_POINTER (i));
      col = gtk_tree_view_column_new_with_attributes (
		p->descr->description, renderer,
		_dia_gtk_type_map[idx].bind, i,
		NULL);
      gtk_tree_view_column_set_sort_column_id (col, i);
      gtk_tree_view_column_set_cell_data_func (col, renderer,
					       _dia_gtk_type_map[idx].data_func,
					       GINT_TO_POINTER(i), NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (view), col);

      if (p->descr->tooltip) {
	/* FIXME: does not work, probably due to immediately done size calculation */
	GtkTooltip *tooltip = g_object_new (GTK_TYPE_TOOLTIP, NULL);

	gtk_tooltip_set_text (tooltip, p->descr->tooltip);

	gtk_tree_view_set_tooltip_cell (GTK_TREE_VIEW (view), tooltip, NULL, col, NULL);
      }
    } else {
      g_print ("No model type for '%s'\n", p->descr->name);
    }
  }

}

static void
_update_button (GtkTreeSelection *selection,
		GtkWidget        *button)
{
  gtk_widget_set_sensitive (button, gtk_tree_selection_get_selected(selection, NULL, NULL));
}

/*!
 * \brief Setup buttons including action and sensitivity signals
 */
static GtkWidget *
_make_button_box_for_view (GtkTreeView *view, GtkTreeView *master_view)
{
  static struct {
    const gchar *stock;
    GCallback    callback;
  } _button_data[] = {
    { "gtk-add",     G_CALLBACK (_insert_row_callback) },
    { "gtk-remove",  G_CALLBACK (_remove_row_callback) },
    { "gtk-go-up",   G_CALLBACK (_upper_row_callback) },
    { "gtk-go-down", G_CALLBACK (_lower_row_callback) },
    { NULL, NULL }
  };
  GtkWidget *vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *button;
  int i;

  for (i = 0; _button_data[i].stock != NULL; ++i) {
    button = gtk_button_new_from_icon_name (_button_data[i].stock, GTK_ICON_SIZE_LARGE_TOOLBAR);
    /* start with everything disabled ... */
    gtk_widget_set_sensitive (button, FALSE);
    g_signal_connect (G_OBJECT (button), "clicked",
		      _button_data[i].callback, view);
    if (i != 0) /* the Add enabling is different ... */
      g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (view)), "changed",
		        G_CALLBACK (_update_button), button);
    else if (master_view) /* ... it depends on the other views selection */
      g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (master_view)), "changed",
		        G_CALLBACK (_update_button), button);
    else /* ... the main button is always enabled */
      gtk_widget_set_sensitive (button, TRUE);
    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  }
  return vbox;
}
/*! Wrap the given widget into a scrollable setting certain defaults */
static GtkWidget *
_make_scrollable (GtkWidget *view)
{
  GtkWidget *sw;

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (sw), view);
  gtk_widget_show (sw);
  gtk_widget_set_vexpand (sw, TRUE);
  gtk_widget_set_hexpand (sw, TRUE);

  return sw;
}
/*!
 * PropertyType_GetWidget: create a widget capable of editing the property
 */
WIDGET *
_arrayprop_get_widget (ArrayProperty *prop, PropDialog *dialog)
{
  GtkTreeStore *model;
  GtkWidget *view;
  GtkWidget *branch_view = NULL;
  ArrayProperty *branch_prop = NULL;

  /* create */
  model = create_sdarray_model (prop);
  /* visualize */
  view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
  gtk_widget_set_vexpand (view, TRUE);
  /* to adapt the branch_view */
  g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (view)), "changed",
		    G_CALLBACK (_update_branch), view);

  _build_tree_view_columns (GTK_TREE_VIEW (view), prop, &branch_prop);
  if (branch_prop) {
    ArrayProperty *second_branch_prop = NULL;
    branch_view = gtk_tree_view_new ();

    _build_tree_view_columns (GTK_TREE_VIEW (branch_view), branch_prop, &second_branch_prop);
    if (second_branch_prop)
      g_warning (G_STRLOC " Only one nesting level of PROP_TYPE_DARRAY supported");
  }

  /* setup additional controls ...
   *  hbox = outer container
   *   \- vbox
   *   \- view
   * or:
   *  hbox
   *   \- vbox - major buttons
   *   \- vbox2
   *       \- view
   *       \- hbox2
   *           \- vbox3 - minor buttons
   *           \- branch_view
   */
  {
    GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL /* less size for button column */, 6);
    GtkWidget *vbox = _make_button_box_for_view (GTK_TREE_VIEW (view), NULL);

    gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE /* no expand */, FALSE, 0);

    gtk_widget_show_all (vbox);

    if (!branch_view) {
      gtk_widget_show (view);
      gtk_box_pack_start (GTK_BOX (hbox), _make_scrollable (view), TRUE /* expand */, TRUE /* fill */, 0);
    } else {
      /* almost the same once more */
      GtkWidget *hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL /* less size for button column */, 0);
      GtkWidget *vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
      GtkWidget *vbox3 = _make_button_box_for_view (GTK_TREE_VIEW (branch_view), GTK_TREE_VIEW (view));

      gtk_box_pack_start (GTK_BOX (vbox2), _make_scrollable (view), TRUE, TRUE, 0);
      /* Todo: get label for the branch view from props, e.g. UML Operations Parameters  */
      gtk_box_pack_start (GTK_BOX (vbox2), gtk_label_new (_("Parameters")), FALSE, FALSE, 0);

      gtk_box_pack_start (GTK_BOX (hbox2), vbox3, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (hbox2), _make_scrollable (branch_view), TRUE, TRUE, 0);
      gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, FALSE, 0);
      gtk_widget_show_all (vbox2);
      gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE /* expand */, TRUE /* fill */, 0);

      g_object_set_data (G_OBJECT (view), "branch-view", branch_view);
    }
    g_object_set_data (G_OBJECT (hbox), "tree-view", view);
    gtk_widget_set_vexpand (hbox, TRUE);
    return hbox;
  }
}

/*!
 * \brief Transfer from the property to the view model
 */
static void
_write_store (GtkTreeStore *store, GtkTreeIter *parent_iter, ArrayProperty *prop)
{
  int idx, i, j, cols, rows;

  cols = prop->ex_props->len;
  rows = prop->records->len;

  for (j = 0; j < rows; ++j) {
    GtkTreeIter iter;
    GPtrArray *r = g_ptr_array_index(prop->records, j);

    gtk_tree_store_append (store, &iter, parent_iter);

    for (i = 0; i < cols; ++i) {
      Property *p = g_ptr_array_index(r,i);

      idx = _find_type (p);
      if (idx < 0)
	continue;

      if (p->type_quark == g_quark_from_static_string (PROP_TYPE_DARRAY)) {
        /* recurse - with own store */
        GtkTreeStore *child_store = create_sdarray_model ((ArrayProperty *)p);
        _write_store (child_store, NULL, (ArrayProperty *)p);
        gtk_tree_store_set (store, &iter, i, child_store, -1);
        g_clear_object (&child_store);
      } else if (p->type_quark == g_quark_from_static_string (PROP_TYPE_BOOL))
	gtk_tree_store_set (store, &iter, i, ((BoolProperty *)p)->bool_data, -1);
      else if (p->type_quark == g_quark_from_static_string (PROP_TYPE_INT))
	gtk_tree_store_set (store, &iter, i, ((IntProperty *)p)->int_data, -1);
      else if (p->type_quark == g_quark_from_static_string (PROP_TYPE_ENUM))
	gtk_tree_store_set (store, &iter, i, ((EnumProperty *)p)->enum_data, -1);
      else if (p->type_quark == g_quark_from_static_string (PROP_TYPE_REAL))
	gtk_tree_store_set (store, &iter, i, ((RealProperty *)p)->real_data, -1);
      else if (   p->type_quark == g_quark_from_static_string (PROP_TYPE_STRING)
	       || p->type_quark == g_quark_from_static_string (PROP_TYPE_MULTISTRING))
	gtk_tree_store_set (store, &iter, i, ((StringProperty *)p)->string_data, -1);
      else {
	/* only complain if we have a visible widget */
	if (_dia_gtk_type_map[idx].create_renderer != NULL)
	  g_warning (G_STRLOC " Missing getter for '%s'", p->descr->type);
      }
    }
  }
}

/*!
 * PropertyType_ResetWidget: get the value of the property into the widget
 */
void
_arrayprop_reset_widget(ArrayProperty *prop, WIDGET *widget)
{
  GtkWidget *view = widget;
  GtkTreeView *tree_view = g_object_get_data (G_OBJECT (view), "tree-view");
  GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (tree_view));
  GtkTreeIter iter;

  gtk_tree_store_clear (store);

  _write_store (store, NULL, prop);
  g_object_set_data (G_OBJECT (store), "modified", GINT_TO_POINTER (0));

  /* select the first row if any */
  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter)) {
    GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
    gtk_tree_view_set_cursor (tree_view, path, NULL, FALSE);
    gtk_tree_path_free (path);
  } else {
    /* do something to disable the buttons */
  }
}


static gboolean
_array_prop_adjust_len (ArrayProperty *prop, guint len)
{
  guint i;
  guint num_props = prop->ex_props->len;

  if (prop->records->len == len) {
    return FALSE;
  }

  /* see also: pydia-property.c */
  for (i = len; i < prop->records->len; ++i) {
    GPtrArray *record = g_ptr_array_index (prop->records, i);

    for (guint j = 0; j < num_props; j++) {
      Property *inner = g_ptr_array_index (record, j);

      inner->ops->free (inner);
    }
    g_ptr_array_free (record, TRUE);
  }
  for (i = prop->records->len; i < len; ++i) {
    GPtrArray *record = g_ptr_array_new ();

    for (guint j = 0; j < num_props; j++) {
      Property *ex = g_ptr_array_index (prop->ex_props, j);
      Property *inner = ex->ops->copy (ex);

      g_ptr_array_add (record, inner);
    }

    g_ptr_array_add (prop->records, record);
  }
  g_ptr_array_set_size (prop->records, len);

  return TRUE;
}


/*!
 * \brief Transfer from the view model to the property
 */
static void
_read_store (GtkTreeStore *store, GtkTreeIter *iter, ArrayProperty *prop)
{
  GtkTreeModel *model = GTK_TREE_MODEL (store);
  int idx, i, j, cols, rows;
  GtkTreeIter parent_iter;
  gboolean modified;

  cols = prop->ex_props->len;

  if (gtk_tree_model_iter_parent (model, &parent_iter, iter))
    modified = _array_prop_adjust_len (prop, gtk_tree_model_iter_n_children (model, &parent_iter));
  else
    modified = _array_prop_adjust_len (prop, gtk_tree_model_iter_n_children (model, NULL));
  /* Length adjustment might be the only thing done ... */
  if (modified)
    g_object_set_data (G_OBJECT (store), "modified", GINT_TO_POINTER (1));
  rows = prop->records->len;

  for (j = 0; j < rows; ++j) {
    GPtrArray *r = g_ptr_array_index(prop->records, j);

    for (i = 0; i < cols; ++i) {
      Property *p = g_ptr_array_index(r,i);

      idx = _find_type (p);
      if (idx < 0)
	continue;

      if (p->type_quark == g_quark_from_static_string (PROP_TYPE_DARRAY)) {
        /* recurse - with own store */
        GtkTreeStore *child_store;
        GtkTreeIter child_iter;

        gtk_tree_model_get (model, iter, i, &child_store, -1);
        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (child_store), &child_iter)) {
          _read_store (child_store, &child_iter, (ArrayProperty *) p);
        }
        /* FIXME: if this is working we might have a string leak below */
        g_clear_object (&child_store);
      } else if (p->type_quark == g_quark_from_static_string (PROP_TYPE_BOOL))
	gtk_tree_model_get (model, iter, i, &((BoolProperty *)p)->bool_data, -1);
      else if (p->type_quark == g_quark_from_static_string (PROP_TYPE_INT))
	gtk_tree_model_get (model, iter, i, &((IntProperty *)p)->int_data, -1);
      else if (p->type_quark == g_quark_from_static_string (PROP_TYPE_ENUM))
	gtk_tree_model_get (model, iter, i, &((EnumProperty *)p)->enum_data, -1);
      else if (p->type_quark == g_quark_from_static_string (PROP_TYPE_REAL))
	gtk_tree_model_get (model, iter, i, &((RealProperty *)p)->real_data, -1);
      else if (   p->type_quark == g_quark_from_static_string (PROP_TYPE_STRING)
	       || p->type_quark == g_quark_from_static_string (PROP_TYPE_MULTISTRING)) {
	StringProperty *pst = (StringProperty *)p;
	gchar *value;
	gtk_tree_model_get (model, iter, i, &value, -1);
	g_clear_pointer (&pst->string_data, g_free);
	pst->string_data = g_strdup (value);
      } else {
	/* only complain if we have a visible widget */
	if (_dia_gtk_type_map[idx].create_renderer != NULL)
	  g_warning (G_STRLOC " Missing setter for '%s'", p->descr->type);
      }
    }

    gtk_tree_model_iter_next (model, iter);
  }
}

/*!
 * PropertyType_SetFromWidget: set the value of the property from the
 * current value of the widget
 */
void
_arrayprop_set_from_widget(ArrayProperty *prop, WIDGET *widget)
{
  GtkWidget *view = widget;
  GtkTreeView *tree_view = g_object_get_data (G_OBJECT (view), "tree-view");
  GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (tree_view));
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter))
    _read_store (store, &iter, prop);

  /* enough to replace prophandler_connect() ? */
  if (g_object_get_data (G_OBJECT (store), "modified"))
    prop->common.experience &= ~PXP_NOTSET;
}
