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

#include <stdlib.h>

#include "object_ops.h"
#include "connectionpoint_ops.h"
#include "handle_ops.h"
#include "message.h"
#include "object.h"

#define OBJECT_CONNECT_DISTANCE 4.5

void
object_add_updates(DiaObject *obj, Diagram *dia)
{
  int i;

  /* Bounding box */
  if (data_object_get_highlight(dia->data,obj) != DIA_HIGHLIGHT_NONE) {
    diagram_add_update_with_border(dia, dia_object_get_enclosing_box (obj), 5);
  } else {
    diagram_add_update(dia, dia_object_get_enclosing_box (obj));
  }

  /* Handles */
  for (i=0;i<obj->num_handles;i++) {
    handle_add_update(obj->handles[i], dia);
  }

  /* Connection points */
  for (i=0;i<dia_object_get_num_connections(obj);++i) {
    connectionpoint_add_update(obj->connections[i], dia);
  }

}

void
object_add_updates_list(GList *list, Diagram *dia)
{
  DiaObject *obj;

  while (list != NULL) {
    obj = (DiaObject *)list->data;

    object_add_updates(obj, dia);

    list = g_list_next(list);
  }
}


/**
 * object_find_connectpoint_display:
 * @ddisp: The display to search
 * @pos: A position in the display, typically mouse position
 * @notthis: If not null, an object to ignore (typically the object
 *           connecting)
 * @snap_to_objects: Whether snapping to objects should be in effect
 *                   in this call (anded to the display-wide setting).
 *
 * Find a connectionpoint sufficiently close to the given point.
 *
 * Since: dawn-of-time
 */
ConnectionPoint *
object_find_connectpoint_display (DDisplay  *ddisp,
                                  Point     *pos,
                                  DiaObject *notthis,
                                  gboolean   snap_to_objects)
{
  double distance;
  ConnectionPoint *connectionpoint;
  GList *avoid = NULL;
  DiaObject *obj_here;

  distance =
    diagram_find_closest_connectionpoint(ddisp->diagram, &connectionpoint,
					 pos, notthis);

  distance = ddisplay_transform_length(ddisp, distance);
  if (distance < OBJECT_CONNECT_DISTANCE) {
    return connectionpoint;
  }
  if (ddisp->mainpoint_magnetism && snap_to_objects) {
      DiaObject *parent;
      /* Try to find an all-object CP. */
      /* Don't pick a parent, though */
      avoid = g_list_prepend(avoid, notthis);
      for (parent = notthis->parent; parent != NULL; parent = parent->parent) {
	  avoid = g_list_prepend(avoid, parent);
      }
      obj_here = diagram_find_clicked_object_except(ddisp->diagram, pos, 0.00001, avoid);
      if (obj_here != NULL) {
	  int i;
	  for (i = 0; i < dia_object_get_num_connections(obj_here); ++i) {
	      if (obj_here->connections[i]->flags & CP_FLAG_ANYPLACE) {
		  g_list_free(avoid);
		  return obj_here->connections[i];
	      }
	  }
      }
  }

  return NULL;
}

/* pushes undo info */
void
object_connect_display(DDisplay *ddisp, DiaObject *obj, Handle *handle,
		       gboolean snap_to_objects)
{
  ConnectionPoint *connectionpoint;

  if (handle == NULL)
    return;

  if (handle->connected_to == NULL) {
    connectionpoint = object_find_connectpoint_display(ddisp, &handle->pos,
						       obj, snap_to_objects);

    if (connectionpoint != NULL) {
      DiaChange *change = dia_connect_change_new (ddisp->diagram,
                                                  obj,
                                                  handle,
                                                  connectionpoint);
      dia_change_apply (change, DIA_DIAGRAM_DATA (ddisp->diagram));
    }
  }
}

Point
object_list_corner(GList *list)
{
  Point p = {0.0,0.0};
  DiaObject *obj;

  if (list == NULL)
    return p;

  obj = (DiaObject *)list->data;
  p.x = obj->bounding_box.left;
  p.y = obj->bounding_box.top;

  list = g_list_next(list);

  while (list != NULL) {
    obj = (DiaObject *)list->data;

    if (p.x > obj->bounding_box.left)
      p.x = obj->bounding_box.left;
    if (p.y > obj->bounding_box.top)
      p.y = obj->bounding_box.top;

    list = g_list_next(list);
  }

  return p;
}

