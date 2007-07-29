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
#include "object.h"
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
#include "textedit.h"
#include "lib/diamarshal.h"
#include "parent.h"

static GList *open_diagrams = NULL;

struct _ObjectExtent
{
  DiaObject *object;
  Rectangle extent;
};

typedef struct _ObjectExtent ObjectExtent;

static gint diagram_parent_sort_cb(gconstpointer a, gconstpointer b);


static void diagram_class_init (DiagramClass *klass);
static gboolean diagram_init(Diagram *obj, const char *filename);
static void diagram_update_for_filename(Diagram *dia);

enum {
  SELECTION_CHANGED,
  REMOVED,
  LAST_SIGNAL
};

static guint diagram_signals[LAST_SIGNAL] = { 0, };
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

      object_type = g_type_register_static (DIA_TYPE_DIAGRAM_DATA,
                                            "Diagram",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void
diagram_finalize(GObject *object) 
{
  Diagram *dia = DIA_DIAGRAM(object);

  assert(dia->displays==NULL);
  
  open_diagrams = g_list_remove(open_diagrams, dia);
  layer_dialog_update_diagram_list();

  if (dia->undo)
    undo_destroy(dia->undo);
  dia->undo = NULL;
  
  diagram_tree_remove(diagram_tree(), dia);

  diagram_cleanup_autosave(dia);

  if (dia->filename)
    g_free(dia->filename);
  dia->filename = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
_diagram_removed (Diagram* dia)
{
}

static void
_diagram_selection_changed (Diagram* dia, int n)
{
}

static void
diagram_class_init (DiagramClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  diagram_signals[REMOVED] =
    g_signal_new ("removed",
	          G_TYPE_FROM_CLASS (klass),
	          G_SIGNAL_RUN_FIRST,
	          G_STRUCT_OFFSET (DiagramClass, removed),
	          NULL, NULL,
	          dia_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  diagram_signals[SELECTION_CHANGED] =
    g_signal_new ("selection_changed",
	          G_TYPE_FROM_CLASS (klass),
	          G_SIGNAL_RUN_FIRST,
	          G_STRUCT_OFFSET (DiagramClass, selection_changed),
	          NULL, NULL,
	          dia_marshal_VOID__INT,
		  G_TYPE_NONE, 1,
		  G_TYPE_INT);

  klass->removed = _diagram_removed;
  klass->selection_changed = _diagram_selection_changed;

  object_class->finalize = diagram_finalize;
}

GList *
dia_open_diagrams(void)
{
  return open_diagrams;
}

/** Initializes a diagram with standard info and sets it to be called
 * 'filename'.  
 * Returns TRUE if everything went ok, FALSE otherwise.
 * Will return FALSE if filename is not a legal string in the current
 * encoding.
 */
static gboolean
diagram_init(Diagram *dia, const char *filename)
{
  gchar *newfilename = NULL;
  GError *error = NULL;
 
  dia->data = &dia->parent_instance; /* compatibility */

  dia->pagebreak_color = prefs.new_diagram.pagebreak_color;

  dia->grid.width_x = prefs.grid.x;
  dia->grid.width_y = prefs.grid.y;
  dia->grid.width_w = prefs.grid.w;
  dia->grid.hex_size = 1.0;
  dia->grid.colour = prefs.new_diagram.grid_color;
  dia->grid.hex = prefs.grid.hex;
  dia->grid.visible_x = 1;
  dia->grid.visible_y = 1;
  dia->grid.dynamic = prefs.grid.dynamic;
  dia->grid.major_lines = prefs.grid.major_lines;

  dia->guides.nhguides = 0;
  dia->guides.hguides = NULL;
  dia->guides.nvguides = 0;
  dia->guides.vguides = NULL;

  if (dia->filename != NULL)
    g_free(dia->filename);
  /* Make sure the filename is absolute */
  if (!g_path_is_absolute(filename)) {
    gchar *pwd = g_get_current_dir();

    newfilename = g_build_filename(pwd, filename, NULL);
    g_free(pwd);
    filename = newfilename;
  }
  /* All Diagram functions assumes filename in filesystem encoding */
  
  dia->filename = g_filename_to_utf8(filename, -1, NULL, NULL, &error);
  if (error != NULL) {
    message_error(_("Couldn't convert filename '%s' to UTF-8: %s\n"),
		  dia_message_filename(filename), error->message);
    g_error_free(error);
    dia->filename = g_strdup(_("Error"));
    return FALSE;
  }

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

  g_free(newfilename);
  return TRUE;
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

  if (ifilter->import_func(filename, diagram->data, ifilter->user_data)) {
    diagram_set_modified(diagram, TRUE);
    return TRUE;
  } else
    return FALSE;
}

Diagram *
diagram_load(const char *filename, DiaImportFilter *ifilter)
{
  Diagram *diagram = NULL;
  GList *diagrams;
  gboolean was_default = FALSE;

  for (diagrams = open_diagrams; diagrams != NULL; diagrams = g_list_next(diagrams)) {
    Diagram *old_diagram = (Diagram*)diagrams->data;
    if (old_diagram->is_default) {
      diagram = old_diagram;
      was_default = TRUE;
      break;
    }	
  }

  /* TODO: Make diagram not be initialized twice */
  if (diagram == NULL) {
    diagram = new_diagram(filename);
  }
  if (diagram == NULL) return NULL;

  if (   !diagram_init(diagram, filename)
      || !diagram_load_into (diagram, filename, ifilter)) {
    diagram_destroy(diagram);
    diagram = NULL;
  } else {
    diagram->unsaved = FALSE;
    diagram_set_modified(diagram, FALSE);
    if (app_is_interactive())
      recent_file_history_add(filename);
    diagram_tree_add(diagram_tree(), diagram);
  }
  
  if (diagram != NULL && was_default) {
    diagram_update_for_filename(diagram);
    diagram->is_default = FALSE;
    if ( g_slist_length(diagram->displays) == 1 )
      display_set_active (diagram->displays->data);
  }
  
  return diagram;
}

/** Create a new diagram with the given filename.
 * If the diagram could not be created, e.g. because the filename is not
 * a legal string in the current encoding, return NULL.
 */
Diagram *
new_diagram(const char *filename)  /* Note: filename is copied */
{
  Diagram *dia = g_object_new(DIA_TYPE_DIAGRAM, NULL);

  if (diagram_init(dia, filename)) {
    return dia;
  } else {
    g_object_unref(dia);
    return NULL;
  }
}

void
diagram_destroy(Diagram *dia)
{
  g_signal_emit (dia, diagram_signals[REMOVED], 0);
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
  if (diagram_is_modified(dia)) {
    dia->autosaved = FALSE;
    dia->is_default = FALSE;
  }
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
    if (object_flags_set(obj, DIA_OBJECT_CAN_PARENT) && obj->children != NULL) 
      return TRUE;
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
    if (object_flags_set(obj, DIA_OBJECT_CAN_PARENT)) {
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
          /*
	  printf("Obj %f, %f x %f, %f inside %f, %f x %f, %f\n",
		 obj_bb.left,
		 obj_bb.top,
		 obj_bb.right,
		 obj_bb.bottom,
		 p->bounding_box.left,
		 p->bounding_box.top,
		 p->bounding_box.right,
		 p->bounding_box.bottom);
	  */
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
  gint selected_count = g_list_length (dia->data->selected);
  /* Edit menu */
  gtk_action_set_sensitive (items->copy, selected_count > 0);
  gtk_action_set_sensitive (items->cut, selected_count > 0);
  gtk_action_set_sensitive (items->paste, cnp_exist_stored_objects());
  gtk_action_set_sensitive (items->edit_delete, selected_count > 0);
  gtk_action_set_sensitive (items->edit_duplicate, selected_count > 0);

  gtk_action_set_sensitive (items->copy_text, active_focus() != NULL);
  gtk_action_set_sensitive (items->cut_text, active_focus() != NULL);
  gtk_action_set_sensitive (items->paste_text, active_focus() != NULL);
  
  /* Objects menu */
  gtk_action_set_sensitive (items->send_to_back, selected_count > 0);
  gtk_action_set_sensitive (items->bring_to_front, selected_count > 0);
  gtk_action_set_sensitive (items->send_backwards, selected_count > 0);
  gtk_action_set_sensitive (items->bring_forwards, selected_count > 0);
    
  gtk_action_set_sensitive (items->parent, diagram_selected_can_parent (dia));
  gtk_action_set_sensitive (items->unparent, 
			    diagram_selected_any_children (dia));
  gtk_action_set_sensitive (items->unparent_children,
			    diagram_selected_any_parents (dia));
  gtk_action_set_sensitive (items->group, selected_count > 1);
  gtk_action_set_sensitive (items->ungroup, diagram_selected_any_groups (dia));
  gtk_action_set_sensitive (items->properties, selected_count > 0);
  
  /* Objects->Align menu */
  gtk_action_set_sensitive (items->align_h_l, selected_count > 1);
  gtk_action_set_sensitive (items->align_h_c, selected_count > 1);
  gtk_action_set_sensitive (items->align_h_r, selected_count > 1);
  gtk_action_set_sensitive (items->align_h_e, selected_count > 1);
  gtk_action_set_sensitive (items->align_h_a, selected_count > 1);
  gtk_action_set_sensitive (items->align_v_t, selected_count > 1);
  gtk_action_set_sensitive (items->align_v_c, selected_count > 1);
  gtk_action_set_sensitive (items->align_v_b, selected_count > 1);
  gtk_action_set_sensitive (items->align_v_e, selected_count > 1);
  gtk_action_set_sensitive (items->align_v_a, selected_count > 1);
}
    
  
void diagram_update_menubar_sensitivity(Diagram *dia, UpdatableMenuItems *items)
{
    diagram_update_menu_sensitivity (dia, items);
}


void diagram_update_popupmenu_sensitivity(Diagram *dia)
{
  static int initialized = 0;
  static UpdatableMenuItems items;
 
  if (initialized==0) {
      menus_initialize_updatable_items (&items, NULL);      
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
  textedit_remove_focus_all(diagram);
  data_remove_all_selected(diagram->data);
  g_signal_emit (diagram, diagram_signals[SELECTION_CHANGED], 0, g_list_length (diagram->data->selected));
}

void
diagram_unselect_object(Diagram *diagram, DiaObject *obj)
{
  object_add_updates(obj, diagram);
  textedit_remove_focus(obj, diagram);
  data_unselect(DIA_DIAGRAM_DATA(diagram), obj);
  g_signal_emit (diagram, diagram_signals[SELECTION_CHANGED], 0,
		 g_list_length (DIA_DIAGRAM_DATA(diagram)->selected));
}

void
diagram_unselect_objects(Diagram *dia, GList *obj_list)
{
  GList *list;
  DiaObject *obj;

  /* otherwise we would signal objects step by step */
  g_signal_handlers_block_by_func (dia, _diagram_selection_changed, NULL);
  list = obj_list;
  while (list != NULL) {
    obj = (DiaObject *) list->data;

    if (g_list_find(dia->data->selected, obj) != NULL){
      diagram_unselect_object(dia, obj);
    }

    list = g_list_next(list);
  }
  g_signal_handlers_unblock_by_func (dia, _diagram_selection_changed, NULL);
  g_signal_emit (dia, diagram_signals[SELECTION_CHANGED], 0, g_list_length (dia->data->selected));
}

/** Make a single object selected.
 * Note that an object inside a closed group cannot be made selected, nor
 * can an object in a non-active layer.
 * @param diagram The diagram that the object belongs to (sorta redundant now)
 * @param obj The object that should be made selected.
 */
void
diagram_select(Diagram *diagram, DiaObject *obj)
{
  if (dia_object_is_selectable(obj)) {
    data_select(diagram->data, obj);
    obj->ops->selectf(obj, NULL, NULL);
    object_add_updates(obj, diagram);
    g_signal_emit (diagram, diagram_signals[SELECTION_CHANGED], 0,
		   g_list_length (diagram->data->selected));
  }
}

void
diagram_select_list(Diagram *dia, GList *list)
{
  g_return_if_fail (dia && list);
  /* otherwise we would signal objects step by step */
  g_signal_handlers_block_by_func (dia, _diagram_selection_changed, NULL);
  while (list != NULL) {
    DiaObject *obj = (DiaObject *)list->data;

    diagram_select(dia, obj);

    list = g_list_next(list);
  }
  if (active_focus() == NULL) {
    textedit_activate_first(ddisplay_active());
  }
  g_signal_handlers_unblock_by_func (dia, _diagram_selection_changed, NULL);
  g_signal_emit (dia, diagram_signals[SELECTION_CHANGED], 0, g_list_length (dia->data->selected));
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
  real dist = 100000000.0;
  int i;
  for (i=0;i<dia->data->layers->len;i++) {
    Layer *layer = (Layer*)g_ptr_array_index(dia->data->layers, i);
    ConnectionPoint *this_cp;
    real this_dist;
    if (layer->connectable) {
      this_dist = layer_find_closest_connectionpoint(layer,
						     &this_cp, pos, notthis);
      if (this_dist < dist) {
	dist = this_dist;
	*closest = this_cp;
      }
    }
  }
  return dist;
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


/* GCompareFunc */
static gint 
diagram_parent_sort_cb(gconstpointer _a, gconstpointer _b)
{
  ObjectExtent **a = (ObjectExtent **)_a;
  ObjectExtent **b = (ObjectExtent **)_b;

  if ((*a)->extent.left < (*b)->extent.left)
    return 1;
  else if ((*a)->extent.left > (*b)->extent.left)
    return -1;
  else
    if ((*a)->extent.top < (*b)->extent.top)
      return 1;
    else if ((*a)->extent.top > (*b)->extent.top)
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
  ObjectExtent *oe;
  gboolean any_parented = FALSE;
  GPtrArray *extents = g_ptr_array_sized_new(length);
  while (list)
  {
    oe = g_new(ObjectExtent, 1);
    oe->object = list->data;
    parent_handle_extents(list->data, &oe->extent);
    g_ptr_array_add(extents, oe);
    list = g_list_next(list);
  }
  /* sort all the objects by their left position */
  g_ptr_array_sort(extents, diagram_parent_sort_cb);

  for (idx = 0; idx < length; idx++)
  {
    ObjectExtent *oe1 = g_ptr_array_index(extents, idx);
    if (oe1->object->parent)
      continue;

    for (idx2 = idx + 1; idx2 < length; idx2++)
    {
      ObjectExtent *oe2 = g_ptr_array_index(extents, idx2);
      if (!object_flags_set(oe2->object, DIA_OBJECT_CAN_PARENT))
        continue;

      if (oe1->extent.right <= oe2->extent.right
        && oe1->extent.bottom <= oe2->extent.bottom)
      {
	Change *change;
	change = undo_parenting(dia, oe2->object, oe1->object, TRUE);
	(change->apply)(change, dia);
	any_parented = TRUE;
	/*
        oe1->object->parent = oe2->object;
	oe2->object->children = g_list_append(oe2->object->children, oe1->object);
	*/
	break;
      }
    }
  }
  g_ptr_array_free(extents, TRUE);
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
  DiaObject *obj, *child;
  gboolean any_unparented = FALSE;
  for (list = dia->data->selected; list != NULL; list = g_list_next(list))
  {
    obj = (DiaObject *) list->data;
    if (!object_flags_set(obj, DIA_OBJECT_CAN_PARENT) || !obj->children)
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
    if (obj->children != NULL)
      printf("Obj still has %d children\n",
	     g_list_length(obj->children));
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

  if (g_list_length(dia->data->selected) < 1) {
    message_error(_("Trying to group with no selected objects."));
    return;
  }
  
#ifdef USE_NEWGROUP
  list = dia->data->selected;
  current_parent = ((DiaObject *) list->data)->parent;
  while (list != NULL) {
    obj = (DiaObject *)list->data;
    if (obj->parent != current_parent) {
      message_warning(_("You cannot group objects that belong to different groups or have different parents"));
      return;
    }
  }

  group = ...

  list = dia->data->selected;
  while (list != NULL) {
    obj = (DiaObject *)list->data;
    
  }
#else
#if 0
  /* the following is wrong as it screws up the selected list, see bug #153525
     * I just don't get what was originally intented so please speak up if you know  --hb
     */    
  dia->data->selected = parent_list_affected(dia->data->selected);
#endif
    
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
    
    list = g_list_next(list);
  }

  /* Remove list of selected objects */
  textedit_remove_focus_all(dia);
  data_remove_all_selected(dia->data);
  
  group = group_create(group_list);
  change = undo_group_objects(dia, group_list, group, orig_list);
  (change->apply)(change, dia);
#endif

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
  GList *selected, *selection_copy;
  int group_index;
  int any_groups = 0;
  
  if (g_list_length(dia->data->selected) < 1) {
    message_error("Trying to ungroup with no selected objects.");
    return;
  }
  
  selection_copy = g_list_copy(dia->data->selected);
  selected = selection_copy;
  while (selected != NULL) {
    group = (DiaObject *)selected->data;

    if (IS_GROUP(group)) {
      Change *change;

      /* Fix selection */
      diagram_unselect_object(dia, group);

      group_list = group_objects(group);
      diagram_select_list(dia, group_list);

      group_index = layer_object_index(dia->data->active_layer, group);
    
      change = undo_ungroup_objects(dia, group_list, group, group_index);
      (change->apply)(change, dia);

      any_groups = 1;
    }
    selected = g_list_next(selected);
  }
  g_list_free(selection_copy);
  
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

  if (g_list_length (dia->data->selected) == 0)
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

  if (g_list_length (dia->data->selected) == 0)
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

  if (g_list_length (dia->data->selected) == 0)
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

  if (g_list_length (dia->data->selected) == 0)
    return;

  orig_list = g_list_copy(dia->data->active_layer->objects);
  
  sorted_list = diagram_get_sorted_selected(dia);
  object_add_updates_list(orig_list, dia);

  /* Sanity check */
  g_assert(g_list_length (dia->data->selected) == g_list_length(sorted_list));

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
diagram_set_filename(Diagram *dia, const char *filename)
{
  g_free(dia->filename);
  dia->filename = g_filename_to_utf8(filename, -1, NULL, NULL, NULL);
  
  diagram_update_for_filename(dia);
}

/** Update the various areas that require updating when changing filename
 * This will ensure that all places that use the filename are updated:
 * Window titles, layer dialog, recent files, diagram tree.
 * @param dia The diagram whose filename has changed.
 */
static void
diagram_update_for_filename(Diagram *dia)
{
  GSList *l;
  DDisplay *ddisp;
  char *title;
  char *filename = dia->filename;

  title = diagram_get_name(dia);

  l = dia->displays;
  while (l!=NULL) {
    ddisp = (DDisplay *) l->data;

    ddisplay_set_title(ddisp, title);
    
    l = g_slist_next(l);
  }

  g_free(title);

  layer_dialog_update_diagram_list();
  recent_file_history_add(filename);

  diagram_tree_update_name(diagram_tree(), dia);
}

/** Returns a string with a 'sensible' (human-readable) name for the
 * diagram.  The string should be freed after use.
 * This name may or may not be the same as the filename.
 */
gchar *
diagram_get_name(Diagram *dia)
{
  return g_path_get_basename(dia->filename);
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
