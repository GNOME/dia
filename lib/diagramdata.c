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

/** \file diagramdata.c  This file defines the DiagramData object, which holds (mostly) saveable
 * data global to a diagram.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "intl.h"
#include "diagramdata.h"
#include "diarenderer.h"
#include "paper.h"
#include "persistence.h"

#include "dynamic_obj.h"
#include "diamarshal.h"


static const Rectangle invalid_extents = { -1.0,-1.0,-1.0,-1.0 };

static void diagram_data_class_init (DiagramDataClass *klass);
static void diagram_data_init (DiagramData *object);

enum {
  OBJECT_ADD,
  OBJECT_REMOVE,
  SELECTION_CHANGED,
  LAST_SIGNAL
};

typedef struct {
  DiaObject *obj;
  DiaHighlightType type;
} ObjectHighlight;

static guint diagram_data_signals[LAST_SIGNAL] = { 0, };

static gpointer parent_class = NULL;

/** Get the type object for the DiagramData class.
 */
GType
diagram_data_get_type(void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (DiagramDataClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) diagram_data_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (DiagramData),
        0,              /* n_preallocs */
	(GInstanceInitFunc)diagram_data_init /* init */
      };

      object_type = g_type_register_static (G_TYPE_OBJECT,
                                            "DiagramData",
                                            &object_info, 0);
    }
  
  return object_type;
}

/* signal default handlers */
static void
_diagram_data_object_add (DiagramData* dia,Layer* layer,DiaObject* obj)
{
}

static void
_diagram_data_object_remove (DiagramData* dia,Layer* layer,DiaObject* obj)
{
}

static void
_diagram_data_selection_changed (DiagramData* dia, int n)
{
}

/** Initialize a new diagram data object.
 * @param data A diagram data object to initialize.
 */
static void
diagram_data_init(DiagramData *data)
{
  Color* color = persistence_register_color ("new_diagram_bgcolour", &color_white);
  gboolean compress = persistence_register_boolean ("compress_save", TRUE);
  Layer *first_layer;

  data->extents.left = 0.0; 
  data->extents.right = 10.0; 
  data->extents.top = 0.0; 
  data->extents.bottom = 10.0; 
 
  data->bg_color = *color;

  get_paper_info (&data->paper, -1, NULL);

  first_layer = new_layer(g_strdup(_("Background")),data);
  
  data->layers = g_ptr_array_new ();
  g_ptr_array_add (data->layers, first_layer);
  data->active_layer = first_layer;

  data->selected_count_private = 0;
  data->selected = NULL;

  data->highlighted = NULL;
  
  data->is_compressed = compress; /* Overridden by doc */

  data->text_edits = NULL;
  data->active_text_edit = NULL;
}


/** Deallocate memory owned by a DiagramData object.
 * @param object A DiagramData object to finalize.
 */
static void
diagram_data_finalize(GObject *object)
{
  DiagramData *data = DIA_DIAGRAM_DATA(object);

  guint i;

  g_free(data->paper.name);

  for (i=0;i<data->layers->len;i++) {
    layer_destroy(g_ptr_array_index(data->layers, i));
  }
  g_ptr_array_free(data->layers, TRUE);
  data->active_layer = NULL;

  g_list_free(data->selected);
  data->selected = NULL; /* for safety */
  data->selected_count_private = 0;
}


/*!
 * \brief Create a copy of the whole diagram data
 *
 * This is kind of a deep copy, everything not immutable gets duplicated.
 * Still this is supposed to be relatively fast, because the most expensive
 * objects have immutuable properties, namely the pixbufs in DiaImage are
 * only referenced.
 *
 * \memberof _DiagramData
 */