static int
object_list_sort_vertical(const void *o1, const void *o2)
{
    DiaObject *obj1 = *(DiaObject **)o1;
    DiaObject *obj2 = *(DiaObject **)o2;

    return (obj1->bounding_box.bottom+obj1->bounding_box.top)/2 -
	(obj2->bounding_box.bottom+obj2->bounding_box.top)/2;
}

/*!
 * \brief Separate list of objects into connected and not connected ones
 *
 * When moving around objects it is useful to separate nodes and edges.
 * The former usually should be aligned somehow, the latter will just follow
 * the nodes through their connection points.
 * More specialized algorithms need real edges, i.e. at least two handles have
 * to be connected to (different) objects.
 *
 * @param objects the list to filter
 * @param num_connects the minimum number to be considered connection
 * @param connected the edges (objects connected)
 * @param unconnected the nodes (still might be connected by)
 */
static void
filter_connected (const GList *objects,
		  int num_connects,
		  GList **connected,
		  GList **unconnected)
{
  const GList *list;

  for (list = objects; list != NULL; list = g_list_next (list)) {
    DiaObject *obj = list->data;
    gboolean is_connected = 0;
    int i;

    for (i = 0; i < obj->num_handles; ++i) {
      if (obj->handles[i]->connected_to)
        ++is_connected;
    }

    if (is_connected >= num_connects) {
      if (connected)
        *connected = g_list_append (*connected, obj);
    } else {
      if (unconnected)
        *unconnected = g_list_append (*unconnected, obj);
    }
  }
}

/*!
 * \brief Align objects by moving them vertically
 *
 * For each node in objects align them vertically. The connections (edges) will follow.
 *
 * @param objects  selection of objects to be considered
 * @param dia      the diagram owning the objects (and holding undo information)
 * @param align    the alignment algorithm
 */
void
object_list_align_v(GList *objects, Diagram *dia, int align)
{
  GList *list;
  Point *orig_pos;
  Point *dest_pos;
  real y_pos = 0;
  DiaObject *obj;
  Point pos;
  real top, bottom, freespc;
  int nobjs;
  int i;
  GList *unconnected = NULL;

  filter_connected (objects, 1, NULL, &unconnected);
  objects = unconnected;
  if (objects==NULL)
    return;

  obj = (DiaObject *) objects->data; /*  First object */

  top = obj->bounding_box.top;
  bottom = obj->bounding_box.bottom;
  freespc = bottom - top;

  nobjs = 1;
  list = objects->next;
  while (list != NULL) {
    obj = (DiaObject *) list->data;

    if (obj->bounding_box.top < top)
      top = obj->bounding_box.top;
    if (obj->bounding_box.bottom > bottom)
      bottom = obj->bounding_box.bottom;

    freespc += obj->bounding_box.bottom - obj->bounding_box.top;
    nobjs++;

    list = g_list_next(list);
  }

  /*
   * These alignments can alter the order of elements, so we need
   * to sort them out by position.
   */
  if (align == DIA_ALIGN_EQUAL || align == DIA_ALIGN_ADJACENT) {
    DiaObject **object_array = g_new0 (DiaObject *, nobjs);
    int j = 0;

    list = objects;
    while (list != NULL) {
      obj = (DiaObject *) list->data;
      object_array[j] = obj;
      j++;
      list = g_list_next (list);
    }
    qsort (object_array, nobjs, sizeof (DiaObject*), object_list_sort_vertical);
    list = NULL;
    for (int k = 0; k < nobjs; k++) {
      list = g_list_append (list, object_array[k]);
    }

    g_return_if_fail (objects == unconnected);

    g_list_free (unconnected);
    objects = unconnected = list;
    g_clear_pointer (&object_array, g_free);
  }

  switch (align) {
    case DIA_ALIGN_TOP: /* TOP */
      y_pos = top;
      break;
    case DIA_ALIGN_CENTER: /* CENTER */
      y_pos = (top + bottom)/2.0;
      break;
    case DIA_ALIGN_BOTTOM: /* BOTTOM */
      y_pos = bottom;
      break;
    case DIA_ALIGN_POSITION: /* OBJECT POSITION */
      y_pos = (top + bottom)/2.0;
      break;
    case DIA_ALIGN_EQUAL: /* EQUAL DISTANCE */
      freespc = (bottom - top - freespc)/(double)(nobjs - 1);
      y_pos = top;
      break;
    case DIA_ALIGN_ADJACENT: /* ADJACENT */
      y_pos = top;
      break;
    default:
      message_warning("Wrong argument to object_list_align_v()\n");
  }

  dest_pos = g_new(Point, nobjs);
  orig_pos = g_new(Point, nobjs);

  i = 0;
  list = objects;
  while (list != NULL) {
    obj = (DiaObject *) list->data;

    pos.x = obj->position.x;

    switch (align) {
      case DIA_ALIGN_TOP: /* TOP */
        pos.y = y_pos + obj->position.y - obj->bounding_box.top;
        break;
      case DIA_ALIGN_CENTER: /* CENTER */
        pos.y = y_pos + obj->position.y - (obj->bounding_box.top + obj->bounding_box.bottom)/2.0;
        break;
      case DIA_ALIGN_BOTTOM: /* BOTTOM */
        pos.y = y_pos - (obj->bounding_box.bottom - obj->position.y);
        break;
      case DIA_ALIGN_POSITION: /* OBJECT POSITION */
        pos.y = y_pos;
        break;
      case DIA_ALIGN_EQUAL: /* EQUAL DISTANCE */
        pos.y = y_pos + obj->position.y - obj->bounding_box.top;
        y_pos += obj->bounding_box.bottom - obj->bounding_box.top + freespc;
        break;
      case DIA_ALIGN_ADJACENT: /* ADJACENT */
        pos.y = y_pos + obj->position.y - obj->bounding_box.top;
        y_pos += obj->bounding_box.bottom - obj->bounding_box.top;
        break;
      default:
        g_return_if_reached ();
    }

    pos.x = obj->position.x;

    orig_pos[i] = obj->position;
    dest_pos[i] = pos;

    obj->ops->move(obj, &pos);

    i++;
    list = g_list_next(list);
  }

  dia_move_objects_change_new (dia, orig_pos, dest_pos, g_list_copy (objects));
  g_list_free (unconnected);
}


