/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
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

#include <config.h>

#include <assert.h>
#include <string.h>

#include "intl.h"
#include "diagram.h"
#include "group.h"
#include "object_ops.h"
#include "render_eps.h"
#include "focus.h"
#include "message.h"
#include "menus.h"
#include "preferences.h"
#include "properties.h"
#include "cut_n_paste.h"
#include "layer_dialog.h"
#include "app_procs.h"
#include "dia_dirs.h"
#include "load_save.h"
#include "recent_files.h"
#include "diagram_tree_window.h"
#include "autosave.h"
#include "dynamic_refresh.h"

static GList *open_diagrams = NULL;

static void diagram_class_init (DiagramClass *klass);
static void diagram_init(Diagram *obj, const char *filename);

static gpointer parent_class = NULL;

GType
diagram_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (DiagramClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) diagram_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (Diagram),
        0,              /* n_preallocs */
	NULL            /* init */
      };

      object_type = g_type_register_static (G_TYPE_OBJECT,
                                            "Diagram",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void
diagram_finalize(GObject *object) 
{
  Diagram *dia = DIA_DIAGRAM(object);
  Diagram *other_diagram;

  assert(dia->displays==NULL);
  g_object_unref(dia->data);
  dia->data = NULL;
  
  g_free(dia->filename);

  open_diagrams = g_list_remove(open_diagrams, dia);
  layer_dialog_update_diagram_list();

  undo_destroy(dia->undo);
  
  diagram_tree_remove(diagram_tree(), dia);

  diagram_cleanup_autosave(dia);
}

static void
diagram_class_init (DiagramClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = diagram_finalize;
}

/** Creates the raw, uninitialize diagram */
Diagram *
diagram_new() {
  Diagram *dia = g_object_new(DIA_TYPE_DIAGRAM, NULL);
  g_object_ref(dia);
  return dia;
}

GList *
dia_open_diagrams(void)
{
  return open_diagrams;
}

/** Initializes a diagram with standard info and sets it to be called
 * 'filename'.
 */
static void
diagram_init(Diagram *dia, const char *filename)
{
  if (dia->data)
    diagram_data_destroy(dia->data);
  
  dia->data = new_diagram_data(&prefs.new_diagram);
  dia->data->grid.width_x = prefs.grid.x;
  dia->data->grid.width_y = prefs.grid.y;
  dia->data->grid.dynamic = prefs.grid.dynamic;
  dia->data->grid.major_lines = prefs.grid.major_lines;

  if (dia->filename != NULL)
    g_free(dia->filename);
  dia->filename = g_strdup(filename);
  
  dia->unsaved = TRUE;
  dia->mollified = FALSE;
  dia->autosavefilename = NULL;

  if (dia->undo)
    undo_destroy(dia->undo);
  dia->undo = new_undo_stack(dia);

  if (!g_list_find(open_diagrams, dia))
    open_diagrams = g_list_prepend(open_diagrams, dia);

  if (app_is_interactive())
    layer_dialog_update_diagram_list();
}

int
diagram_load_into(Diagram         *diagram,
		  const char      *filename,
		  DiaImportFilter *ifilter)
{
  if (!ifilter)
    ifilter = filter_guess_import_filter(filename);
  if (!ifilter)  /* default to native format */
    ifilter = &dia_import_filter;

  diagram_init(diagram, filename);

  if (ifilter->import_func(filename, diagram->data, ifilter->user_data)) {
    diagram->unsaved = FALSE;
    diagram_set_modified(diagram, FALSE);
    if (app_is_interactive())
      recent_file_history_add(filename);
    diagram_tree_add(diagram_tree(), diagram);
    return TRUE;
  } else
    return FALSE;
}

Diagram *
diagram_load(const char *filename, DiaImportFilter *ifilter)
{
  Diagram *diagram;

  diagram = new_diagram(filename);

  if (!diagram_load_into (diagram, filename, ifilter)) {
    diagram_destroy(diagram);
    diagram = NULL;
  }

  return diagram;
}

Diagram *
new_diagram(const char *filename)  /* Note: filename is copied */
{
  Diagram *dia = diagram_new();

  diagram_init(dia, filename);

  return dia;
}

void
diagram_destroy(Diagram *dia)
{
  g_object_unref(dia);
}

/** Returns true if we consider the diagram modified.
 */
gboolean
diagram_is_modified(Diagram *dia)
{
  return dia->mollified || !undo_is_saved(dia->undo);
}

/** We might just have change the diagrams modified status.
 * This doesn't set the status, but merely updates the display.
 */
void
diagram_modified(Diagram *dia)
{
  GSList *displays;
  displays = dia->displays;
  while (displays != NULL) {
    DDisplay *display = (DDisplay*) displays->data;
    ddisplay_update_statusbar(display);
    displays = g_slist_next(displays);
  }
  if (diagram_is_modified(dia)) dia->autosaved = FALSE;
  /*  diagram_set_modified(dia, TRUE);*/
}

/** Set this diagram explicitly modified.  This should not be called
 * by things that change the undo stack, as those modifications are
 * noticed through changes in the undo stack.
 */
void
diagram_set_modified(Diagram *dia, int modified)
{
  if (dia->mollified != modified)
  {
    dia->mollified = modified;
  }
  diagram_modified(dia);
}

/* ************ Functions that check for menu sensitivity ********* */

/* Suggested optimization: The loops take a lot of the time.  Collapse them
 * into one, have them set flags for the things that have been found true.
 * Harder to maintain.
 */
static gboolean
diagram_selected_any_groups(Diagram *dia) {
  GList *selected;

  for (selected = dia->data->selected;
       selected != NULL; selected = selected->next) {
    DiaObject *obj = (DiaObject*)selected->data;
    if (IS_GROUP(obj)) return TRUE;
  }
  return FALSE;
}

static gboolean
diagram_selected_any_parents(Diagram *dia) {
  GList *selected;

  for (selected = dia->data->selected;
       selected != NULL; selected = selected->next) {
    DiaObject *obj = (DiaObject*)selected->data;
    if (obj->can_parent && obj->children != NULL) return TRUE;
  }
  return FALSE;
}

static gboolean
diagram_selected_any_children(Diagram *dia) {
  GList *selected;

  for (selected = dia->data->selected;
       selected != NULL; selected = selected->next) {
    DiaObject *obj = (DiaObject*)selected->data;
    if (obj->parent != NULL) return TRUE;
  }
  return FALSE;
}

/** This is slightly more complex -- must see if any non-parented objects
 * are within a parenting object.
 */
static gboolean
diagram_selected_can_parent(Diagram *dia) {
  GList *selected;
  GList *parents = NULL;

  for (selected = dia->data->selected;
       selected != NULL; selected = selected->next) {
    DiaObject *obj = (DiaObject*)selected->data;
    if (obj->can_parent) {
      parents = g_list_prepend(parents, obj);
    }
  }
  for (selected = dia->data->selected;
       selected != NULL; selected = selected->next) {
    DiaObject *obj = (DiaObject*)selected->data;
    if (obj->parent == NULL) {
      GList *parent_tmp;
      Rectangle obj_bb = obj->bounding_box;
      for (parent_tmp = parents; parent_tmp != NULL; parent_tmp = parent_tmp->next) {
	DiaObject *p = (DiaObject*)parent_tmp->data;
	if (p == obj) continue;
	if (obj_bb.left > p->bounding_box.left &&
	    obj_bb.right < p->bounding_box.right &&
	    obj_bb.top > p->bounding_box.top &&
	    obj_bb.bottom < p->bounding_box.bottom) {
	  printf("Obj %f, %f x %f, %f inside %f, %f x %f, %f\n",
		 obj_bb.left,
		 obj_bb.top,
		 obj_bb.right,
		 obj_bb.bottom,
		 p->bounding_box.left,
		 p->bounding_box.top,
		 p->bounding_box.right,
		 p->bounding_box.bottom);
	  g_list_free(parents);
	  return TRUE;
	}
      }
    }
  }
  g_list_free(parents);
  return FALSE;
}

/*
  This is the real implementation of the sensitivity update.
  TODO: move it to the DDisplay as it belongs to it IMHO
 */
void 
diagram_update_menu_sensitivity (Diagram *dia, UpdatableMenuItems *items)
{
  /* Edit menu */
  gtk_widget_set_sensitive(GTK_WIDGET(items->copy),
			   dia->data->selected_count > 0);
  gtk_widget_set_sensitive(GTK_WIDGET(items->cut),
			   dia->data->selected_count > 0);
  gtk_widget_set_sensitive(GTK_WIDGET(items->paste),
			   cnp_exist_stored_objects());
  #ifndef GNOME
  gtk_widget_set_sensitive(GTK_WIDGET(items->edit_delete),
			   dia->data->selected_count > 0);
  #endif
  gtk_widget_set_sensitive(GTK_WIDGET(items->copy_text),
			   active_focus() != NULL);
  gtk_widget_set_sensitive(GTK_WIDGET(items->cut_text),
			   active_focus() != NULL);
  gtk_widget_set_sensitive(GTK_WIDGET(items->paste_text),
			   active_focus() != NULL);
  
  /* Objects menu */
  gtk_widget_set_sensitive(GTK_WIDGET(items->send_to_back),
			   dia->data->selected_count > 0);
  gtk_widget_set_sensitive(GTK_WIDGET(items->bring_to_front),
			   dia->data->selected_count > 0);
  gtk_widget_set_sensitive(GTK_WIDGET(items->send_backwards),
			   dia->data->selected_count > 0);
  gtk_widget_set_sensitive(GTK_WIDGET(items->bring_forwards),
			   dia->data->selected_count > 0);
    
  gtk_widget_set_sensitive(GTK_WIDGET(items->parent),
			   diagram_selected_can_parent(dia));
  gtk_widget_set_sensitive(GTK_WIDGET(items->unparent),
			   diagram_selected_any_children(dia));
  gtk_widget_set_sensitive(GTK_WIDGET(items->unparent_children),
			   diagram_selected_any_parents(dia));
  gtk_widget_set_sensitive(GTK_WIDGET(items->group),
			   dia->data->selected_count > 1);
  gtk_widget_set_sensitive(GTK_WIDGET(items->ungroup),
			   diagram_selected_any_groups(dia));
  gtk_widget_set_sensitive(GTK_WIDGET(items->properties),
			   dia->data->selected_count > 0);
  
  /* Objects->Align menu */
  gtk_widget_set_sensitive(GTK_WIDGET(items->align_h_l),
			   dia->data->selected_count > 1);
  gtk_widget_set_sensitive(GTK_WIDGET(items->align_h_c),
			   dia->data->selected_count > 1);
  gtk_widget_set_sensitive(GTK_WIDGET(items->align_h_r),
			   dia->data->selected_count > 1);
  gtk_widget_set_sensitive(GTK_WIDGET(items->align_h_e),
			   dia->data->selected_count > 1);
  gtk_widget_set_sensitive(GTK_WIDGET(items->align_h_a),
			   dia->data->selected_count > 1);
  gtk_widget_set_sensitive(GTK_WIDGET(items->align_v_t),
			   dia->data->selected_count > 1);
  gtk_widget_set_sensitive(GTK_WIDGET(items->align_v_c),
			   dia->data->selected_count > 1);
  gtk_widget_set_sensitive(GTK_WIDGET(items->align_v_b),
			   dia->data->selected_count > 1);
  gtk_widget_set_sensitive(GTK_WIDGET(items->align_v_e),
			   dia->data->selected_count > 1);
  gtk_widget_set_sensitive(GTK_WIDGET(items->align_v_a),
			   dia->data->selected_count > 1);
}
    
  
void diagram_update_menubar_sensitivity(Diagram *dia, UpdatableMenuItems *items)
{
    diagram_update_menu_sensitivity (dia, items);
}


void diagram_update_popupmenu_sensitivity(Diagram *dia)
{
  static int initialized = 0;
  static UpdatableMenuItems items;
  

  char *display = "<Display>";
  
  if (initialized==0) {
      menus_initialize_updatable_items (&items, NULL, display);
      
      initialized = 1;
  }
  
  diagram_update_menu_sensitivity (dia, &items);
}

void
diagram_add_ddisplay(Diagram *dia, DDisplay *ddisp)
{
  dia->displays = g_slist_prepend(dia->displays, ddisp);
  dia->display_count++;
}

void
diagram_remove_ddisplay(Diagram *dia, DDisplay *ddisp)
{
  dia->displays = g_slist_remove(dia->displays, ddisp);
  dia->display_count--;

  if (dia->display_count == 0) {
    if (!app_is_embedded()) {
      /* Don't delete embedded diagram when last view is closed */
      diagram_destroy(dia);
    }
  }
}

void
diagram_add_object(Diagram *dia, DiaObject *obj)
{
  layer_add_object(dia->data->active_layer, obj);

  diagram_modified(dia);

  diagram_tree_add_object(diagram_tree(), dia, obj);
}

void
diagram_add_object_list(Diagram *dia, GList *list)
{
  layer_add_objects(dia->data->active_layer, list);

  diagram_modified(dia);

  diagram_tree_add_objects(diagram_tree(), dia, list);
}

void
diagram_selected_break_external(Diagram *dia)
{
  GList *list;
  GList *connected_list;
  DiaObject *obj;
  DiaObject *other_obj;
  int i,j;

  list = dia->data->selected;
  while (list != NULL) {
    obj = (DiaObject *)list->data;
    
    /* Break connections between this object and objects not selected: */
    for (i=0;i<obj->num_handles;i++) {
      ConnectionPoint *con_point;
      con_point = obj->handles[i]->connected_to;
      
      if ( con_point == NULL )
	break; /* Not connected */
      
      other_obj = con_point->object;
      if (g_list_find(dia->data->selected, other_obj) == NULL) {
	/* other_obj is not selected, break connection */
	Change *change = undo_unconnect(dia, obj, obj->handles[i]);
	(change->apply)(change, dia);
	object_add_updates(obj, dia);
      }
    }
    
    /* Break connections from non selected objects to this object: */
    for (i=0;i<obj->num_connections;i++) {
      connected_list = obj->connections[i]->connected;
      
      while (connected_list != NULL) {
	other_obj = (DiaObject *)connected_list->data;
	
	if (g_list_find(dia->data->selected, other_obj) == NULL) {
	  /* other_obj is not in list, break all connections
	     to obj from other_obj */
	  
	  for (j=0;j<other_obj->num_handles;j++) {
	    ConnectionPoint *con_point;
	    con_point = other_obj->handles[j]->connected_to;

	    if (con_point && (con_point->object == obj)) {
	      Change *change;
	      connected_list = g_list_previous(connected_list);
	      change = undo_unconnect(dia, other_obj,
					      other_obj->handles[j]);
	      (change->apply)(change, dia);
	      if (connected_list == NULL)
		connected_list = obj->connections[i]->connected;
	    }
	  }
	}
	
	connected_list = g_list_next(connected_list);
      }
    }
    diagram_tree_remove_object(diagram_tree(), obj);
    list = g_list_next(list);
  }
}

void
diagram_remove_all_selected(Diagram *diagram, int delete_empty)
{
  object_add_updates_list(diagram->data->selected, diagram);
	
  data_remove_all_selected(diagram->data);
  
  remove_focus();
}

void
diagram_unselect_object(Diagram *diagram, DiaObject *obj)
{
  object_add_updates(obj, diagram);
  
  data_unselect(diagram->data, obj);
  
  if ((active_focus()!=NULL) && (active_focus()->obj == obj)) {
    remove_focus();
  }
}

void
diagram_unselect_objects(Diagram *dia, GList *obj_list)
{
  GList *list;
  DiaObject *obj;

  list = obj_list;
  while (list != NULL) {
    obj = (DiaObject *) list->data;

    if (g_list_find(dia->data->selected, obj) != NULL){
      diagram_unselect_object(dia, obj);
    }

    list = g_list_next(list);
  }
}

void
diagram_select(Diagram *diagram, DiaObject *obj)
{
  data_select(diagram->data, obj);
  object_add_updates(obj, diagram);
}

void
diagram_select_list(Diagram *dia, GList *list)
{

  while (list != NULL) {
    DiaObject *obj = (DiaObject *)list->data;

    diagram_select(dia, obj);

    list = g_list_next(list);
  }
}

int
diagram_is_selected(Diagram *diagram, DiaObject *obj)
{
  return g_list_find(diagram->data->selected, obj) != NULL;
}

void
diagram_redraw_all()
{
  GList *list;
  Diagram *dia;

  list = open_diagrams;

  while (list != NULL) {
    dia = (Diagram *) list->data;

    diagram_add_update_all(dia);
    diagram_flush(dia);

    list = g_list_next(list);
  }
  return;
}

void
diagram_add_update_all(Diagram *dia)
{
  GSList *l;
  DDisplay *ddisp;
  
  l = dia->displays;
  while (l!=NULL) {
    ddisp = (DDisplay *) l->data;

    ddisplay_add_update_all(ddisp);
    
    l = g_slist_next(l);
  }
}

void
diagram_add_update(Diagram *dia, Rectangle *update)
{
  GSList *l;
  DDisplay *ddisp;
  
  l = dia->displays;
  while (l!=NULL) {
    ddisp = (DDisplay *) l->data;

    ddisplay_add_update(ddisp, update);
    
    l = g_slist_next(l);
  }
}

/** Add an update of the given rectangle, but with an additional
 * border around it.  The pixels are added after the rectangle has
 * been converted to pixel coords.
 * Currently used for leaving room for highlighting.
 * */
void
diagram_add_update_with_border(Diagram *dia, Rectangle *update,
			       int pixel_border)
{
  GSList *l;
  DDisplay *ddisp;
  
  l = dia->displays;
  while (l!=NULL) {
    ddisp = (DDisplay *) l->data;

    ddisplay_add_update_with_border(ddisp, update, pixel_border);
    
    l = g_slist_next(l);
  }
}

void
diagram_add_update_pixels(Diagram *dia, Point *point,
			  int pixel_width, int pixel_height)
{
  GSList *l;
  DDisplay *ddisp;

  l = dia->displays;
  while (l!=NULL) {
    ddisp = (DDisplay *) l->data;

    ddisplay_add_update_pixels(ddisp, point, pixel_width, pixel_height);
    
    l = g_slist_next(l);
  }
}

void
diagram_flush(Diagram *dia)
{
  GSList *l;
  DDisplay *ddisp;
  l = dia->displays;
  while (l!=NULL) {
    ddisp = (DDisplay *) l->data;

    ddisplay_flush(ddisp);
    
    l = g_slist_next(l);
  }
  dynobj_refresh_kick();
}

DiaObject *
diagram_find_clicked_object(Diagram *dia, Point *pos,
			    real maxdist)
{
  return layer_find_closest_object_except(dia->data->active_layer, 
					  pos, maxdist, NULL);
}

DiaObject *
diagram_find_clicked_object_except(Diagram *dia, Point *pos,
				   real maxdist, GList *avoid)
{
  return layer_find_closest_object_except(dia->data->active_layer, pos,
					  maxdist, avoid);
}

/*
 * Always returns the last handle in an object that has
 * the closest distance
 */
real
diagram_find_closest_handle(Diagram *dia, Handle **closest,
			    DiaObject **object, Point *pos)
{
  GList *l;
  DiaObject *obj;
  Handle *handle;
  real mindist, dist;
  int i;

  mindist = 1000000.0; /* Realy big value... */
  
  *closest = NULL;
  
  l = dia->data->selected;
  while (l!=NULL) {
    obj = (DiaObject *) l->data;

    for (i=0;i<obj->num_handles;i++) {
      handle = obj->handles[i];
      /* Note: Uses manhattan metric for speed... */
      dist = distance_point_point_manhattan(pos, &handle->pos);
      if (dist<=mindist) { 
	mindist = dist;
	*closest = handle;
	*object = obj;
      }
    }
    
    l = g_list_next(l);
  }

  return mindist;
}

real
diagram_find_closest_connectionpoint(Diagram *dia,
				     ConnectionPoint **closest,
				     Point *pos,
				     DiaObject *notthis)
{
  return layer_find_closest_connectionpoint(dia->data->active_layer,
					    closest, pos, notthis);
}

void
diagram_update_extents(Diagram *dia)
{
  gfloat cur_scale = dia->data->paper.scaling;
  /* anropar update_scrollbars() */

  if (data_update_extents(dia->data)) {
    /* Update scrollbars because extents were changed: */
    GSList *l;
    DDisplay *ddisp;
    
    l = dia->displays;
    while (l!=NULL) {
      ddisp = (DDisplay *) l->data;
      
      ddisplay_update_scrollbars(ddisp);
      
      l = g_slist_next(l);
    }
    if (cur_scale != dia->data->paper.scaling) {
      diagram_add_update_all(dia);
      diagram_flush(dia);
    }
  }  
}

/* Remove connections from obj to objects outside created group. */
static void
strip_connections(DiaObject *obj, GList *not_strip_list, Diagram *dia)
{
  int i;
  Handle *handle;
  Change *change;

  for (i=0;i<obj->num_handles;i++) {
    handle = obj->handles[i];
    if ((handle->connected_to != NULL) &&
	(g_list_find(not_strip_list, handle->connected_to->object)==NULL)) {
      change = undo_unconnect(dia, obj, handle);
      (change->apply)(change, dia);
    }
  }
}


gint diagram_parent_sort_cb(object_extent ** a, object_extent **b)
{
  if ((*a)->extent->left < (*b)->extent->left)
    return 1;
  else if ((*a)->extent->left > (*b)->extent->left)
    return -1;
  else
    if ((*a)->extent->top < (*b)->extent->top)
      return 1;
    else if ((*a)->extent->top > (*b)->extent->top)
      return -1;
    else
      return 0;
}


/* needs faster algorithm -- if we find that parenting is slow.
 * If it works, don't optimize it until it's a hotspot. */
void diagram_parent_selected(Diagram *dia)
{
  GList *list = dia->data->selected;
  int length = g_list_length(list);
  int idx, idx2;
  object_extent *oe;
  gboolean any_parented = FALSE;
  GPtrArray *rects = g_ptr_array_sized_new(length);
  while (list)
  {
    oe = g_new(object_extent, 1);
    oe->object = list->data;
    oe->extent = parent_handle_extents(list->data);
    g_ptr_array_add(rects, oe);
    list = g_list_next(list);
  }
  /* sort all the objects by its left position */
  g_ptr_array_sort(rects, diagram_parent_sort_cb);

  for (idx = 0; idx < length; idx++)
  {
    object_extent *rect = g_ptr_array_index(rects, idx);
    if (rect->object->parent)
      continue;

    for (idx2 = idx + 1; idx2 < length; idx2++)
    {
      object_extent *rect2 = g_ptr_array_index(rects, idx2);
      if (!rect2->object->can_parent)
        continue;

      if (rect->extent->right <= rect2->extent->right
        && rect->extent->bottom <= rect2->extent->bottom)
      {
	Change *change;
	change = undo_parenting(dia, rect2->object, rect->object, TRUE);
	(change->apply)(change, dia);
	any_parented = TRUE;
	/*
        rect->object->parent = rect2->object;
	rect2->object->children = g_list_append(rect2->object->children, rect->object);
	*/
	break;
      }
    }
  }
  g_ptr_array_free(rects, TRUE);
  if (any_parented) {
    diagram_modified(dia);
    diagram_flush(dia);
    undo_set_transactionpoint(dia->undo);
  }
}

/** Remove all selected objects from their parents (if any). */
void diagram_unparent_selected(Diagram *dia)
{
  GList *list;
  DiaObject *obj, *parent;
  Change *change;
  gboolean any_unparented = FALSE;

  for (list = dia->data->selected; list != NULL; list = g_list_next(list))
  {
    obj = (DiaObject *) list->data;
    parent = obj->parent;
    
    if (!parent)
      continue;

    change = undo_parenting(dia, parent, obj, FALSE);
    (change->apply)(change, dia);
    any_unparented = TRUE;
    /*
    parent->children = g_list_remove(parent->children, obj);
    obj->parent = NULL;
    */
  }
  if (any_unparented) {
    diagram_modified(dia);
    diagram_flush(dia);
    undo_set_transactionpoint(dia->undo);
  }
}

/** Remove all children from the selected parents. */
void diagram_unparent_children_selected(Diagram *dia)
{
  GList *list;
  GList *child_ptr;
  DiaObject *obj, *child;
  gboolean any_unparented = FALSE;
  for (list = dia->data->selected; list != NULL; list = g_list_next(list))
  {
    obj = (DiaObject *) list->data;
    if (!obj->can_parent || !obj->children)
      continue;

    any_unparented = TRUE;
    /* Yes, this creates a whole bunch of Changes.  They're lightweight
     * structures, though, and it's easier to assure correctness this
     * way.  If needed, we can make a parent undo with a list of children.
     */
    while (obj->children != NULL) {
      Change *change;
      child = (DiaObject *) obj->children->data;
      change = undo_parenting(dia, obj, child, FALSE);
      /* This will remove one item from the list, so the while terminates. */
      (change->apply)(change, dia);
    }
    /*
    for (child_ptr = obj->children; child_ptr != NULL; 
	 child_ptr = g_list_next(child_ptr))
    {
      Change *change;
      child = (DiaObject *) child_ptr->data;
      change = undo_parenting(dia, obj, child, FALSE);
      (change->apply)(change, dia);
    }
    */
    if (obj->children != NULL)
      printf("Obj still has %d children\n",
	     g_list_length(obj->children));
    /*
    g_list_free(obj->children);
    obj->children = NULL;
    */
  }
  if (any_unparented) {
    diagram_modified(dia);
    diagram_flush(dia);
    undo_set_transactionpoint(dia->undo);
  }
}

void diagram_group_selected(Diagram *dia)
{
  GList *list;
  GList *group_list;
  DiaObject *group;
  DiaObject *obj;
  GList *orig_list;
  Change *change;

  dia->data->selected = parent_list_affected(dia->data->selected);

  orig_list = g_list_copy(dia->data->active_layer->objects);
  
  /* We have to rebuild the selection list so that it is the same
     order as in the Diagram list. */
  group_list = diagram_get_sorted_selected_remove(dia);
  
  list = group_list;
  while (list != NULL) {
    obj = (DiaObject *)list->data;

    /* Remove connections from obj to objects outside created group. */
    /* strip_connections sets up its own undo info. */
    /* The connections aren't reattached by ungroup. */
    strip_connections(obj, dia->data->selected, dia);
    
    /* Have to hide any open properties dialog
     * if it contains some object in cut_list */
    /* Now handled by undo_apply
    properties_hide_if_shown(dia, obj);
    */

    /* Remove focus if active */
    /* Now handled by undo apply
    if ((active_focus()!=NULL) && (active_focus()->obj == obj)) {
      remove_focus();
    }
    */

    /* Now handled by the undo apply
    object_add_updates(obj, dia);
    */
    list = g_list_next(list);
  }

  /* Remove list of selected objects */
  data_remove_all_selected(dia->data);
  
  group = group_create(group_list);
  change = undo_group_objects(dia, group_list, group, orig_list);
  (change->apply)(change, dia);

  /* Select the created group */
  diagram_select(dia, group);
  
  diagram_modified(dia);
  diagram_flush(dia);

  undo_set_transactionpoint(dia->undo);
}

void diagram_ungroup_selected(Diagram *dia)
{
  DiaObject *group;
  GList *group_list;
/*   GList *list; */
  GList *selected;
  int group_index;
  int any_groups = 0;
  
  if (dia->data->selected_count < 1) {
    message_error("Trying to ungroup with no selected objects.");
    return;
  }
  
  selected = dia->data->selected;
  while (selected != NULL) {
    group = (DiaObject *)selected->data;

    if (IS_GROUP(group)) {
      Change *change;

      /* Fix selection */
      diagram_unselect_object(dia, group);

      group_list = group_objects(group);
      diagram_select_list(dia, group_list);
      /* Now handled by undo apply */      
      /*list = group_list;
      while (list != NULL) {
	DiaObject *obj = (DiaObject *)list->data;
	object_add_updates(obj, dia);
	list = g_list_next(list);
      }
      */

      /* Now handled by undo apply
      layer_replace_object_with_list(dia->data->active_layer,
				     group, g_list_copy(group_list));
      */

      group_index = layer_object_index(dia->data->active_layer, group);
    
      change = undo_ungroup_objects(dia, group_list, group, group_index);
      (change->apply)(change, dia);
      /* Now handled by undo apply 
      diagram_tree_add_objects(diagram_tree(), dia, group_list);
      diagram_tree_remove_object(diagram_tree(), group);
      properties_hide_if_shown(dia, group);
      */

      any_groups = 1;
    }
    selected = g_list_next(selected);
  }
  
  if (any_groups) {
    diagram_modified(dia);
    diagram_flush(dia);
    undo_set_transactionpoint(dia->undo);
  }
}

GList *
diagram_get_sorted_selected(Diagram *dia)
{
  return data_get_sorted_selected(dia->data);
}

/** Remove the currently selected objects from the diagram's object list.
 * Returns a newly created list of the selected objects, in order.
 */
GList *
diagram_get_sorted_selected_remove(Diagram *dia)
{
  diagram_modified(dia);

  return data_get_sorted_selected_remove(dia->data);
}

void
diagram_place_under_selected(Diagram *dia)
{
  GList *sorted_list;
  GList *orig_list;

  if (dia->data->selected_count == 0)
    return;

  orig_list = g_list_copy(dia->data->active_layer->objects);

  sorted_list = diagram_get_sorted_selected_remove(dia);
  object_add_updates_list(sorted_list, dia);
  layer_add_objects_first(dia->data->active_layer, sorted_list);
  
  undo_reorder_objects(dia, g_list_copy(sorted_list), orig_list);

  diagram_modified(dia);
  diagram_flush(dia);
  undo_set_transactionpoint(dia->undo);
}

void
diagram_place_over_selected(Diagram *dia)
{
  GList *sorted_list;
  GList *orig_list;

  if (dia->data->selected_count == 0)
    return;

  orig_list = g_list_copy(dia->data->active_layer->objects);
  
  sorted_list = diagram_get_sorted_selected_remove(dia);
  object_add_updates_list(sorted_list, dia);
  layer_add_objects(dia->data->active_layer, sorted_list);
  
  undo_reorder_objects(dia, g_list_copy(sorted_list), orig_list);

  diagram_modified(dia);
  diagram_flush(dia);
  undo_set_transactionpoint(dia->undo);
}

void
diagram_place_up_selected(Diagram *dia)
{
  GList *sorted_list;
  GList *orig_list;
  GList *tmp, *stmp;
  GList *new_list = NULL;

  if (dia->data->selected_count == 0)
    return;

  orig_list = g_list_copy(dia->data->active_layer->objects);
  
  sorted_list = diagram_get_sorted_selected(dia);
  object_add_updates_list(orig_list, dia);

  new_list = g_list_copy(orig_list);
  stmp = g_list_last(sorted_list);

  for (tmp = g_list_last(new_list);
       tmp != NULL;
       tmp = g_list_previous(tmp)) {
    if (stmp == NULL) break;
    if (tmp->prev == NULL) break;
    if (tmp->data == stmp->data) {
      stmp = g_list_previous(stmp);
    } else if (tmp->prev->data == stmp->data) {
      void *swap = tmp->data;
      tmp->data = tmp->prev->data;
      tmp->prev->data = swap;
      stmp = g_list_previous(stmp);
    }
  }

  layer_set_object_list(dia->data->active_layer, new_list);

  undo_reorder_objects(dia, g_list_copy(sorted_list), orig_list);

  diagram_modified(dia);
  diagram_flush(dia);
  undo_set_transactionpoint(dia->undo);
}

void
diagram_place_down_selected(Diagram *dia)
{
  GList *sorted_list;
  GList *orig_list;
  GList *tmp, *stmp;
  GList *new_list = NULL;

  if (dia->data->selected_count == 0)
    return;

  orig_list = g_list_copy(dia->data->active_layer->objects);
  
  sorted_list = diagram_get_sorted_selected(dia);
  object_add_updates_list(orig_list, dia);

  /* Sanity check */
  g_assert(dia->data->selected_count == g_list_length(sorted_list));

  new_list = g_list_copy(orig_list);
  tmp = new_list;
  stmp = sorted_list;

  for (tmp = new_list; tmp != NULL; tmp = g_list_next(tmp)) {
    if (stmp == NULL) break;
    if (tmp->next == NULL) break;
    if (tmp->data == stmp->data) {
      /* This just takes care of any starting matches */
      stmp = g_list_next(stmp);
    } else if (tmp->next->data == stmp->data) {
      /* This flips the non-selected element forwards, ala bubblesort */
      void *swap = tmp->data;
      tmp->data = tmp->next->data;
      tmp->next->data = swap;
      stmp = g_list_next(stmp);
    }
  }

  layer_set_object_list(dia->data->active_layer, new_list);

  undo_reorder_objects(dia, g_list_copy(sorted_list), orig_list);

  diagram_modified(dia);
  diagram_flush(dia);
  undo_set_transactionpoint(dia->undo);
}

void
diagram_set_filename(Diagram *dia, char *filename)
{
  GSList *l;
  DDisplay *ddisp;
  char *title;
  
  g_free(dia->filename);
  dia->filename = g_strdup(filename);

  title = diagram_get_name(dia);

  l = dia->displays;
  while (l!=NULL) {
    ddisp = (DDisplay *) l->data;

    ddisplay_set_title(ddisp, title);
    
    l = g_slist_next(l);
  }

  g_free(title);

  layer_dialog_update_diagram_list();
  recent_file_history_add((const char *)filename);

  diagram_tree_update_name(diagram_tree(), dia);
}

/** Returns a string with a 'sensible' (human-readable) name for the
 * diagram.  The string should be freed after use.
 * This name may or may not be the same as the filename.
 */
gchar *
diagram_get_name(Diagram *dia)
{
  gchar *title = strrchr(dia->filename, G_DIR_SEPARATOR);
  if (title==NULL) {
    title = dia->filename;
  } else {
    title++;
  }
  
  return g_strdup(title);
}

int diagram_modified_exists(void)
{
  GList *list;
  Diagram *dia;

  list = open_diagrams;

  while (list != NULL) {
    dia = (Diagram *) list->data;

    if (diagram_is_modified(dia))
      return TRUE;

    list = g_list_next(list);
  }
  return FALSE;
}

void diagram_object_modified(Diagram *dia, DiaObject *object)
{
  diagram_tree_update_object(diagram_tree(), dia, object);
}
