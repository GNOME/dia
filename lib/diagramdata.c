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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "intl.h"
#include "diagramdata.h"

#include "paper.h"

static const Rectangle invalid_extents = { -1.0,-1.0,-1.0,-1.0 };
static void set_parent_layer(gpointer layer, gpointer object);

DiagramData *
new_diagram_data (void)
{
	DiagramData *data;
	Layer *first_layer;
   
	data = g_new (DiagramData, 1);
   
	data->extents.left = 0.0; 
	data->extents.right = 10.0; 
	data->extents.top = 0.0; 
	data->extents.bottom = 10.0; 
 
	data->bg_color = color_white;

	get_paper_info (&data->paper, -1);

	data->grid.width_x = 1.0;
	data->grid.width_y = 1.0;
	data->grid.visible_x = 1;
	data->grid.visible_y = 1;

	data->guides.nhguides = 0;
	data->guides.hguides = NULL;
	data->guides.nvguides = 0;
	data->guides.vguides = NULL;

	first_layer = new_layer(g_strdup(_("Background")));
	data->layers = g_ptr_array_new ();
	g_ptr_array_add (data->layers, first_layer);
	data->active_layer = first_layer;

	data->selected_count = 0;
	data->selected = NULL;
  
	return data;
}

void
diagram_data_destroy(DiagramData *data)
{
  int i;

  g_free(data->paper.name);

  for (i=0;i<data->layers->len;i++) {
    layer_destroy(g_ptr_array_index(data->layers, i));
  }
  g_ptr_array_free(data->layers, TRUE);
  data->active_layer = NULL;

  g_list_free(data->selected);
  data->selected = NULL; /* for safety */
  data->selected_count = 0;
  g_free(data);
}

Layer *
new_layer(gchar *name)
{
  Layer *layer;

  layer = g_new(Layer, 1);

  layer->name = name;
  
  layer->visible = TRUE;

  layer->objects = NULL;

  layer->extents.left = 0.0; 
  layer->extents.right = 10.0; 
  layer->extents.top = 0.0; 
  layer->extents.bottom = 10.0; 
  
  return layer;
}

void
layer_destroy(Layer *layer)
{
  g_free(layer->name);
  destroy_object_list(layer->objects);
  g_free(layer);
}

void
data_raise_layer(DiagramData *data, Layer *layer)
{
  int i;
  int layer_nr = -1;
  Layer *tmp;
  
  for (i=0;i<data->layers->len;i++) {
    if (g_ptr_array_index(data->layers, i)==layer)
      layer_nr = i;
  }

  assert(layer_nr>=0);

  if (layer_nr < data->layers->len-1) {
    tmp = g_ptr_array_index(data->layers, layer_nr+1);
    g_ptr_array_index(data->layers, layer_nr+1) =
      g_ptr_array_index(data->layers, layer_nr);
    g_ptr_array_index(data->layers, layer_nr) = tmp;
  }
}

void
data_lower_layer(DiagramData *data, Layer *layer)
{
  int i;
  int layer_nr = -1;
  Layer *tmp;
  
  for (i=0;i<data->layers->len;i++) {
    if (g_ptr_array_index(data->layers, i)==layer)
      layer_nr = i;
  }

  assert(layer_nr>=0);

  if (layer_nr > 0) {
    tmp = g_ptr_array_index(data->layers, layer_nr-1);
    g_ptr_array_index(data->layers, layer_nr-1) =
      g_ptr_array_index(data->layers, layer_nr);
    g_ptr_array_index(data->layers, layer_nr) = tmp;
  }
}


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

void
data_set_active_layer(DiagramData *data, Layer *layer)
{
  data->active_layer = layer;
}

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

void
data_select(DiagramData *data, Object *obj)
{
  data->selected = g_list_prepend(data->selected, obj);
  data->selected_count++;
}

void
data_unselect(DiagramData *data, Object *obj)
{
  data->selected = g_list_remove(data->selected, obj);
  data->selected_count--;
}

void
data_remove_all_selected(DiagramData *data)
{
  g_list_free(data->selected); /* Remove previous selection */
  data->selected_count = 0;
  data->selected = NULL;
}

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