DiagramData *
diagram_data_clone (DiagramData *data)
{
  DiagramData *clone;
  guint i;
  
  clone = g_object_new (DIA_TYPE_DIAGRAM_DATA, NULL);
  
  clone->extents = data->extents;
  clone->bg_color = data->bg_color;
  clone->paper = data->paper; /* so ugly */
  clone->paper.name = g_strdup (data->paper.name);
  clone->is_compressed = data->is_compressed;
  
  layer_destroy(g_ptr_array_index(clone->layers, 0));
  g_ptr_array_remove(clone->layers, clone->active_layer);

  for (i=0; i < data->layers->len; ++i) {
    Layer *src_layer = g_ptr_array_index(data->layers, i);
    Layer *dest_layer = new_layer (layer_get_name (src_layer), clone);
  
    /* copy layer, init the active one */
    dest_layer->extents = src_layer->extents;
    dest_layer->objects = object_copy_list (src_layer->objects);
    dest_layer->visible = src_layer->visible;
    dest_layer->connectable = src_layer->connectable;
    g_ptr_array_add (clone->layers, dest_layer);
    if (src_layer == data->active_layer)
      clone->active_layer = dest_layer;
      
    /* the rest should be initialized by construction */
  }

  return clone;  
}

/*!
 * \brief Create a new diagram data object only containing the selected objects
 * \memberof _DiagramData
 */
DiagramData *
diagram_data_clone_selected (DiagramData *data)
{
  DiagramData *clone;
  Layer *dest_layer;
  GList *sorted;

  clone = g_object_new (DIA_TYPE_DIAGRAM_DATA, NULL);
  
  clone->extents = data->extents;
  clone->bg_color = data->bg_color;
  clone->paper = data->paper; /* so ugly */
  clone->paper.name = g_strdup (data->paper.name);
  clone->is_compressed = data->is_compressed;

  /* the selection - if any - can only be on the active layer */
  dest_layer = g_ptr_array_index(clone->layers, 0);
  g_free (dest_layer->name);
  dest_layer->name = layer_get_name (data->active_layer);

  sorted = data_get_sorted_selected (data);
  dest_layer->objects = object_copy_list (sorted);
  g_list_free (sorted);
  dest_layer->visible = data->active_layer->visible;
  dest_layer->connectable = data->active_layer->connectable;
  
  data_update_extents (clone);
  
  return clone;
}

/** Initialize the DiagramData class data.
 * @param klass The class object to initialize functions for.
 */
static void
diagram_data_class_init(DiagramDataClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  diagram_data_signals[OBJECT_ADD] =
    g_signal_new ("object_add",
              G_TYPE_FROM_CLASS (klass),
              G_SIGNAL_RUN_FIRST,
              G_STRUCT_OFFSET (DiagramDataClass, object_add),
              NULL, NULL,
              dia_marshal_VOID__POINTER_POINTER,
              G_TYPE_NONE, 
              2,
              G_TYPE_POINTER,
              G_TYPE_POINTER);
              
  diagram_data_signals[OBJECT_REMOVE] =
    g_signal_new ("object_remove",
              G_TYPE_FROM_CLASS (klass),
              G_SIGNAL_RUN_FIRST,
              G_STRUCT_OFFSET (DiagramDataClass, object_remove),
              NULL, NULL,
              dia_marshal_VOID__POINTER_POINTER,
              G_TYPE_NONE, 
              2,
              G_TYPE_POINTER,
              G_TYPE_POINTER);

  diagram_data_signals[SELECTION_CHANGED] =
    g_signal_new ("selection_changed",
	          G_TYPE_FROM_CLASS (klass),
	          G_SIGNAL_RUN_FIRST,
	          G_STRUCT_OFFSET (DiagramDataClass, selection_changed),
	          NULL, NULL,
	          dia_marshal_VOID__INT,
		  G_TYPE_NONE, 1,
		  G_TYPE_INT);

  object_class->finalize = diagram_data_finalize;
  klass->object_add = _diagram_data_object_add;
  klass->object_remove = _diagram_data_object_remove;
  klass->selection_changed = _diagram_data_selection_changed;
}

/*!
 * \brief Raise a layer up one in a diagram.
 * @param data The diagram that the layer belongs to.
 * @param layer The layer to raise.
 * \memberof _DiagramData
 */
void
data_raise_layer(DiagramData *data, Layer *layer)
{
  guint i;
  guint layer_nr = 0;
  Layer *tmp;
  
  for (i=0;i<data->layers->len;i++) {
    if (g_ptr_array_index(data->layers, i)==layer)
      layer_nr = i;
  }

  if (layer_nr < data->layers->len-1) {
    tmp = g_ptr_array_index(data->layers, layer_nr+1);
    g_ptr_array_index(data->layers, layer_nr+1) =
      g_ptr_array_index(data->layers, layer_nr);
    g_ptr_array_index(data->layers, layer_nr) = tmp;
  }
}