static int
object_list_sort_horizontal(const void *o1, const void *o2)
{
  DiaObject *obj1 = *(DiaObject **)o1;
  DiaObject *obj2 = *(DiaObject **)o2;

  return (obj1->bounding_box.right+obj1->bounding_box.left)/2 -
    (obj2->bounding_box.right+obj2->bounding_box.left)/2;
}

/*!
 * \brief Align objects by moving then horizontally
 *
 * For each node in objects align them horizontally. The connections (edges) will follow.
 *
 * @param objects  selection of objects to be considered
 * @param dia      the diagram owning the objects (and holding undo information)
 * @param align    the alignment algorithm
 */
void
object_list_align_h(GList *objects, Diagram *dia, int align)
{
  GList *list;
  Point *orig_pos;
  Point *dest_pos;
  real x_pos = 0;
  DiaObject *obj;
  Point pos;
  real left, right, freespc = 0;
  int nobjs;
  int i;
  GList *unconnected = NULL;

  filter_connected (objects, 1, NULL, &unconnected);
  objects = unconnected;
  if (objects==NULL)
    return;

  obj = (DiaObject *) objects->data; /*  First object */

  left = obj->bounding_box.left;
  right = obj->bounding_box.right;
  freespc = right - left;

  nobjs = 1;
  list = objects->next;
  while (list != NULL) {
    obj = (DiaObject *) list->data;

    if (obj->bounding_box.left < left)
      left = obj->bounding_box.left;
    if (obj->bounding_box.right > right)
      right = obj->bounding_box.right;

    freespc += obj->bounding_box.right - obj->bounding_box.left;
    nobjs++;

    list = g_list_next(list);
  }

  /*
   * These alignments can alter the order of elements, so we need
   * to sort them out by position.
   */
  if (align == DIA_ALIGN_EQUAL || align == DIA_ALIGN_ADJACENT) {
    DiaObject **object_array = g_new0 (DiaObject *, nobjs);
    int j = 0;

    list = objects;
    while (list != NULL) {
      obj = (DiaObject *) list->data;
      object_array[j] = obj;
      j++;
      list = g_list_next(list);
    }
    qsort(object_array, nobjs, sizeof(DiaObject*), object_list_sort_horizontal);
    list = NULL;
    for (j = 0; j < nobjs; j++) {
      list = g_list_append(list, object_array[j]);
    }
    g_assert (objects == unconnected);
    g_list_free (unconnected);
    objects = unconnected = list;
    g_clear_pointer (&object_array, g_free);
  }

  switch (align) {
  case DIA_ALIGN_LEFT:
    x_pos = left;
    break;
  case DIA_ALIGN_CENTER:
    x_pos = (left + right)/2.0;
    break;
  case DIA_ALIGN_RIGHT:
    x_pos = right;
    break;
  case DIA_ALIGN_POSITION:
    x_pos = (left + right)/2.0;
    break;
  case DIA_ALIGN_EQUAL:
    freespc = (right - left - freespc)/(double)(nobjs - 1);
    x_pos = left;
    break;
  case DIA_ALIGN_ADJACENT:
    x_pos = left;
    break;
  default:
    message_warning("Wrong argument to object_list_align_h()\n");
  }

  dest_pos = g_new(Point, nobjs);
  orig_pos = g_new(Point, nobjs);

  i = 0;
  list = objects;
  while (list != NULL) {
    obj = (DiaObject *) list->data;

    switch (align) {
      case DIA_ALIGN_LEFT:
        pos.x = x_pos + obj->position.x - obj->bounding_box.left;
        break;
      case DIA_ALIGN_CENTER:
        pos.x = x_pos + obj->position.x - (obj->bounding_box.left + obj->bounding_box.right)/2.0;
        break;
      case DIA_ALIGN_RIGHT:
        pos.x = x_pos - (obj->bounding_box.right - obj->position.x);
        break;
      case DIA_ALIGN_POSITION:
        pos.x = x_pos;
        break;
      case DIA_ALIGN_EQUAL:
        pos.x = x_pos + obj->position.x - obj->bounding_box.left;
        x_pos += obj->bounding_box.right - obj->bounding_box.left + freespc;
        break;
      case DIA_ALIGN_ADJACENT:
        pos.x = x_pos + obj->position.x - obj->bounding_box.left;
        x_pos += obj->bounding_box.right - obj->bounding_box.left;
        break;
      default:
        break;
    }

    pos.y = obj->position.y;

    orig_pos[i] = obj->position;
    dest_pos[i] = pos;

    obj->ops->move(obj, &pos);

    i++;
    list = g_list_next(list);
  }

  dia_move_objects_change_new (dia, orig_pos, dest_pos, g_list_copy (objects));
  g_list_free (unconnected);
}

