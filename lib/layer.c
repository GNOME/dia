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

#include "config.h"

#include <glib/gi18n-lib.h>

#include "diagramdata.h"
#include "diarenderer.h"
#include "diainteractiverenderer.h"
#include "dynamic_obj.h"
#include "dia-layer.h"

static const DiaRectangle invalid_extents = { -1.0,-1.0,-1.0,-1.0 };

typedef struct _DiaLayerPrivate DiaLayerPrivate;
struct _DiaLayerPrivate {
  char *name;                  /* The name of the layer */
  DiaRectangle extents;           /* The extents of the layer */

  GList *objects;              /* List of objects in the layer,
                                  sorted by decreasing z-value,
                                  objects can ONLY be connected to objects
                                  in the same layer! */

  gboolean visible;            /* The visibility of the layer */
  gboolean connectable;        /* Whether the layer can currently be connected
                                  to. The selected layer is by default
                                  connectable */

  /* NOTE: THIS IS A _WEAK_ REF */
  /* This avoids ref cycles with diagram */
  DiagramData *parent_diagram; /* Back-pointer to the diagram. This
                                  must only be set by functions internal
                                  to the diagram, and accessed via
                                  layer_get_parent_diagram() */
};

G_DEFINE_TYPE_WITH_PRIVATE (DiaLayer, dia_layer, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_NAME,
  PROP_CONNECTABLE,
  PROP_VISIBLE,
  PROP_PARENT_DIAGRAM,
  LAST_PROP
};

static GParamSpec *pspecs[LAST_PROP] = { NULL, };


/**
 * SECTION:dia-layer
 * @title: DiaLayer
 * @short_description: Group of #DiaObject s in a #DiagramData
 *
 * Since: 0.98
 */


static void
dia_layer_finalize (GObject *object)
{
  DiaLayer *self = DIA_LAYER (object);
  DiaLayerPrivate *priv = dia_layer_get_instance_private (self);

  g_clear_pointer (&priv->name, g_free);
  destroy_object_list (priv->objects);

  g_clear_weak_pointer (&priv->parent_diagram);

  G_OBJECT_CLASS (dia_layer_parent_class)->finalize (object);
}