/*!
 * \brief Lower a layer by one in a diagram.
 * @param data The diagram that the layer belongs to.
 * @param layer The layer to lower.
 * \memberof _DiagramData
 */
void
data_lower_layer(DiagramData *data, Layer *layer)
{
  guint i;
  int layer_nr = -1;
  Layer *tmp;
  
  for (i=0;i<data->layers->len;i++) {
    if (g_ptr_array_index(data->layers, i)==layer)
      layer_nr = i;
  }

  g_assert(layer_nr>=0);

  if (layer_nr > 0) {
    tmp = g_ptr_array_index(data->layers, layer_nr-1);
    g_ptr_array_index(data->layers, layer_nr-1) =
      g_ptr_array_index(data->layers, layer_nr);
    g_ptr_array_index(data->layers, layer_nr) = tmp;
  }
}

/*!
 * \brief Add a layer object to a diagram.
 * @param data The diagram to add the layer to.
 * @param layer The layer to add.
 * \memberof _DiagramData
 */
void
data_add_layer(DiagramData *data, Layer *layer)
{
  g_ptr_array_add(data->layers, layer);
  layer->parent_diagram = data;
  data_emit (data, layer, NULL, "object_add");
  layer_update_extents(layer);
  data_update_extents(data);
}

/*!
 * \brief Add a layer at a defined postion in the stack
 *
 * If the given pos is out of range the layer is added at the top of the stack.
 *
 * @param data the diagram
 * @param layer layer to add
 * @param pos the position in the layer stack
 * \memberof _DiagramData
 */
void
data_add_layer_at(DiagramData *data, Layer *layer, int pos)
{
  int len;
  int i;
  
  g_ptr_array_add(data->layers, layer);
  len = data->layers->len;

  if ( (pos>=0) && (pos < len)) {
    for (i=len-1;i>pos;i--) {
      g_ptr_array_index(data->layers, i) = g_ptr_array_index(data->layers, i-1);
    }
    g_ptr_array_index(data->layers, pos) = layer;
  }
  
  layer->parent_diagram = data;
  data_emit (data, layer, NULL, "object_add");
  layer_update_extents(layer);
  data_update_extents(data);
}

/*!
 * \brief Get the index of a layer contained in the diagram
 *
 * @param data the diagram
 * @param layer the layer
 * @return the zero based index or -1 if not found
 *
 * \memberof _DiagramData
 */
int
data_layer_get_index (const DiagramData *data, const Layer *layer)
{
  int len;
  int i;
  
  len = data->layers->len;
  for (i=0;i<len;++i) {
    if (layer == g_ptr_array_index(data->layers, i))
      return i;
  }
  return -1;
}
/*!
 * \brief Get the layer at position index
 *
 * @param data the diagram
 * @param index the layer position
 * @return the _Layer or NULL if not found
 *
 * \memberof _DiagramData
 */
Layer *
data_layer_get_nth (const DiagramData *data, guint index)
{
  if (data->layers->len > index)
    return g_ptr_array_index(data->layers, index);
  return NULL;
}
int 
data_layer_count(const DiagramData *data)
{
  return data->layers->len;
}

/*!
 * \brief Set which layer is the active layer in a diagram.
 * @param data The diagram in which to set the active layer.
 * @param layer The layer that should be active.
 *
 * \memberof _DiagramData
 */
void
data_set_active_layer(DiagramData *data, Layer *layer)
{
  data->active_layer = layer;
}

/*!
 * \brief Remove a layer from a diagram.
 * @param data The diagram to remove the layer from.
 * @param layer The layer to remove.
 * \memberof _DiagramData
 */
void
data_remove_layer(DiagramData *data, Layer *layer)
{
  if (data->layers->len<=1)
    return;

  if (data->active_layer == layer) {
    data_remove_all_selected(data);
  }
  data_emit (data, layer, NULL, "object_remove");
  layer->parent_diagram = NULL;
  g_ptr_array_remove(data->layers, layer);

  if (data->active_layer == layer) {
    data->active_layer = g_ptr_array_index(data->layers, 0);
  }
}