/*!
 * \brief Align objects at their connected points
 *
 * The result of the algorithm depends on the order of objects
 * in the list. The list is typically coming from the selection.
 * That order depends on the kind of selection performed.
 *  - selection by rubberband gives objects in reverse order of
 *    the the original layer order as of this writing
 *  - Select/Transitive lets the order follow the connection order,
 *    but still reversed due data_select() prepending
 *  - Step-wise manual selection would also be in reverse order
 * So it appears to be a good idea to reverse the processing order
 * In this function to minimize surprise.
 *
 * The result also currently depends on the direction of the connections.
 * When aligning two objects the one connected with HANDLE_MOVE_ENDPOINT
 * will be moved. This might be contradictory to the selection order.
 *
 * @param objects  selection of objects to be considered
 * @param dia      the diagram owning the objects (and holding undo information)
 * @param align    unused
 */
void
object_list_align_connected (GList *objects, Diagram *dia, int align)
{
  GList *list;
  Point *orig_pos;
  Point *dest_pos;
  DiaObject *obj, *o2;
  int i, nobjs;
  GList *connected = NULL;
  GList *to_be_moved = NULL;
  GList *movelist = NULL;

  /* find all elements to be moved directly */
  filter_connected (objects, 2, &connected, &to_be_moved);
  dia_log_message ("Moves %d - Connections %d", g_list_length (to_be_moved), g_list_length (connected));
  /* for every connection check:
   * - "matching" directions of both object connection points (this also gives
   *    the direction of the move of the second object)
   * -
   * - move every object only once
   */
  nobjs = g_list_length (to_be_moved);
  orig_pos = g_new (Point, nobjs);
  dest_pos = g_new (Point, nobjs);

  list = g_list_reverse (connected);
  while (list != NULL) {
    DiaObject *con = list->data;
    Handle *h1 = NULL, *h2 = NULL;

    g_assert (con->num_handles >= 2);
    for (i = 0; i < con->num_handles; ++i) {
      if (con->handles[i]->id == HANDLE_MOVE_STARTPOINT)
        h1 = con->handles[i];
      else if (con->handles[i]->id == HANDLE_MOVE_ENDPOINT)
        h2 = con->handles[i];
    }
    /* should this be an assert? */
    if (h1 && h2 && h1->connected_to && h2->connected_to) {
      ConnectionPoint *cps = h1->connected_to;
      ConnectionPoint *cpe = h2->connected_to;

      obj = cps->object;
      o2 = cpe->object;
      /* swap alignment direction if we are working backwards by the selection order */
      if (g_list_index (to_be_moved, obj) < g_list_index (to_be_moved, o2)) {
	DiaObject *otmp = o2;
	ConnectionPoint *cptmp = cpe;
	o2 = obj;
	obj = otmp;
	cpe = cps;
	cps = cptmp;
      }
      if (   !g_list_find (movelist, o2)
	  && g_list_find(to_be_moved, o2) && g_list_find(to_be_moved, obj)) {
        Point delta = {0, 0};
        /* If we haven't moved it yet, check if we want to */
	int hweight = 0;
	int vweight = 0;
	if (cps->directions == DIR_NORTH || cps->directions == DIR_SOUTH)
	  ++vweight;
	else if (cps->directions == DIR_EAST || cps->directions == DIR_WEST)
	  ++hweight;
	if (cpe->directions == DIR_NORTH || cpe->directions == DIR_SOUTH)
	  ++vweight;
	else if (cpe->directions == DIR_EAST || cpe->directions == DIR_WEST)
	  ++hweight;

	/* One clear directions is required to move at all */
        if (vweight > hweight) {
          /* horizontal move */
          delta.x = cps->pos.x - cpe->pos.x;
        } else if (hweight > vweight) {
          /* vertical move */
          delta.y = cps->pos.y - cpe->pos.y;
        } else {
          /* would need more context */
          char dirs[] = "NESW";
          int j;
          for (j = 0; j < 4; ++j) if (cps->directions & (1<<j)) g_print ("%c", dirs[j]);
          g_print ("(%s) -> ", obj->type->name);
          for (j = 0; j < 4; ++j) if (cpe->directions & (1<<j)) g_print ("%c", dirs[j]);
          g_print ("(%s)\n", o2->type->name);
        }
        if (delta.x != 0.0 || delta.y != 0) {
          Point pos = o2->position;

          point_add (&pos, &delta);

          i = g_list_length (movelist);
          orig_pos[i] = o2->position;
          dest_pos[i] = pos;

          dia_log_message ("Move '%s' by %g,%g", o2->type->name, delta.x, delta.y);
#if 0
          o2->ops->move (o2, &pos);
#else
          {
            GList *move_list = g_list_append (NULL, o2);
            object_list_move_delta (move_list, &delta);
            g_list_free (move_list);
          }
#endif
          diagram_update_connections_object (dia, o2, TRUE);
          movelist = g_list_append (movelist, o2);
        }
      }
    }

    list = g_list_next (list);
  }

  /* eating all the passed in parameters */
  dia_move_objects_change_new (dia, orig_pos, dest_pos, movelist);
  g_list_free (to_be_moved);
  g_list_free (connected);
}