static void
data_get_layers_extents_union(DiagramData *data) {
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

static void
data_adapt_scaling_to_extents(DiagramData *data)
{
  gdouble pwidth = data->paper.width * data->paper.scaling;
  gdouble pheight = data->paper.height * data->paper.scaling;

  gdouble xscale = data->paper.fitwidth * pwidth /
    (data->extents.right - data->extents.left);
  gdouble yscale = data->paper.fitheight * pheight /
    (data->extents.bottom - data->extents.top);
  
  data->paper.scaling = MIN(xscale, yscale);
  data->paper.width  = pwidth  / data->paper.scaling;
  data->paper.height = pheight / data->paper.scaling;
}

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

gboolean
data_update_extents(DiagramData *data)
{

  gboolean changed;

  changed = data_compute_extents(data);
  if (changed && data->paper.fitto) data_adapt_scaling_to_extents(data);

  return changed;
}

GList *
data_get_sorted_selected(DiagramData *data)
{
  GList *list;
  GList *sorted_list;
  GList *found;
  Object *obj;

  if (data->selected_count == 0)
    return NULL;
  
  sorted_list = NULL;
  list = g_list_last(data->active_layer->objects);
  while (list != NULL) {
    found = g_list_find(data->selected, list->data);
    if (found) {
      obj = (Object *)found->data;
      sorted_list = g_list_prepend(sorted_list, obj);
    }
    list = g_list_previous(list);
  }

  return sorted_list;
}

GList *
data_get_sorted_selected_remove(DiagramData *data)
{
  GList *list,*tmp;
  GList *sorted_list;
  GList *found;
  Object *obj;
  
  if (data->selected_count == 0)
    return NULL;
  
  sorted_list = NULL;
  list = g_list_last(data->active_layer->objects);
  while (list != NULL) {
    found = g_list_find(data->selected, list->data);
    if (found) {
      obj = (Object *)found->data;
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

void
data_render(DiagramData *data, Renderer *renderer, Rectangle *update,
	    ObjectRenderer obj_renderer /* Can be NULL */,
	    gpointer gdata)
{
  Layer *layer;
  int i;
  int active_layer;

  if (!renderer->is_interactive) (renderer->ops->begin_render)(renderer);
  
  for (i=0; i<data->layers->len; i++) {
    layer = (Layer *) g_ptr_array_index(data->layers, i);
    active_layer = (layer == data->active_layer);
    if (layer->visible)
      layer_render(layer, renderer, update, obj_renderer, gdata, active_layer);
  }
  
  if (!renderer->is_interactive) (renderer->ops->end_render)(renderer);
}

static void
normal_render(Object *obj, Renderer *renderer,
	      int active_layer,
	      gpointer data)
{
  renderer->ops->draw_object(renderer, obj);
}


int render_bounding_boxes = FALSE;

/* If obj_renderer is NULL normal_render is used. */
void
layer_render(Layer *layer, Renderer *renderer, Rectangle *update,
	     ObjectRenderer obj_renderer,
	     gpointer data,
	     int active_layer)
{
  GList *list;
  Object *obj;

  if (obj_renderer == NULL)
    obj_renderer = normal_render;
  
  /* Draw all objects: */
  list = layer->objects;
  while (list!=NULL) {
    obj = (Object *) list->data;

    if (update==NULL || rectangle_intersects(update, &obj->bounding_box)) {
      if ((render_bounding_boxes) && (renderer->is_interactive)) {
	Point p1, p2;
	Color col;
	p1.x = obj->bounding_box.left;
	p1.y = obj->bounding_box.top;
	p2.x = obj->bounding_box.right;
	p2.y = obj->bounding_box.bottom;
	col.red = 1.0;
	col.green = 0.0;
	col.blue = 1.0;

        renderer->ops->set_linewidth(renderer,0.01);
	renderer->ops->draw_rect(renderer, &p1, &p2, &col);
      }
      (*obj_renderer)(obj, renderer, active_layer, data);
    }
    
    list = g_list_next(list);
  }
}

static void
set_parent_layer(gpointer element, gpointer user_data) {
  ((Object*)element)->parent_layer = (Layer*)user_data;
}

int
layer_object_index(Layer *layer, Object *obj)
{
  return (int)g_list_index(layer->objects, (gpointer) obj);
}

void
layer_add_object(Layer *layer, Object *obj)
{
  layer->objects = g_list_append(layer->objects, (gpointer) obj);
  set_parent_layer(obj, layer);
}

void
layer_add_object_at(Layer *layer, Object *obj, int pos)
{
  layer->objects = g_list_insert(layer->objects, (gpointer) obj, pos);
  set_parent_layer(obj, layer);
}

void
layer_add_objects(Layer *layer, GList *obj_list)
{
  layer->objects = g_list_concat(layer->objects, obj_list);
  g_list_foreach(obj_list, set_parent_layer, layer);
}

void
layer_add_objects_first(Layer *layer, GList *obj_list)
{
  layer->objects = g_list_concat(obj_list, layer->objects);
  g_list_foreach(obj_list, set_parent_layer, layer);
}

void
layer_remove_object(Layer *layer, Object *obj)
{
  layer->objects = g_list_remove(layer->objects, obj);
  set_parent_layer(obj, NULL);
}

void
layer_remove_objects(Layer *layer, GList *obj_list)
{
  Object *obj;
  while (obj_list != NULL) {
    obj = (Object *) obj_list->data;
    
    layer->objects = g_list_remove(layer->objects, obj);
    
    obj_list = g_list_next(obj_list);
    set_parent_layer(obj, NULL);
  }
}


GList *
layer_find_objects_intersecting_rectangle(Layer *layer, Rectangle *rect)
{
  GList *list;
  GList *selected_list;
  Object *obj;

  selected_list = NULL;
  list = layer->objects;
  while (list != NULL) {
    obj = (Object *)list->data;

    if (rectangle_intersects(rect, &obj->bounding_box)) {
      selected_list = g_list_prepend(selected_list, obj);
    }

    list = g_list_next(list);
  }

  return selected_list;
}

GList *
layer_find_objects_in_rectangle(Layer *layer, Rectangle *rect)
{
  GList *list;
  GList *selected_list;
  Object *obj;

  selected_list = NULL;
  list = layer->objects;
  while (list != NULL) {
    obj = (Object *)list->data;

    if (rectangle_in_rectangle(rect, &obj->bounding_box)) {
      selected_list = g_list_prepend(selected_list, obj);
    }
    
    list = g_list_next(list);
  }

  return selected_list;
}

Object *
layer_find_closest_object(Layer *layer, Point *pos, real maxdist)
{
  GList *l;
  Object *closest;
  Object *obj;
  real dist;
  
  closest = NULL;
  
  l = layer->objects;
  while(l!=NULL) {
    obj = (Object *) l->data;

    /* Check bounding box here too. Might give speedup. */
    dist = obj->ops->distance_from(obj, pos);

    if (dist<=maxdist) {
      closest = obj;
    }
    
    l = g_list_next(l);
  }

  return closest;
}

real layer_find_closest_connectionpoint(Layer *layer,
					ConnectionPoint **closest,
					Point *pos)
{
  GList *l;
  Object *obj;
  ConnectionPoint *cp;
  real mindist, dist;
  int i;

  mindist = 1000000.0; /* Realy big value... */
  
  *closest = NULL;
  
  l = layer->objects;
  while(l!=NULL) {
    obj = (Object *) l->data;

    for (i=0;i<obj->num_connections;i++) {
      cp = obj->connections[i];
      /* Note: Uses manhattan metric for speed... */
      dist = distance_point_point_manhattan(pos, &cp->pos);
      if (dist<mindist) {
	mindist = dist;
	*closest = cp;
      }
    }
    
    l = g_list_next(l);
  }

  return mindist;
}

int layer_update_extents(Layer *layer)
{
  GList *l;
  Object *obj;
  Rectangle new_extents;
  
  l = layer->objects;
  if (l!=NULL) {
    obj = (Object *) l->data;
    new_extents = obj->bounding_box;
    l = g_list_next(l);
  
    while(l!=NULL) {
      obj = (Object *) l->data;
      rectangle_union(&new_extents, &obj->bounding_box);
      l = g_list_next(l);
    }
  } else {
    new_extents = invalid_extents;
  }

  if (rectangle_equals(&new_extents,&layer->extents)) return FALSE;

  layer->extents = new_extents;
  return TRUE;
}

void
layer_replace_object_with_list(Layer *layer, Object *remove_obj,
			       GList *insert_list)
{
  GList *list;

  list = g_list_find(layer->objects, remove_obj);

  assert(list!=NULL);
  set_parent_layer(remove_obj, NULL);
  g_list_foreach(insert_list, set_parent_layer, layer);

  if (list->prev == NULL) {
    layer->objects = insert_list;
  } else {
    list->prev->next = insert_list;
    insert_list->prev = list->prev;
  }
  if (list->next != NULL) {
    GList *last;
    last = g_list_last(insert_list);
    last->next = list->next;
    list->next->prev = last;
  }
  g_list_free_1(list);
}

void layer_set_object_list(Layer *layer, GList *list)
{
  g_list_foreach(layer->objects, set_parent_layer, NULL);
  g_list_free(layer->objects);
  layer->objects = list;
  g_list_foreach(layer->objects, set_parent_layer, layer);
}

DiagramData *
layer_get_parent_diagram(Layer *layer)
{
  return layer->parent_diagram;
}