static ObjectHighlight *
find_object_highlight(GList *list, DiaObject *obj)
{
  ObjectHighlight *oh=NULL;
  while(list) {
    oh = (ObjectHighlight*)list->data;
    if (oh && oh->obj == obj) {
      return oh;
    }
    list = g_list_next(list);
  }
  return NULL;
}

void 
data_highlight_add(DiagramData *data, DiaObject *obj, DiaHighlightType type)
{
  ObjectHighlight *oh;
  if (find_object_highlight (data->highlighted, obj))
    return; /* should this be an error?`*/
  oh = g_malloc(sizeof(ObjectHighlight));
  oh->obj = obj;
  oh->type = type;
  data->highlighted = g_list_prepend(data->highlighted, oh);
}

void 
data_highlight_remove(DiagramData *data, DiaObject *obj)
{
  ObjectHighlight *oh;
  if (!(oh = find_object_highlight (data->highlighted, obj)))
    return; /* should this be an error?`*/
  data->highlighted = g_list_remove(data->highlighted, oh);
  g_free(oh);
}

DiaHighlightType 
data_object_get_highlight(DiagramData *data, DiaObject *obj)
{
  ObjectHighlight *oh;
  DiaHighlightType type = DIA_HIGHLIGHT_NONE;
  if ((oh = find_object_highlight (data->highlighted, obj)) != NULL) {
    type = oh->type;
  }
  return type;
}

/*!
 * \brief Select an object in a diagram
 *
 * Note that this does not unselect other objects currently selected in the diagram.
 * @param data The diagram to select in.
 * @param obj The object to select
 *
 * \memberof _DiagramData
 */
void
data_select(DiagramData *data, DiaObject *obj)
{
  if (g_list_find (data->selected, obj))
    return; /* should this be an error?`*/
  data->selected = g_list_prepend(data->selected, obj);
  data->selected_count_private++;
  g_signal_emit (data, diagram_data_signals[SELECTION_CHANGED], 0, data->selected_count_private);
}

/*!
 * \brief Deselect an object in a diagram
 *
 * Note that other objects may still be selected after this function is done.
 * @param data The diagram to deselect in.
 * @param obj The object to deselect.
 *
 * \memberof _DiagramData
 */
void
data_unselect(DiagramData *data, DiaObject *obj)
{
  if (!g_list_find (data->selected, obj))
    return; /* should this be an error?`*/
  data->selected = g_list_remove(data->selected, obj);
  data->selected_count_private--;
  g_signal_emit (data, diagram_data_signals[SELECTION_CHANGED], 0, data->selected_count_private);
}

/*!
 * \brief Clears the list of selected objects.
 * Does *not* remove these objects from the object list, despite its name.
 * @param data The diagram to unselect all objects in.
 * \memberof _DiagramData
 */
void
data_remove_all_selected(DiagramData *data)
{
  g_list_free(data->selected); /* Remove previous selection */
  data->selected = NULL;
  data->selected_count_private = 0;
  g_signal_emit (data, diagram_data_signals[SELECTION_CHANGED], 0, data->selected_count_private);
}

/*!
 * \brief Return TRUE if the diagram has visible layers.
 * @param data The diagram to check.
 * @return TRUE if at least one layer in the diagram is visible.
 * \protected \memberof _DiagramData
 */
static gboolean
data_has_visible_layers(DiagramData *data)
{
  guint i;
  for (i = 0; i < data->layers->len; i++) {
    Layer *layer = g_ptr_array_index(data->layers, i);
    if (layer->visible) return TRUE;
  }
  return FALSE;
}

/*!
 * \brief Set the diagram extents field to the union of the extents of the layers.
 * @param data The diagram to get the extents for.
 * \protected \memberof _DiagramData
 */
static void
data_get_layers_extents_union(DiagramData *data)
{
  guint i;
  gboolean first = TRUE;
  Rectangle new_extents;

  for ( i = 0 ; i<data->layers->len; i++) {
    Layer *layer = g_ptr_array_index(data->layers, i);    
    if (!layer->visible) continue;
    
    layer_update_extents(layer);

    if (first) {
      new_extents = layer->extents;
      first = rectangle_equals(&new_extents,&invalid_extents);
    } else {
      if (!rectangle_equals(&layer->extents,&invalid_extents)) {
        rectangle_union(&new_extents, &layer->extents);
      }
    }
  }

  data->extents = new_extents;
}