static void
dia_layer_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  DiaLayer *self = DIA_LAYER (object);
  DiaLayerPrivate *priv = dia_layer_get_instance_private (self);

  switch (property_id) {
    case PROP_NAME:
      g_clear_pointer (&priv->name, g_free);
      priv->name = g_value_dup_string (value);
      break;
    case PROP_CONNECTABLE:
      dia_layer_set_connectable (self, g_value_get_boolean (value));
      break;
    case PROP_VISIBLE:
      dia_layer_set_visible (self, g_value_get_boolean (value));
      break;
    case PROP_PARENT_DIAGRAM:
      dia_layer_set_parent_diagram (self, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_layer_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  DiaLayer *self = DIA_LAYER (object);

  switch (property_id) {
    case PROP_NAME:
      g_value_set_string (value, dia_layer_get_name (self));
      break;
    case PROP_CONNECTABLE:
      g_value_set_boolean (value, dia_layer_is_connectable (self));
      break;
    case PROP_VISIBLE:
      g_value_set_boolean (value, dia_layer_is_visible (self));
      break;
    case PROP_PARENT_DIAGRAM:
      g_value_set_object (value, dia_layer_get_parent_diagram (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_layer_class_init (DiaLayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = dia_layer_finalize;
  object_class->set_property = dia_layer_set_property;
  object_class->get_property = dia_layer_get_property;

  /**
   * DiaLayer:name:
   *
   * Since: 0.98
   */
  pspecs[PROP_NAME] =
    g_param_spec_string ("name",
                         "Name",
                         "Layer name",
                         NULL,
                         G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);

  /**
   * DiaLayer:connectable:
   *
   * Since: 0.98
   */
  pspecs[PROP_CONNECTABLE] =
    g_param_spec_boolean ("connectable",
                          "Connectable",
                          "Layer is connectable",
                          TRUE,
                          G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * DiaLayer:visible:
   *
   * Since: 0.98
   */
  pspecs[PROP_VISIBLE] =
    g_param_spec_boolean ("visible",
                          "Visible",
                          "Layer is visible",
                          TRUE,
                          G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * DiaLayer:parent-diagram:
   *
   * Since: 0.98
   */
  pspecs[PROP_PARENT_DIAGRAM] =
    g_param_spec_object ("parent-diagram",
                         "Parent Diagram",
                         "The diagram containing the layer",
                         DIA_TYPE_DIAGRAM_DATA,
                         G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}


static void
dia_layer_init (DiaLayer *self)
{
  DiaLayerPrivate *priv = dia_layer_get_instance_private (self);

  priv->visible = TRUE;
  priv->connectable = FALSE;

  priv->objects = NULL;

  priv->extents.left = 0.0;
  priv->extents.right = 10.0;
  priv->extents.top = 0.0;
  priv->extents.bottom = 10.0;
}


/*! The default object renderer.
 * @param obj An object to render.
 * @param renderer The renderer to render on.
 * @param active_layer The layer containing the object.
 * @param data The diagram containing the layer.
 * \ingroup DiagramStructure
 */
static void
normal_render (DiaObject   *obj,
               DiaRenderer *renderer,
               int          active_layer,
               gpointer     data)
{
  dia_renderer_draw_object (renderer, obj, NULL);
}


/**
 * render_bounding_boxes:
 *
 * bounding box debug helper : environment variable DIA_RENDER_BOUNDING_BOXES
 * set to !0 to see the calculated bounding boxes
 */
int
render_bounding_boxes (void)
{
  static int rbb = FALSE;
  static int once = TRUE;

  if (once) {
    rbb = g_getenv ("DIA_RENDER_BOUNDING_BOXES") != NULL;
    once = FALSE;
  }
  return rbb;
}

/**
 * layer_render:
 * @layer: The layer to render.
 * @renderer: The renderer to draw things with.
 * @update: The rectangle that requires update.  Only objects that
 *  intersect with this rectangle will actually be get rendered.
 * @obj_renderer: A function that will render an object.
 * @data: The diagram that the layer belongs to.
 * @active_layer: Which number layer in the diagram is currently active.
 *
 * Render all components of a single layer.
 *
 * This function also handles rendering of bounding boxes for debugging purposes.
 *
 * Since: 0.98
 */
void
dia_layer_render (DiaLayer       *layer,
                  DiaRenderer    *renderer,
                  DiaRectangle   *update,
                  ObjectRenderer  obj_renderer,
                  gpointer        data,
                  int             active_layer)
{
  GList *list;
  DiaObject *obj;
  DiaLayerPrivate *priv = dia_layer_get_instance_private (layer);

  if (obj_renderer == NULL)
    obj_renderer = normal_render;

  /* Draw all objects: */
  list = priv->objects;
  while (list != NULL) {
    obj = (DiaObject *) list->data;

    if (update==NULL || rectangle_intersects (update, &obj->bounding_box)) {
      if ((render_bounding_boxes ()) && DIA_IS_INTERACTIVE_RENDERER (renderer)) {
        Point p1, p2;
        Color col;
        p1.x = obj->bounding_box.left;
        p1.y = obj->bounding_box.top;
        p2.x = obj->bounding_box.right;
        p2.y = obj->bounding_box.bottom;
        col.red = 1.0;
        col.green = 0.0;
        col.blue = 1.0;
        col.alpha = 1.0;

        dia_renderer_set_linewidth (renderer,0.01);
        dia_renderer_draw_rect (renderer, &p1, &p2, NULL, &col);
      }
      (*obj_renderer) (obj, renderer, active_layer, data);
    }

    list = g_list_next (list);
  }
}

/**
 * dia_layer_new:
 * @name: Name of the new layer.
 * @parent: The #DiagramData that the layer will belong to,.
 *
 * Create a new layer in this diagram.
 *
 * Returns: A new #DiaLayer object
 *
 * Since: 0.98
 */
DiaLayer *
dia_layer_new (const char *name, DiagramData *parent)
{
  DiaLayer *layer;

  layer = g_object_new (DIA_TYPE_LAYER,
                        "name", name,
                        "parent-diagram", parent,
                        NULL);

  return layer;
}

/**
 * dia_layer_new_from_layer:
 * @old: The #DiaLayer to clone
 *
 * Returns: A new #DiaLayer object
 *
 * Since: 0.98
 */
DiaLayer *
dia_layer_new_from_layer (DiaLayer *old)
{
  DiaLayer *layer;
  DiaLayerPrivate *priv;
  DiaLayerPrivate *old_priv;

  g_return_val_if_fail (DIA_IS_LAYER (old), NULL);

  old_priv = dia_layer_get_instance_private (old);

  layer = g_object_new (DIA_TYPE_LAYER,
                        "name", dia_layer_get_name (old),
                        "visible", old_priv->visible,
                        "connectable", old_priv->connectable,
                        "parent-diagram", old_priv->parent_diagram,
                        NULL);

  priv = dia_layer_get_instance_private (layer);

  priv->extents = old_priv->extents;
  priv->objects = object_copy_list (priv->objects);

  return layer;
}


/*!
 * \brief Set the parent layer of an object.
 * @param element A DiaObject that should be part of a layer.
 * @param user_data The layer it should be part of.
 * \protected \memberof _Layer
 */
static void
set_parent_layer (gpointer element, gpointer user_data)
{
  ((DiaObject*) element)->parent_layer = (DiaLayer *) user_data;
  /* FIXME: even group members need a parent_layer and what about parent objects  ???
   * Now I know again why I always try to avoid back-pointers )-; --hb.
   * If the group objects didn't actually leave the diagram, this wouldn't
   * be a problem.  --LC */
}

/**
 * dia_layer_object_get_index:
 * @layer: The layer the object is (should be) in.
 * @obj: The object to look for.
 *
 * Get the index of an object in a layer.
 *
 * Returns: The index of the object in the layers list of objects. This is also
 *  the vertical position of the object.
 *
 * Since: 0.98
 */
int
dia_layer_object_get_index (DiaLayer *layer, DiaObject *obj)
{
  DiaLayerPrivate *priv = dia_layer_get_instance_private (layer);

  return (int) g_list_index (priv->objects, (gpointer) obj);
}

/**
 * dia_layer_object_get_nth:
 * @layer: The layer to query for the nth object
 * @index: The zero-based indexed of the object
 *
 * Get the object a index or %NULL
 *
 * Since: 0.98
 */
DiaObject *
dia_layer_object_get_nth (DiaLayer *layer, guint index)
{
  DiaLayerPrivate *priv = dia_layer_get_instance_private (layer);

  if (g_list_length (priv->objects) > index) {
    g_assert (g_list_nth (priv->objects, index));
    return (DiaObject *) g_list_nth (priv->objects, index)->data;
  }
  return NULL;
}

/**
 * dia_layer_object_count:
 * @layer: the #DiaLayer
 *
 * Since: 0.98
 */
int
dia_layer_object_count (DiaLayer *layer)
{
  DiaLayerPrivate *priv = dia_layer_get_instance_private (layer);

  return g_list_length (priv->objects);
}

/**
 * dia_layer_get_name:
 * @layer: the #DiaLayer
 *
 * Since: 0.98
 */
const char *
dia_layer_get_name (DiaLayer *layer)
{
  DiaLayerPrivate *priv = dia_layer_get_instance_private (layer);

  return priv->name;
}

/**
 * dia_layer_add_object:
 * @layer: The layer to add the object to.
 * @obj: The object to add. This must not already be part of another layer.
 *
 * Add an object to the top of a layer.
 *
 * Since: 0.98
 */
void
dia_layer_add_object (DiaLayer *layer, DiaObject *obj)
{
  DiaLayerPrivate *priv = dia_layer_get_instance_private (layer);

  priv->objects = g_list_append (priv->objects, (gpointer) obj);
  set_parent_layer (obj, layer);

  /* send a signal that we have added a object to the diagram */
  data_emit (dia_layer_get_parent_diagram (layer), layer, obj, "object_add");
}

/**
 * dia_layer_add_object_at:
 * @layer: The layer to add the object to.
 * @obj: The object to add.  This must not be part of another layer.
 * @pos: The top-to-bottom position this object should be inserted at.
 *
 * Add an object to a layer at a specific position.
 *
 * Since: 0.98
 */
void
dia_layer_add_object_at (DiaLayer *layer, DiaObject *obj, int pos)
{
  DiaLayerPrivate *priv = dia_layer_get_instance_private (layer);

  priv->objects = g_list_insert (priv->objects, (gpointer) obj, pos);
  set_parent_layer (obj, layer);

  /* send a signal that we have added a object to the diagram */
  data_emit (dia_layer_get_parent_diagram (layer), layer, obj, "object_add");
}

/**
 * dia_layer_add_objects:
 * @layer: The layer to add objects to.
 * @obj_list: The list of objects to add.  These must not already
 *  be part of another layer.
 *
 * Add a list of objects to the end of a layer.
 *
 * Since: 0.98
 */
void
dia_layer_add_objects (DiaLayer *layer, GList *obj_list)
{
  GList *list = obj_list;
  DiaLayerPrivate *priv = dia_layer_get_instance_private (layer);

  priv->objects = g_list_concat (priv->objects, obj_list);
  g_list_foreach (obj_list, set_parent_layer, layer);

  while (list != NULL) {
    DiaObject *obj = (DiaObject *)list->data;
    /* send a signal that we have added a object to the diagram */
    data_emit (dia_layer_get_parent_diagram (layer), layer, obj, "object_add");

    list = g_list_next(list);
  }
}

/**
 * dia_layer_add_objects_first:
 * @layer: The layer to add objects to.
 * @obj_list: The list of objects to add. These must not already
 *  be part of another layer.
 *
 * Add a list of objects to the top of a layer.
 *
 * Since: 0.98
 */
void
dia_layer_add_objects_first (DiaLayer *layer, GList *obj_list) {
  GList *list = obj_list;
  DiaLayerPrivate *priv = dia_layer_get_instance_private (layer);

  priv->objects = g_list_concat (obj_list, priv->objects);
  g_list_foreach (obj_list, set_parent_layer, layer);

  /* Send one signal per object added */
  while (list != NULL) {
    DiaObject *obj = (DiaObject *)list->data;
    /* send a signal that we have added a object to the diagram */
    data_emit (dia_layer_get_parent_diagram (layer), layer, obj, "object_add");

    list = g_list_next (list);
  }
}

/**
 * dia_layer_remove_object:
 * @layer: The layer to remove the object from.
 * @obj: The object to remove.
 *
 * Remove an object from a layer.
 *
 * Since: 0.98
 */
void
dia_layer_remove_object (DiaLayer *layer, DiaObject *obj)
{
  DiaLayerPrivate *priv = dia_layer_get_instance_private (layer);

  /* send a signal that we'll remove a object from the diagram */
  data_emit (dia_layer_get_parent_diagram (layer), layer, obj, "object_remove");

  priv->objects = g_list_remove (priv->objects, obj);
  dynobj_list_remove_object (obj);
  set_parent_layer (obj, NULL);
}

/**
 * dia_layer_remove_objects:
 * @layer: The layer to remove the objects from.
 * @obj_list: The objects to remove.
 *
 * Remove a list of objects from a layer.
 *
 * Since: 0.98
 */
void
dia_layer_remove_objects (DiaLayer *layer, GList *obj_list)
{
  DiaObject *obj;
  while (obj_list != NULL) {
    obj = (DiaObject *) obj_list->data;

    dia_layer_remove_object (layer, obj);

    obj_list = g_list_next (obj_list);
  }
}

/**
 * dia_layer_find_objects_intersecting_rectangle:
 * @layer: The layer to search in.
 * @rect: The rectangle to intersect with.
 *
 * Find the objects that intersect a given rectangle.
 *
 * Returns: List of objects whose bounding box intersect the rectangle.  The
 *  list should be freed by the caller.
 *
 * Since: 0.98
 */
GList *
dia_layer_find_objects_intersecting_rectangle (DiaLayer     *layer,
                                               DiaRectangle *rect)
{
  GList *list;
  GList *selected_list;
  DiaObject *obj;
  DiaLayerPrivate *priv = dia_layer_get_instance_private (layer);

  selected_list = NULL;
  list = priv->objects;
  while (list != NULL) {
    obj = (DiaObject *)list->data;

    if (rectangle_intersects (rect, &obj->bounding_box)) {
      if (dia_object_is_selectable (obj)) {
        selected_list = g_list_prepend (selected_list, obj);
      }
      /* Objects in closed groups do not get selected, but their parents do.
      * Since the parents bbox is outside the objects, they will be found
      * anyway and the inner object can just be skipped. */
    }

    list = g_list_next (list);
  }

  return selected_list;
}

/**
 * dia_layer_find_objects_in_rectangle:
 * @layer: The layer to search for objects in.
 * @rect: The rectangle that the objects should be in.
 *
 * Find objects entirely contained in a rectangle.
 *
 * Returns: A list containing the objects that are entirely contained in the
 *  rectangle.  The list should be freed by the caller.
 *
 * Since: 0.98
 */
GList *
dia_layer_find_objects_in_rectangle (DiaLayer *layer, DiaRectangle *rect)
{
  GList *list;
  GList *selected_list;
  DiaObject *obj;
  DiaLayerPrivate *priv = dia_layer_get_instance_private (layer);

  selected_list = NULL;
  list = priv->objects;
  while (list != NULL) {
    obj = (DiaObject *)list->data;

    if (rectangle_in_rectangle (rect, &obj->bounding_box)) {
      if (dia_object_is_selectable (obj)) {
        selected_list = g_list_prepend (selected_list, obj);
      }
    }

    list = g_list_next (list);
  }

  return selected_list;
}

/**
 * dia_layer_find_objects_containing_rectangle:
 * @layer: The layer to search for objects in.
 * @rect: The rectangle that the objects should surround.
 *
 * Find objects entirely containing a rectangle.
 *
 * Returns: A list containing the objects that entirely contain the
 *  rectangle.  The list should be freed by the caller.
 *
 * Since: 0.98
 */
GList *
dia_layer_find_objects_containing_rectangle (DiaLayer *layer, DiaRectangle *rect)
{
  GList *list;
  GList *selected_list;
  DiaObject *obj;
  DiaLayerPrivate *priv = dia_layer_get_instance_private (layer);

  g_return_val_if_fail  (layer != NULL, NULL);

  selected_list = NULL;
  list = priv->objects;
  while (list != NULL) {
    obj = (DiaObject *) list->data;

    if (rectangle_in_rectangle (&obj->bounding_box, rect)) {
      if (dia_object_is_selectable (obj)) {
        selected_list = g_list_prepend (selected_list, obj);
      }
    }

    list = g_list_next (list);
  }

  return selected_list;
}


/**
 * dia_layer_find_closest_object_except:
 * @layer: The layer to search in.
 * @pos: The point to compare to.
 * @maxdist: The maximum distance the object can be from the point.
 * @avoid: A list of objects that cannot be returned by this search.
 *
 * Find the object closest to the given point in the layer no further away
 * than maxdist, and not included in avoid.
 *
 * Stops it if finds an object that includes the point.
 *
 * Returns: The closest object, or %NULL if no allowed objects are closer than
 *  maxdist.
 *
 * Since: 0.98
 */
DiaObject *
dia_layer_find_closest_object_except (DiaLayer *layer,
                                      Point    *pos,
                                      real      maxdist,
                                      GList    *avoid)
{
  GList *l;
  DiaObject *closest;
  DiaObject *obj;
  real dist;
  GList *avoid_tmp;
  DiaLayerPrivate *priv = dia_layer_get_instance_private (layer);

  closest = NULL;

  for (l = priv->objects; l!=NULL; l = g_list_next(l)) {
    obj = (DiaObject *) l->data;

    /* Check bounding box here too. Might give speedup. */
    dist = dia_object_distance_from (obj, pos);

    if (maxdist-dist > 0.00000001) {
      for (avoid_tmp = avoid; avoid_tmp != NULL; avoid_tmp = avoid_tmp->next) {
        if (avoid_tmp->data == obj) {
          goto NEXTOBJECT;
        }
      }
      closest = obj;
    }
  NEXTOBJECT:
  ;
  }

  return closest;
}

/**
 * dia_layer_find_closest_object:
 * @layer: The layer to search in.
 * @pos: The point to compare to.
 * @maxdist: The maximum distance the object can be from the point.
 *
 * Find the object closest to the given point in the layer, no further away
 * than maxdist. Stops it if finds an object that includes the point.
 *
 * Returns: The closest object, or %NULL if none are closer than maxdist.
 *
 * Since: 0.98
 */
DiaObject *
dia_layer_find_closest_object (DiaLayer *layer, Point *pos, real maxdist)
{
  return dia_layer_find_closest_object_except (layer, pos, maxdist, NULL);
}


/**
 * dia_layer_find_closest_connectionpoint:
 * @layer: the layer to search in
 * @closest: connection point found or NULL
 * @pos: refernce position in diagram coordinates
 * @notthis: object not to search on
 *
 * Find the #ConnectionPoint closest to pos in a layer.
 *
 * Returns: the distance of the connection point and pos
 *
 * Since: 0.98
 */
real
dia_layer_find_closest_connectionpoint (DiaLayer         *layer,
                                        ConnectionPoint **closest,
                                        Point            *pos,
                                        DiaObject        *notthis)
{
  GList *l;
  DiaObject *obj;
  ConnectionPoint *cp;
  real mindist, dist;
  int i;
  DiaLayerPrivate *priv = dia_layer_get_instance_private (layer);

  mindist = 1000000.0; /* Realy big value... */

  *closest = NULL;

  for (l = priv->objects; l!=NULL; l = g_list_next (l) ) {
    obj = (DiaObject *) l->data;

    if (obj == notthis)
      continue;

    for (i = 0; i < obj->num_connections; i++) {
      cp = obj->connections[i];
      /* Note: Uses manhattan metric for speed... */
      dist = distance_point_point_manhattan (pos, &cp->pos);
      if (dist < mindist) {
        mindist = dist;
        *closest = cp;
      }
    }
  }

  return mindist;
}

/**
 * dia_layer_update_extents:
 * @layer: the #DiaLayer
 *
 * Recalculation of the bounding box containing all objects in the layer
 *
 * Since: 0.98
 */
int
dia_layer_update_extents (DiaLayer *layer)
{
  GList *l;
  DiaObject *obj;
  DiaRectangle new_extents;
  DiaLayerPrivate *priv = dia_layer_get_instance_private (layer);

  l = priv->objects;
  if (l!=NULL) {
    obj = (DiaObject *) l->data;
    new_extents = obj->bounding_box;
    l = g_list_next (l);

    while (l!=NULL) {
      const DiaRectangle *bbox;
      obj = (DiaObject *) l->data;
      /* don't consider empty (or broken) objects in the overall extents */
      bbox = &obj->bounding_box;
      if (bbox->right > bbox->left && bbox->bottom > bbox->top)
        rectangle_union (&new_extents, &obj->bounding_box);
      l = g_list_next (l);
    }
  } else {
    new_extents = invalid_extents;
  }

  if (rectangle_equals (&new_extents, &priv->extents)) return FALSE;

  priv->extents = new_extents;
  return TRUE;
}

/**
 * dia_layer_replace_object_with_list:
 * @layer: the #DiaLayer
 * @remove_obj: the #DiaObject that will be removed from layer
 * @insert_list: list of #DiaObject to insert where remove_obj was
 *
 * Swaps a list of objects with a single object
 *
 * This function exchanges the given object with the list of objects.
 *
 * Ownership of @remove_obj and @insert_list objects is swapped, too.
 *
 * Since: 0.98
 */
void
dia_layer_replace_object_with_list (DiaLayer  *layer,
                                    DiaObject *remove_obj,
                                    GList     *insert_list)
{
  GList *list, *il;
  DiaLayerPrivate *priv = dia_layer_get_instance_private (layer);

  list = g_list_find (priv->objects, remove_obj);

  g_assert (list!=NULL);
  dynobj_list_remove_object (remove_obj);
  data_emit (dia_layer_get_parent_diagram (layer), layer, remove_obj, "object_remove");
  set_parent_layer (remove_obj, NULL);
  g_list_foreach (insert_list, set_parent_layer, layer);

  if (list->prev == NULL) {
    priv->objects = insert_list;
  } else {
    list->prev->next = insert_list;
    insert_list->prev = list->prev;
  }
  if (list->next != NULL) {
    GList *last;
    last = g_list_last (insert_list);
    last->next = list->next;
    list->next->prev = last;
  }
  il = insert_list;
  while (il) {
    data_emit (dia_layer_get_parent_diagram (layer), layer, il->data, "object_add");
    il = g_list_next (il);
  }
  g_list_free_1 (list);

  /* with transformed groups the list and the single object are not necessarily
   * of the same size */
  dia_layer_update_extents (layer);
}

static void
layer_remove_dynobj (gpointer obj, gpointer userdata)
{
  dynobj_list_remove_object ((DiaObject*)obj);
}

/**
 * dia_layer_set_object_list:
 * @layer: the #DiaLayer
 * @list: new list of #DiaObject s
 *
 * Since: 0.98
 */
void
dia_layer_set_object_list (DiaLayer *layer, GList *list)
{
  GList *ol;
  DiaLayerPrivate *priv = dia_layer_get_instance_private (layer);

  /* signal removal on all objects */
  ol = priv->objects;
  while (ol) {
    if (!g_list_find (list, ol->data)) /* only if it really vanishes */
      data_emit (dia_layer_get_parent_diagram (layer), layer, ol->data, "object_remove");
    ol = g_list_next (ol);
  }
  /* restore old list */
  ol = priv->objects;
  g_list_foreach (priv->objects, set_parent_layer, NULL);
  g_list_foreach (priv->objects, layer_remove_dynobj, NULL);

  priv->objects = list;
  g_list_foreach (priv->objects, set_parent_layer, layer);
  /* signal addition on all objects */
  list = priv->objects;
  while (list) {
    if (!g_list_find (ol, list->data)) /* only if it is new */
      data_emit (dia_layer_get_parent_diagram (layer), layer, list->data, "object_add");
    list = g_list_next (list);
  }
  g_list_free (ol);
}


/**
 * dia_layer_get_object_list:
 * @layer: the #DiaLayer
 *
 * Since: 0.98
 */
GList *
dia_layer_get_object_list (DiaLayer *layer)
{
  DiaLayerPrivate *priv;

  g_return_val_if_fail (DIA_IS_LAYER (layer), NULL);

  priv = dia_layer_get_instance_private (layer);

  return priv->objects;
}


/**
 * dia_layer_get_parent_diagram:
 * @layer: the #DiaLayer
 *
 * Since: 0.98
 */
DiagramData *
dia_layer_get_parent_diagram (DiaLayer *layer)
{
  DiaLayerPrivate *priv;

  g_return_val_if_fail (DIA_IS_LAYER (layer), NULL);

  priv = dia_layer_get_instance_private (layer);

  return priv->parent_diagram;
}


/**
 * dia_layer_set_parent_diagram:
 * @layer: the #DiaLayer
 * @diagram: the #DiagramData
 *
 * Since: 0.98
 */
void
dia_layer_set_parent_diagram (DiaLayer    *layer,
                              DiagramData *diagram)
{
  DiaLayerPrivate *priv;

  g_return_if_fail (DIA_IS_LAYER (layer));

  priv = dia_layer_get_instance_private (layer);

  if (g_set_weak_pointer (&priv->parent_diagram, diagram)) {
    g_object_notify_by_pspec (G_OBJECT (layer), pspecs[PROP_PARENT_DIAGRAM]);
  }
}


/**
 * dia_layer_is_connectable:
 * @self: the #DiaLayer
 *
 * Since: 0.98
 */
gboolean
dia_layer_is_connectable (DiaLayer *self)
{
  DiaLayerPrivate *priv;

  g_return_val_if_fail (DIA_IS_LAYER (self), FALSE);

  priv = dia_layer_get_instance_private (self);

  return priv->connectable;
}


/**
 * dia_layer_set_connectable:
 * @self: the #DiaLayer
 * @connectable: the new connectable status
 *
 * Since: 0.98
 */
void
dia_layer_set_connectable (DiaLayer *self,
                           gboolean  connectable)
{
  DiaLayerPrivate *priv;

  g_return_if_fail (DIA_IS_LAYER (self));

  priv = dia_layer_get_instance_private (self);

  priv->connectable = connectable;

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_CONNECTABLE]);
}


/**
 * dia_layer_is_visible:
 * @self: the #DiaLayer
 *
 * Since: 0.98
 */
gboolean
dia_layer_is_visible (DiaLayer *self)
{
  DiaLayerPrivate *priv;

  g_return_val_if_fail (DIA_IS_LAYER (self), FALSE);

  priv = dia_layer_get_instance_private (self);

  return priv->visible;
}


/**
 * dia_layer_set_visible:
 * @self: the #DiaLayer
 * @visible: new visibility
 *
 * Since: 0.98
 */
void
dia_layer_set_visible (DiaLayer *self,
                       gboolean  visible)
{
  DiaLayerPrivate *priv;

  g_return_if_fail (DIA_IS_LAYER (self));

  priv = dia_layer_get_instance_private (self);

  priv->visible = visible;

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_VISIBLE]);
}


/**
 * dia_layer_get_extents:
 * @self: the #DiaLayer
 * @rect: (out): the extents
 *
 * Since: 0.98
 */
void
dia_layer_get_extents (DiaLayer     *self,
                       DiaRectangle *rect)
{
  DiaLayerPrivate *priv;

  g_return_if_fail (DIA_IS_LAYER (self));
  g_return_if_fail (rect != NULL);

  priv = dia_layer_get_instance_private (self);

  *rect = priv->extents;
}
