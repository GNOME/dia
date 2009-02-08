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
  LAST_SIGNAL
};

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

  object_class->finalize = diagram_data_finalize;
  klass->object_add = _diagram_data_object_add;
  klass->object_remove = _diagram_data_object_remove;
}

/** Create a new layer in this diagram.
 * @param name Name of the new layer.
 * @param parent The DiagramData that the layer will belong to,.
 * @return A new Layer object.
 * @bug Must determine if a NULL name is ok.
 * @bug This belongs in a layers.c file.
 * @bug Even though this sets parent diagram, it doesn't call the
 * update_extents functions that add_layer does.
 */
Layer *
new_layer(gchar *name, DiagramData *parent)
{
  Layer *layer;

  layer = g_new(Layer, 1);

  layer->name = name;

  layer->parent_diagram = parent;
  layer->visible = TRUE;
  layer->connectable = TRUE;

  layer->objects = NULL;

  layer->extents.left = 0.0; 
  layer->extents.right = 10.0; 
  layer->extents.top = 0.0; 
  layer->extents.bottom = 10.0; 
  
  return layer;
}

/** Destroy a layer object.
 * @param layer The layer object to deallocate entirely.
 * @bug This belongs in a layers.c file.
 */
void
layer_destroy(Layer *layer)
{
  g_free(layer->name);
  destroy_object_list(layer->objects);
  g_free(layer);
}

/** Raise a layer up one in a diagram.
 * @param data The diagram that the layer belongs to.
 * @param layer The layer to raise.
 * @bug The diagram doesn't really need to be passed, as the layer knows it.
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

/** Lower a layer by one in a diagram.
 * @param data The diagram that the layer belongs to.
 * @param layer The layer to lower.
 * @bug The diagram doesn't really need to be passed, as the layer knows it.
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

/** Add a layer object to a diagram.
 * @param data The diagram to add the layer to.
 * @param layer The layer to add.
 * @bug Should just call data_add_layer_at().
 */
void
data_add_layer(DiagramData *data, Layer *layer)
{
  g_ptr_array_add(data->layers, layer);
  layer->parent_diagram = data;
  layer_update_extents(layer);
  data_update_extents(data);
}

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
  layer_update_extents(layer);
  data_update_extents(data);
}

/** Set which layer is the active layer in a diagram.
 * @param data The diagram in which to set the active layer.
 * @param layer The layer that should be active.
 * @bug The diagram doesn't really need to be passed, as the layer knows it.
 */
void
data_set_active_layer(DiagramData *data, Layer *layer)
{
  data->active_layer = layer;
}

/** Delete a layer from a diagram.
 * @param data The diagram to delete the layer from.
 * @param layer The layer to delete.
 * @bug The diagram doesn't really need to be passed, as the layer knows it.
 */
void
data_delete_layer(DiagramData *data, Layer *layer)
{
  if (data->layers->len<=1)
    return;

  if (data->active_layer == layer) {
    data_remove_all_selected(data);
  }
  layer->parent_diagram = NULL;
  g_ptr_array_remove(data->layers, layer);

  if (data->active_layer == layer) {
    data->active_layer = g_ptr_array_index(data->layers, 0);
  }
}

/** Select an object in a diagram.  Note that this does not unselect other
 *  objects currently selected in the diagram.
 * @param data The diagram to select in.
 * @param obj The object to select.
 * @bug Does not need to be passed the diagram, as it can be found from the 
 *  object.
 */
void
data_select(DiagramData *data, DiaObject *obj)
{
  if (g_list_find (data->selected, obj))
    return; /* should this be an error?`*/
  data->selected = g_list_prepend(data->selected, obj);
  data->selected_count_private++;
}

/** Deselect an object in a diagram.  Note that other objects may still be
 *  selected after this function is done.
 * @param data The diagram to deselect in.
 * @param obj The object to deselect.
 * @bug Does not need to be passed the diagram, as it can be found from the 
 *  object.
 */
void
data_unselect(DiagramData *data, DiaObject *obj)
{
  if (!g_list_find (data->selected, obj))
    return; /* should this be an error?`*/
  data->selected = g_list_remove(data->selected, obj);
  data->selected_count_private--;
}

/** Clears the list of selected objects.
 * Does *not* remove these objects from the object list, despite its name.
 * @param data The diagram to unselect all objects in.
 */
void
data_remove_all_selected(DiagramData *data)
{
  g_list_free(data->selected); /* Remove previous selection */
  data->selected = NULL;
  data->selected_count_private = 0;
}

/** Return TRUE if the diagram has visible layers.
 * @param data The diagram to check.
 * @return TRUE if at least one layer in the diagram is visible.
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

/** Set the diagram extents field to the union of the extents of the layers.
 * @param data The diagram to get the extents for.
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

/** Change diagram scaling so that the extents are exactly visible.
 * @param data The diagram to adjust.
 * @bug Consider making it a teeny bit larger, or check that *all* objects
 *  calculate their extents correctly.
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

/** Adjust the extents field of a diagram.
 * @param data The diagram to adjust.
 * @return TRUE if the extents changed.
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

/** Update the extents of a diagram and adjust scaling if needed.
 * @param data Diagram to update.
 * @return TRUE if the diagram extents changed.
 */
gboolean
data_update_extents(DiagramData *data)
{

  gboolean changed;

  changed = data_compute_extents(data);
  if (changed && data->paper.fitto) data_adapt_scaling_to_extents(data);

  return changed;
}

/** Get a list of selected objects in layer ordering.
 * @param data The diagram to get objects from.
 * @return A list of all currently selected objects.  These all reside in
 *  the currently active layer.  This list should be freed after use.
 * @bug Does selection update correctly when the layer changes?
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

/** Remove the currently selected objects from the list of objects.
 * The selected objects are returned in a newly created GList.
 * @param data The diagram to get and remove the selected objects from.
 * @return A list of all selected objects in the current diagram.  This list
 *  should be freed after use.  Unlike data_get_sorted_selected, the
 *  objects in the list are not in the diagram anymore.
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


/** Emits a GObject signal on DiagramData
 *  @param data The DiagramData that emits the signal.
 *  @param layer The Layer that the fired signal carries.
 *  @param obj The DiaObject that the fired signal carries.
 *  @param signal_name The name of the signal.
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


/** Render a diagram.
 * @param data The diagram to render.
 * @param renderer The renderer to render on.
 * @param update The area that needs updating.
 * @param obj_renderer If non-NULL, an alternative renderer of objects.
 * @param gdata User data passed on to inner calls.
 * @bug Describe obj_renderer better.
 */
void
data_render(DiagramData *data, DiaRenderer *renderer, Rectangle *update,
	    ObjectRenderer obj_renderer,
	    gpointer gdata)
{
  Layer *layer;
  guint i, active_layer;

  if (!renderer->is_interactive) 
    (DIA_RENDERER_GET_CLASS(renderer)->begin_render)(renderer);
  
  for (i=0; i<data->layers->len; i++) {
    layer = (Layer *) g_ptr_array_index(data->layers, i);
    active_layer = (layer == data->active_layer);
    if (layer->visible)
      layer_render(layer, renderer, update, obj_renderer, gdata, active_layer);
  }
  
  if (!renderer->is_interactive) 
    (DIA_RENDERER_GET_CLASS(renderer)->end_render)(renderer);
}

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