/*!
 * \brief Change diagram scaling so that the extents are exactly visible.
 * @param data The diagram to adjust.
 * \protected \memberof _DiagramData
 */
static void
data_adapt_scaling_to_extents(DiagramData *data)
{
  gdouble pwidth = data->paper.width * data->paper.scaling;
  gdouble pheight = data->paper.height * data->paper.scaling;

  gdouble xscale = data->paper.fitwidth * pwidth /
    (data->extents.right - data->extents.left);
  gdouble yscale = data->paper.fitheight * pheight /
    (data->extents.bottom - data->extents.top);
  
  data->paper.scaling = (float)MIN(xscale, yscale);
  data->paper.width  = (float)(pwidth  / data->paper.scaling);
  data->paper.height = (float)(pheight / data->paper.scaling);
}

/*!
 * \brief Adjust the extents field of a diagram.
 * @param data The diagram to adjust.
 * @return TRUE if the extents changed.
 * \protected \memberof _DiagramData
 */
static gboolean
data_compute_extents(DiagramData *data)
{
  Rectangle old_extents = data->extents;

  if (!data_has_visible_layers(data)) {   
    if (data->layers->len > 0) {
      Layer *layer = g_ptr_array_index(data->layers, 0);    
      layer_update_extents(layer);
      
      data->extents = layer->extents;
    } else {
      data->extents = invalid_extents;
    }
  } else {
    data_get_layers_extents_union(data);
  }

  if (rectangle_equals(&data->extents,&invalid_extents)) {
      data->extents.left = 0.0;
      data->extents.right = 10.0;
      data->extents.top = 0.0;
      data->extents.bottom = 10.0;
  }
  return (!rectangle_equals(&data->extents,&old_extents));
}

/*!
 * \brief Update the extents of a diagram and adjust scaling if needed.
 * @param data Diagram to update.
 * @return TRUE if the diagram extents changed.
 * \memberof _DiagramData
 */
gboolean
data_update_extents(DiagramData *data)
{

  gboolean changed;

  changed = data_compute_extents(data);
  if (changed && data->paper.fitto) data_adapt_scaling_to_extents(data);

  return changed;
}

/*!
 * \brief Get a list of selected objects in layer ordering.
 * @param data The diagram to get objects from.
 * @return A list of all currently selected objects.  These all reside in
 *  the currently active layer.  This list should be freed after use.
 * \memberof _DiagramData
 */
GList *
data_get_sorted_selected(DiagramData *data)
{
  GList *list;
  GList *sorted_list;
  GList *found;
  DiaObject *obj;

  g_assert (g_list_length (data->selected) == data->selected_count_private);
  if (data->selected_count_private == 0)
    return NULL;
 
  sorted_list = NULL;
  list = g_list_last(data->active_layer->objects);
  while (list != NULL) {
    found = g_list_find(data->selected, list->data);
    if (found) {
      obj = (DiaObject *)found->data;
      sorted_list = g_list_prepend(sorted_list, obj);
    }
    list = g_list_previous(list);
  }

  return sorted_list;
}

/*!
 * \brief Remove the currently selected objects from the list of objects.
 *
 * The selected objects are returned in a newly created GList.
 * @param data The diagram to get and remove the selected objects from.
 * @return A list of all selected objects in the current diagram.  This list
 *  should be freed after use.  Unlike data_get_sorted_selected, the
 *  objects in the list are not in the diagram anymore.
 * \memberof _DiagramData
 */
GList *
data_get_sorted_selected_remove(DiagramData *data)
{
  GList *list,*tmp;
  GList *sorted_list;
  GList *found;
  DiaObject *obj;
  
  g_assert (g_list_length (data->selected) == data->selected_count_private);
  if (data->selected_count_private == 0)
    return NULL;
  
  sorted_list = NULL;
  list = g_list_last(data->active_layer->objects);
  while (list != NULL) {
    found = g_list_find(data->selected, list->data);
    if (found) {
      obj = (DiaObject *)found->data;
      sorted_list = g_list_prepend(sorted_list, obj);

      tmp = list;
      list = g_list_previous(list);
      data->active_layer->objects =
	g_list_remove_link(data->active_layer->objects, tmp);
    } else {
      list = g_list_previous(list);
    }
  }

  return sorted_list;
}


