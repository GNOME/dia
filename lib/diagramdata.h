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
#ifndef DIAGRAMDATA_H
#define DIAGRAMDATA_H

#include <glib.h>
#include "object.h"

typedef struct _DiagramData DiagramData;
typedef struct _Layer Layer;

struct _DiagramData {
  Rectangle extents;      /* The extents of the diagram        */

  Color bg_color;

  GPtrArray *layers;     /* Layers ordered by decreasing z-order */
  Layer *active_layer;

  guint selected_count;
  GList *selected;        /* List of objects that are selected,
			     all from the active layer! */
};

struct _Layer {
  char *name;
  Rectangle extents;      /* The extents of the layer        */

  GList *objects;         /* List of objects in the layer,
			     sorted by decreasing z-valued,
			     objects can ONLY be connected to objects
			     in the same layer! */

  int visible;
};

extern DiagramData *new_diagram_data(void);
extern void diagram_data_destroy(DiagramData *data);

extern Layer *new_layer(char *name);
extern void layer_destroy(Layer *layer);

extern void data_add_layer(DiagramData *data, Layer *layer);
extern void data_set_active_layer(DiagramData *data, Layer *layer);
extern void data_delete_active_layer(DiagramData *data);
extern void data_add_selected(DiagramData *data, Object *obj);
extern void data_remove_selected(DiagramData *data, Object *obj);
extern void data_remove_all_selected(DiagramData *data);
extern int data_update_extents(DiagramData *data); /* returns true if changed. */
extern GList *data_get_sorted_selected(DiagramData *data);
extern GList *data_get_sorted_selected_remove(DiagramData *data);

typedef void (*ObjectRenderer)(Object *obj, Renderer *renderer,
			       int active_layer,
			       gpointer data);
extern void data_render(DiagramData *data, Renderer *renderer,
			ObjectRenderer obj_renderer /* Can be NULL */,
			gpointer gdata);  
extern void layer_render(Layer *layer, Renderer *renderer,
			 ObjectRenderer obj_renderer /* Can be NULL */,
			 gpointer data,
			 int active_layer);

extern int layer_add_object(Layer *layer, Object *obj);
extern int layer_add_objects(Layer *layer, GList *obj_list);
extern int layer_add_objects_first(Layer *layer, GList *obj_list);
extern void layer_remove_object(Layer *layer, Object *obj);
extern GList *layer_find_objects_in_rectangle(Layer *layer, Rectangle *rect);
extern Object *layer_find_closest_object(Layer *layer, Point *pos,
					 real maxdist);
extern real layer_find_closest_connectionpoint(Layer *layer,
					       ConnectionPoint **closest,
					       Point *pos);
extern int layer_update_extents(Layer *layer); /* returns true if changed. */
extern void layer_replace_object_with_list(Layer *layer, Object *obj,
					   GList *list);


#endif /* DIAGRAMDATA_H */