/*!
 * \brief Move the list in the given direction.
 *
 * @param objects The objects to move
 * @param dia The diagram owning the objects
 * @param dir The direction to move to
 * @param step The step-width to move
 */
void
object_list_nudge(GList *objects, Diagram *dia, Direction dir, real step)
{
  Point *orig_pos;
  Point *dest_pos;
  guint nobjs, i;
  real inc_x, inc_y;
  GList *list;
  DiaObject *obj;

  if (!objects)
    return;
  g_return_if_fail (step > 0);

  nobjs = g_list_length (objects);
  g_return_if_fail (nobjs > 0);

  dest_pos = g_new(Point, nobjs);
  orig_pos = g_new(Point, nobjs);

  inc_x = DIR_RIGHT == dir ? step : (DIR_LEFT == dir ? -step : 0);
  inc_y = DIR_DOWN == dir ? step : (DIR_UP == dir ? -step : 0);

  /* remember original positions and calculate destination */
  i = 0;
  list = objects;
  while (list != NULL) {
    obj = (DiaObject *) list->data;

    orig_pos[i] = obj->position;
    dest_pos[i].x = orig_pos[i].x + inc_x;
    dest_pos[i].y = orig_pos[i].y + inc_y;

    obj->ops->move(obj, &dest_pos[i]);
    ++i;
    list = g_list_next(list);
  }
  /* if anything is connected not anymore */
  diagram_unconnect_selected(dia);
  dia_move_objects_change_new (dia, orig_pos, dest_pos, g_list_copy(objects));
}