/*!
 * \brief Emits a GObject signal on DiagramData
 * @param data The DiagramData that emits the signal.
 * @param layer The Layer that the fired signal carries.
 * @param obj The DiaObject that the fired signal carries.
 * @param signal_name The name of the signal.
 * \memberof _DiagramData
 */
void 
data_emit(DiagramData *data, Layer *layer, DiaObject* obj, 
	  const char *signal_name) 
{
  /* check what signal it is */
  if (strcmp("object_add",signal_name) == 0)
    g_signal_emit(data, diagram_data_signals[OBJECT_ADD], 0, layer, obj);
  
  if (strcmp("object_remove",signal_name) == 0)
    g_signal_emit(data, diagram_data_signals[OBJECT_REMOVE], 0, layer, obj);
  
}


/*!
 * \brief Render a diagram
 * @param data The diagram to render.
 * @param renderer The renderer to render on.
 * @param update The area that needs updating or NULL
 * @param obj_renderer If non-NULL, an alternative renderer of objects.
 * @param gdata User data passed on to inner calls.
 * \memberof _DiagramData
 */
void
data_render(DiagramData *data, DiaRenderer *renderer, Rectangle *update,
	    ObjectRenderer obj_renderer, gpointer gdata)
{
  Layer *layer;
  guint i, active_layer;

  if (!renderer->is_interactive) 
    (DIA_RENDERER_GET_CLASS(renderer)->begin_render)(renderer, update);
  
  for (i=0; i<data->layers->len; i++) {
    layer = (Layer *) g_ptr_array_index(data->layers, i);
    active_layer = (layer == data->active_layer);
    if (layer->visible) {
      if (obj_renderer)
        layer_render(layer, renderer, update, obj_renderer, gdata, active_layer);
      else
        (DIA_RENDERER_GET_CLASS(renderer)->draw_layer)(renderer, layer, active_layer, update);
    }
  }
  
  if (!renderer->is_interactive) 
    (DIA_RENDERER_GET_CLASS(renderer)->end_render)(renderer);
}

/*!
 * \brief Calls data_render() for paginated formats
 *
 * Call data_render() for every used page in the diagram
 *
 * \memberof _DiagramData
 */
void
data_render_paginated (DiagramData *data, DiaRenderer *renderer, gpointer user_data)
{
  Rectangle *extents;
  gdouble width, height;
  gdouble x, y, initx, inity;
  gint xpos, ypos;

  /* the usable area of the page */
  width = data->paper.width;
  height = data->paper.height;

  /* get extents, and make them multiples of width / height */
  extents = &data->extents;
  initx = extents->left;
  inity = extents->top;
  /* make page boundaries align with origin */
  if (!data->paper.fitto) {
    initx = floor(initx / width)  * width;
    inity = floor(inity / height) * height;
  }

  /* iterate through all the pages in the diagram */
  for (y = inity, ypos = 0; y < extents->bottom; y += height, ypos++) {
    /* ensure we are not producing pages for epsilon */
    if ((extents->bottom - y) < 1e-6)
      break;
    for (x = initx, xpos = 0; x < extents->right; x += width, xpos++) {
      Rectangle page_bounds;

      if ((extents->right - x) < 1e-6)
	break;

      page_bounds.left = x;
      page_bounds.right = x + width;
      page_bounds.top = y;
      page_bounds.bottom = y + height;

      data_render (data, renderer, &page_bounds, NULL, user_data);
    }
  }
}

/*!
 * \brief Visit all objects within the diagram
 * @param data the diagram
 * @param func per object callback function
 * @param user_data data passed to the callback function
 * \memberof _DiagramData
 */
void 
data_foreach_object (DiagramData *data, GFunc func, gpointer user_data)
{
  Layer *layer;
  guint i;
  for (i=0; i<data->layers->len; i++) {
    layer = (Layer *) g_ptr_array_index(data->layers, i);
    g_list_foreach (layer->objects, func, user_data);
  }  
}
