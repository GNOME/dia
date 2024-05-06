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

#include <stdio.h>
#include <string.h>

#include <xpm-pixbuf.h>

#define _DIA_OBJECT_BUILD 1
#include "object.h"
#include "diagramdata.h" /* for Layer */
#include "message.h"
#include "parent.h"
#include "dia-layer.h"

#include "debug.h"

/**
 * object_init:
 * @obj: A newly allocated object with no handles or connections
 *       previously allocated.
 * @num_handles: the number of handles to allocate room for.
 * @num_connections: the number of connections to allocate room for.
 *
 * Initialize an already allocated object with the given number of handles
 * and connections.  This does not create the actual #Handle and #Connection
 * objects, which are expected to be added later.
 */
void
object_init (DiaObject *obj,
             int        num_handles,
             int        num_connections)
{
  obj->num_handles = num_handles;
  obj->handles = g_new0 (Handle *, num_handles);

  obj->num_connections = num_connections;
  obj->connections = g_new0 (ConnectionPoint *, num_connections);
}


/**
 * object_destroy:
 * @obj: The object being destroyed.
 *
 * Destroy an objects allocations and disconnect it from everything else.
 * After calling this function, the object is no longer valid for use
 * in a diagram.  Note that this does not deallocate the object itself.
 */
void
object_destroy (DiaObject *obj)
{
  object_unconnect_all (obj);

  g_clear_pointer (&obj->handles, g_free);
  g_clear_pointer (&obj->connections, g_free);
  if (obj->meta)
    g_hash_table_destroy (obj->meta);
  obj->meta = NULL;
}

/**
 * object_copy:
 * @from: The object being copied from
 * @to: The object being copied to.  This object does not need to
 *      have been object_init'ed, but if it is, its handles and
 *      connections arrays will be freed.
 *
 * Copy the object-level information of this object.
 *
 * This includes type, position, bounding box, number of handles and
 * connections, operations, parentability, parent and children.
 *
 * After this copying you have to fix up:
 *
 *  - #Handle s
 *  - #Connection s
 *  - children / parents
 *
 * In particular the children lists will contain the same objects, which
 * is not a valid situation.
 *
 * bug Any existing children list will not be freed and will leak.
 */
void
object_copy (DiaObject *from, DiaObject *to)
{
  to->type = from->type;
  to->position = from->position;
  to->bounding_box = from->bounding_box;

  to->num_handles = from->num_handles;
  g_clear_pointer (&to->handles, g_free);
  if (to->num_handles > 0)
    to->handles = g_new0 (Handle *, to->num_handles);
  else
    to->handles = NULL;

  to->num_connections = from->num_connections;
  g_clear_pointer (&to->connections, g_free);
  if (to->num_connections > 0)
    to->connections = g_new0 (ConnectionPoint *, to->num_connections);
  else
    to->connections = NULL;

  to->ops = from->ops;

  to->parent = from->parent;
  to->children = g_list_copy (from->children);
}

/**
 * pointer_hash:
 * @some_pointer: pointer to hash
 *
 * A hash function of a pointer value.  Not the most well-spreadout
 * function, as it has the same low bits as the pointer.
 */
static guint
pointer_hash(gpointer some_pointer)
{
  return GPOINTER_TO_UINT(some_pointer);
}


/**
 * object_copy_list:
 * @list_orig: The original list.  This list will not be changed,
 *             nor will its objects.
 *
 * Copy a list of objects, keeping connections and parent-children
 * relation ships between the objects.  It is assumed that the
 * ops->copy function correctly creates the connections and handles
 * objects.
 *
 * Returns: A list with the list_orig copies copied.
 *
 * bug Any children of an object in the list that are not themselves
 *       in the list will cause a NULL entry in the children list.
 */
GList *
object_copy_list (GList *list_orig)
{
  GList *list_copy;
  GList *list;
  DiaObject *obj;
  DiaObject *obj_copy;
  GHashTable *hash_table;
  int i;

  hash_table = g_hash_table_new((GHashFunc) pointer_hash, NULL);

  /* First ops->copy the entire list */
  list = list_orig;
  list_copy = NULL;
  while (list != NULL) {
    obj = (DiaObject *)list->data;

    obj_copy = obj->ops->copy(obj);

    g_hash_table_insert(hash_table, obj, obj_copy);

    list_copy = g_list_append(list_copy, obj_copy);

    list = g_list_next(list);
  }

  /* Rebuild the connections and parent/child references between the
  objects in the list: */
  list = list_orig;
  while (list != NULL) {
    obj = (DiaObject *)list->data;
    obj_copy = g_hash_table_lookup(hash_table, obj);

    if (obj_copy->parent)
      obj_copy->parent = g_hash_table_lookup(hash_table, obj_copy->parent);

    if (object_flags_set(obj_copy, DIA_OBJECT_CAN_PARENT)
	&& obj_copy->children)
    {
      GList *child_list = obj_copy->children;
      while(child_list)
      {
        DiaObject *child_obj = (DiaObject *) child_list->data;
        child_list->data = g_hash_table_lookup(hash_table, child_obj);
	child_list = g_list_next(child_list);
      }
    }

    for (i=0;i<obj->num_handles;i++) {
      ConnectionPoint *con_point;
      con_point = obj->handles[i]->connected_to;

      if ( con_point != NULL ) {
	DiaObject *other_obj;
	DiaObject *other_obj_copy;
	int con_point_nr;

	other_obj = con_point->object;
	other_obj_copy = g_hash_table_lookup(hash_table, other_obj);

	if (other_obj_copy == NULL) {
	  /* Ensure we have no dangling connection to avoid crashing, on
	   * object_unconnect() e.g. bug #497070. Two questions remaining:
	   *  - shouldn't the object::copy() have initialized this to NULL?
	   *  - could we completely solve this by looking deeper into groups?
	   *    The sample from #497070 has nested groups but this function currently
	   *    works on one level at the time. Thus the object within the group are
	   *    invisible when we try to restore the groups connectons. BUT the
	   *    connectionpoints in the group are shared with the connectionpoints
	   *    of the inner objects ...
	   */
	  obj_copy->handles[i]->connected_to = NULL;
	  break; /* other object was not on list. */
	}

	con_point_nr=0;
	while (other_obj->connections[con_point_nr] != con_point) {
	  con_point_nr++;
	}

	object_connect(obj_copy, obj_copy->handles[i],
		       other_obj_copy->connections[con_point_nr]);
      }
    }

    list = g_list_next(list);
  }

  g_hash_table_destroy(hash_table);

  return list_copy;
}

/**
 * object_list_move_delta_r:
 * @objects: The list of objects to move.  This list must not contain
 *           any object that is a child (at any depth) of another object.
 *           see parent_list_affected_hierarchy()
 * @delta: How far to move the objects.
 * @affected: Whether to check parent boundaries???
 *
 * Move a number of objects the same distance.  Any children of objects in
 * the list are moved as well.  This is intended to be called from within
 * object_list_move_delta().
 *
 * Returns: Undo information for the move, or %NULL if no objects moved.
 *
 * bug The return Change object only contains info for a single object.
 */
DiaObjectChange *
object_list_move_delta_r (GList *objects, Point *delta, gboolean affected)
{
  GList *list;
  DiaObject *obj;
  Point pos;
  DiaObjectChange *objchange = NULL;

  if (delta->x == 0 && delta->y == 0)
       return NULL;

  list = objects;
  while (list != NULL) {
    obj = DIA_OBJECT (list->data);

    pos = obj->position;
    point_add (&pos, delta);

    if (obj->parent && affected) {
      DiaRectangle p_ext;
      DiaRectangle c_ext;
      Point new_delta;

      parent_handle_extents (obj->parent, &p_ext);
      parent_handle_extents (obj, &c_ext);
      new_delta = parent_move_child_delta (&p_ext, &c_ext, delta);
      point_add (&pos, &new_delta);
      point_add (delta, &new_delta);
    }
    objchange = dia_object_move (obj, &pos);

    if (object_flags_set (obj, DIA_OBJECT_CAN_PARENT) && obj->children) {
      objchange = object_list_move_delta_r (obj->children, delta, FALSE);
    }

    list = g_list_next (list);
  }

  return objchange;
}


/**
 * object_list_move_delta:
 * @objects: The list ob objects to move.
 * @delta: The amount to move them.
 *
 * Move a set of objects a given amount.
 */
DiaObjectChange *
object_list_move_delta (GList *objects, Point *delta)
{
  GList *list;
  DiaObject *obj;
  GList *process;
  DiaObjectChange *objchange = NULL;

  objects = parent_list_affected_hierarchy (objects);
  list = objects;
  /* The recursive function object_list_move_delta cannot process the toplevel
     (in selection) objects so we have to have this extra loop */
  while (list != NULL) {
    obj = DIA_OBJECT (list->data);

    process = NULL;
    process = g_list_append (process, obj);
    objchange = object_list_move_delta_r (process, delta, (obj->parent != NULL));
    g_list_free (process);

    list = g_list_next (list);
  }

  return objchange;
}


struct _DiaExchangeObjectChange {
  DiaObjectChange  change;
  DiaObject       *orig;
  DiaObject       *subst;
  gboolean         applied;
};


DIA_DEFINE_OBJECT_CHANGE (DiaExchangeObjectChange,
                          dia_exchange_object_change)


static Handle *
_find_connectable (DiaObject *obj, int *num)
{
  int dir = *num >= 0 ? 1 : -1;
  int n = (*num >= 0 ? *num : -*num);
  for (; n < obj->num_handles && n >= 0; n+=dir) {
    if (obj->handles[n]->connect_type!=HANDLE_NONCONNECTABLE) {
      *num = n;
      return obj->handles[n];
    }
  }
  return NULL;
}

/*!
 * Bezierlines don't have just two connectable handles, but every
 * major handle is connectable. To let us find the correct handles
 * for connection transfer this function checks if there is
 * a higher handle index connectable.
 */
static int
_more_connectable (DiaObject *obj, int num)
{
  int n;
  for (n = num+1; n < obj->num_handles; ++n) {
    if (obj->handles[n]->connect_type!=HANDLE_NONCONNECTABLE)
      return TRUE;
  }
  return FALSE;
}
static PropDescription _style_prop_descs[] = {
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_LINE_STYLE,
  PROP_STD_LINE_JOIN,
  PROP_STD_LINE_CAPS,
  PROP_STD_FILL_COLOUR,
  PROP_STD_SHOW_BACKGROUND,
#if 0 /* not this way */
  PROP_STD_TEXT,
#else
  PROP_STD_TEXT_ALIGNMENT,
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  PROP_STD_TEXT_FITTING,
  PROP_STD_PATTERN,
#endif
  PROP_DESC_END
};

/**
 * object_copy_style:
 * @dest: the object to copy onto
 * @src: the object to copy from
 */
void
object_copy_style (DiaObject *dest, const DiaObject *src)
{
  GPtrArray *props;

  g_return_if_fail (src && src->ops->get_props != NULL);
  g_return_if_fail (dest && dest->ops->set_props != NULL);

  props = prop_list_from_descs (_style_prop_descs, pdtpp_true);
  dia_object_get_properties ((DiaObject *) src, props);
  dia_object_set_properties (dest, props);
  prop_list_free (props);
}


static void
_object_exchange (DiaObjectChange *self, DiaObject *obj)
{
  DiaExchangeObjectChange *c = DIA_EXCHANGE_OBJECT_CHANGE (self);
  DiaLayer *layer = dia_object_get_parent_layer (obj);
  DiagramData *dia = layer ? dia_layer_get_parent_diagram (layer) : NULL;
  DiaObject *subst = (obj == c->orig) ? c->subst : c->orig;
  DiaObject *parent_object = obj->parent;
  Handle *h1, *h2;
  int n1 = 0, n2 = 0;
  GPtrArray *props;
  int obj_index = 0;

  props = prop_list_from_descs (_style_prop_descs, pdtpp_true);
  /* removing from the diagram first, to have the right update areas */
  if (layer) {
    obj_index = dia_layer_object_get_index (layer, obj);
    dia_layer_remove_object (layer, obj);
    if (dia)
      data_unselect(dia, obj);
  }
  if (obj->ops->get_props)
    obj->ops->get_props(obj, props);
  /* transfer connections where possible - first find the right handles */
  h1 = _find_connectable (obj, &n1);
  h2 = _find_connectable (subst, &n2);
  while (h1 && h2) {
    /* The connection point of the other object - beware self connections */
    ConnectionPoint *cp = h1->connected_to;

    if (cp) {
      h2->pos = h1->pos;
      object_unconnect (obj, h1);
      object_connect (subst, h2, cp);
      /* make the object update it's data - e.g. autorouting */
      subst->ops->move_handle(subst, h2, &h2->pos, cp, HANDLE_MOVE_CONNECTED, 0);
    }
    ++n1;
    h1 = _find_connectable (obj, &n1);
    /* if we are at the end of the input line change search direction to backward */
    if (h1 && !_more_connectable(obj, n1))
      n2 = -(subst->num_handles-1);
    else
      ++n2;
    h2 = _find_connectable (subst, &n2);
  }
  /* disconnect the rest - sorry: no undo for that */
  object_unconnect_all (obj);
  /* transfer parenting information */
  if (parent_object) {
    GList *sibling = g_list_find (parent_object->children, obj);
    parent_object->children = g_list_insert_before (parent_object->children, sibling, subst);
    parent_object->children = g_list_remove (parent_object->children, obj);
  }
  /* apply style properties - only if it's not restore of original */
  if (subst->ops->get_props && subst != c->orig)
    subst->ops->set_props(subst, props);
  prop_list_free(props);
  /* adding to the diagram last, to have the right update areas */
  if (layer) {
    dia_layer_add_object_at (layer, subst, obj_index);
    if (dia)
      data_select(dia, subst);
  }
}


/* It is adviced to not use the passed in object at all */
static void
dia_exchange_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  DiaExchangeObjectChange *c = DIA_EXCHANGE_OBJECT_CHANGE (self);

  g_return_if_fail (c->applied == 0);
  _object_exchange (self, c->orig);
  c->applied = 1;
}


/* It is adviced to not use the passed in object at all */
static void
dia_exchange_object_change_revert (DiaObjectChange *self, DiaObject *obj)
{
  DiaExchangeObjectChange *c = DIA_EXCHANGE_OBJECT_CHANGE (self);

  g_return_if_fail (c->applied != 0);
  _object_exchange (self, c->subst);
  c->applied = 0;
}


static void
dia_exchange_object_change_free (DiaObjectChange *self)
{
  DiaExchangeObjectChange *c = DIA_EXCHANGE_OBJECT_CHANGE (self);
  DiaObject *obj = c->applied ? c->orig : c->subst;

  if (obj) {
    obj->ops->destroy (obj);
    g_clear_pointer (&obj, g_free);
  }
}


/**
 * object_substitute:
 * @obj: the original object which will be replace
 * @subst: the sunstitute object
 *
 * Replace an object with another one
 *
 * The type of an object can not change dynamically. To substitute one
 * object with another this function helps. It does it's best to transfer
 * all the existing object relations, e.g. connections, parent_layer
 * and parenting information.
 *
 * Returns: #DiaObjectChange containing undo/redo information
 */
DiaObjectChange *
object_substitute (DiaObject *obj, DiaObject *subst)
{
  DiaExchangeObjectChange *change;

  change = dia_object_change_new (DIA_TYPE_EXCHANGE_OBJECT_CHANGE);

  change->orig  = obj;
  change->subst = subst;

  dia_object_change_apply (DIA_OBJECT_CHANGE (change), obj);

  return DIA_OBJECT_CHANGE (change);
}


/**
 * destroy_object_list:
 * @list_to_be_destroyed: A of objects list to destroy.  The list itself
 *                        will also be freed.
 *
 * Destroy a list of objects by calling ops->destroy on each in turn.
 *
 * Since: dawn-of-time
 */
void
destroy_object_list (GList *list_to_be_destroyed)
{
  GList *list;
  DiaObject *obj;

  list = list_to_be_destroyed;
  while (list != NULL) {
    obj = (DiaObject *)list->data;

    obj->ops->destroy (obj);
    g_clear_pointer (&obj, g_free);

    list = g_list_next (list);
  }

  g_list_free(list_to_be_destroyed);
}

/*!
 * \brief Add a new handle to an object.
 *
 * This is merely a utility wrapper around object_add_handle_at().
 * @param obj The object to add a handle to.  This object must have been
 *            object_init()ed.
 * @param handle The new handle to add.  The handle will be inserted as the
 *               last handle in the list.
 */
void
object_add_handle(DiaObject *obj, Handle *handle)
{
  object_add_handle_at(obj, handle, obj->num_handles);
}


/**
 * object_add_handle_at:
 * @obj; The object to add a handle to. This object must have been
 *       object_init()ed.
 * @handle: The new handle to add.
 * @pos: Where in the list of handles (0 <= pos <= obj->num_handles) to
 *       add the handle.
 *
 * Add a new handle to an object at a given position.
 */
void
object_add_handle_at (DiaObject *obj, Handle *handle, int pos)
{
  int i;

  g_return_if_fail (0 <= pos && pos <= obj->num_handles);

  obj->num_handles++;

  obj->handles = g_renew (Handle *, obj->handles, obj->num_handles);

  for (i = obj->num_handles - 1; i > pos; i--) {
    obj->handles[i] = obj->handles[i - 1];
  }
  obj->handles[pos] = handle;
}


/**
 * object_remove_handle:
 * @obj: The object to remove a handle from.
 * @handle: The handle to remove.  If the handle does not exist on this
 *          object, an error message is displayed.  The handle is not
 *          freed by this call.
 *
 * Remove a handle from an object.
 */
void
object_remove_handle (DiaObject *obj, Handle *handle)
{
  int i, handle_nr;

  handle_nr = -1;
  for (i = 0; i < obj->num_handles; i++) {
    if (obj->handles[i] == handle) {
      handle_nr = i;
    }
  }

  if (handle_nr < 0) {
    message_error ("Internal error, object_remove_handle: Handle doesn't exist");
    return;
  }

  for (i = handle_nr; i < (obj->num_handles - 1); i++) {
    obj->handles[i] = obj->handles[i + 1];
  }
  obj->handles[obj->num_handles - 1] = NULL;

  obj->num_handles--;

  obj->handles = g_renew (Handle *, obj->handles, obj->num_handles);
}


/**
 * object_add_connectionpoint:
 *
 * Add a new connection point to an object.
 *
 * This is merely a convenience handler to add a connection point at the
 * end of an objects connection point list.
 *
 * See object_add_connectionpoint_at().
 */
void
object_add_connectionpoint (DiaObject *obj, ConnectionPoint *conpoint)
{
  object_add_connectionpoint_at (obj, conpoint, obj->num_connections);
}


/**
 * object_add_connectionpoint_at:
 * @obj: The object to add the connectionpoint to.
 * @conpoint: The connectionpoiint to add.
 * @pos: Where in the list to add the connectionpoint
 *       (0 <= pos <= obj->num_connections).
 *
 * Add a new connectionpoint to an object.
 */
void
object_add_connectionpoint_at (DiaObject       *obj,
                               ConnectionPoint *conpoint,
                               int              pos)
{
  int i;

  obj->num_connections++;

  obj->connections = g_renew (ConnectionPoint *,
                              obj->connections,
                              obj->num_connections);

  for (i = obj->num_connections - 1; i > pos; i--) {
    obj->connections[i] = obj->connections[i - 1];
  }
  obj->connections[pos] = conpoint;
}


/**
 * object_remove_connectionpoint:
 * @obj: The object to remove the connectionpoint from.
 * @conpoint: The connectionpoint to remove.  The connectionpoint
 *            will not be freed by this function, but any handles
 *            connected to the connectionpoint will be disconnected.
 *            If the connectionpoint does not exist on the object,
 *            an error message is displayed.
 *
 * Remove an existing connectionpoint from and object.
 */
void
object_remove_connectionpoint (DiaObject *obj, ConnectionPoint *conpoint)
{
  int i, nr;

  nr = -1;
  for (i = 0; i < obj->num_connections; i++) {
    if (obj->connections[i] == conpoint) {
      nr = i;
    }
  }

  if (nr < 0) {
    message_error ("Internal error, object_remove_connectionpoint: "
                   "ConnectionPoint doesn't exist");
    return;
  }

  object_remove_connections_to (conpoint);

  for (i = nr; i < (obj->num_connections - 1); i++) {
    obj->connections[i] = obj->connections[i + 1];
  }
  obj->connections[obj->num_connections - 1] = NULL;

  obj->num_connections--;

  obj->connections = g_renew (ConnectionPoint *,
                              obj->connections,
                              obj->num_connections);
}


/**
 * object_connect:
 * @obj: The object having the handle.
 * @handle: The handle being connected.  This handle must not have
 *          connect_type %HANDLE_NONCONNECTABLE, or an incomprehensible
 *          error message is displayed to the user.
 * @connectionpoint: The connection point to connect to.
 *
 * Make a connection between the handle and the connectionpoint.
 */
void
object_connect (DiaObject       *obj,
                Handle          *handle,
                ConnectionPoint *connectionpoint)
{
  g_return_if_fail (obj && obj->type && obj->type->name);
  g_return_if_fail (connectionpoint && connectionpoint->object &&
                    connectionpoint->object->type && connectionpoint->object->type->name);

  if (handle->connect_type==HANDLE_NONCONNECTABLE) {
    message_error ("Error? trying to connect a non connectable handle.\n"
                   "'%s' -> '%s'\n"
                   "Check this out...\n",
                   obj->type->name,
                   connectionpoint->object->type->name);
    return;
  }
  handle->connected_to = connectionpoint;
  connectionpoint->connected =
    g_list_prepend (connectionpoint->connected, obj);
}


/**
 * object_unconnect:
 * @connected_obj: The object having the handle.
 * @handle: The handle to disconnect
 *
 * Disconnect handle from whatever it may be connected to.
 */
void
object_unconnect (DiaObject *connected_obj, Handle *handle)
{
  ConnectionPoint *connectionpoint;

  connectionpoint = handle->connected_to;

  if (connectionpoint!=NULL) {
    connectionpoint->connected =
      g_list_remove (connectionpoint->connected, connected_obj);
    handle->connected_to = NULL;
  }
}


/**
 * object_remove_connections_to:
 * @conpoint: A #ConnectionPoint
 *
 * Remove all connections to the given connectionpoint.
 * After this call, the connectionpoints connected field will be NULL,
 * the list will have been freed, and no handles will be connected to the
 * connectionpoint.
 */
void
object_remove_connections_to (ConnectionPoint *conpoint)
{
  GList *list;
  DiaObject *connected_obj;
  int i;

  list = conpoint->connected;
  while (list != NULL) {
    connected_obj = (DiaObject *)list->data;

    for (i = 0; i < connected_obj->num_handles; i++) {
      if (connected_obj->handles[i]->connected_to == conpoint) {
        connected_obj->handles[i]->connected_to = NULL;
      }
    }
    list = g_list_next (list);
  }
  g_list_free (conpoint->connected);
  conpoint->connected = NULL;
}


/**
 * object_unconnect_all:
 * @obj: An object to disconnect from all connectionpoints and handles.
 *
 * Remove all connections to and from an object.
 */
void
object_unconnect_all (DiaObject *obj)
{
  int i;

  for (i = 0; i < obj->num_handles; i++) {
    object_unconnect (obj, obj->handles[i]);
  }

  for (i = 0; i < obj->num_connections; i++) {
    object_remove_connections_to (obj->connections[i]);
  }
}


/**
 * object_save:
 * @obj: An object to save.
 * @obj_node: An XML node to save the data to.
 * @ctx: The context to transport error information.
 *
 * Save the object-specific parts of an object.
 * Note that this does not save the attributes of an object, merely the
 * basic data (currently position and bounding box).
 */
void
object_save (DiaObject *obj, ObjectNode obj_node, DiaContext *ctx)
{
  data_add_point (new_attribute (obj_node, "obj_pos"),
                  &obj->position,
                  ctx);
  data_add_rectangle (new_attribute (obj_node, "obj_bb"),
                      &obj->bounding_box,
                      ctx);
  if (obj->meta && g_hash_table_size (obj->meta) > 0) {
    data_add_dict (new_attribute (obj_node, "meta"), obj->meta, ctx);
  }
}


/**
 * object_load:
 * @obj: An object to load into.
 * @obj_node: An XML node to load the data from.
 * @ctx: The context in which this function is called
 *
 * Load the object-specific parts of an object.
 * Note that this does not load the attributes of an object, merely the
 * basic data (currently position and bounding box).
 */
void
object_load (DiaObject *obj, ObjectNode obj_node, DiaContext *ctx)
{
  AttributeNode attr;

  obj->position.x = 0.0;
  obj->position.y = 0.0;
  attr = object_find_attribute (obj_node, "obj_pos");
  if (attr != NULL) {
    data_point (attribute_first_data (attr), &obj->position, ctx);
  }

  obj->bounding_box.left = obj->bounding_box.right = 0.0;
  obj->bounding_box.top = obj->bounding_box.bottom = 0.0;
  attr = object_find_attribute (obj_node, "obj_bb");
  if (attr != NULL) {
    data_rectangle (attribute_first_data (attr), &obj->bounding_box, ctx);
  }

  attr = object_find_attribute (obj_node, "meta");
  if (attr != NULL) {
    obj->meta = data_dict (attribute_first_data (attr), ctx);
  }
}


/**
 * dia_object_get_parent_layer:
 * @obj: An object.
 *
 * Returns the layer that the given object belongs to.
 *
 * Returns: The layer that contains the object, or %NULL if the object is
 * not in any layer.
 */
DiaLayer *
dia_object_get_parent_layer (DiaObject *obj)
{
  return obj->parent_layer;
}


/**
 * dia_object_is_selected:
 * @obj: An object to test for selectedness.
 *
 * Returns true if `obj' is currently selected.
 *
 * Note that this takes time proportional to the number of selected
 * objects, so don't use it frivolously.
 *
 * Note that if the objects is not in a layer (e.g. grouped), it is never
 * considered selected.
 *
 * Returns: %TRUE if the object is selected, else %FALSE
 */
gboolean
dia_object_is_selected (const DiaObject *obj)
{
  DiaLayer *layer = obj->parent_layer;
  DiagramData *diagram = layer ? dia_layer_get_parent_diagram (layer) : NULL;
  GList * selected;

  /* although this is a little bogus, it is better than crashing
   * It appears as if neither group members nor "parented" objects do have their
   * parent_layer set (but they aren't selected either, are they ? --hb
   * No, grouped objects at least aren't selectable, but they may need
   * to test selectedness when rendering beziers.  Parented objects are
   * a different thing, though. */
  if (!diagram)
    return FALSE;

  selected = diagram->selected;
  for (; selected != NULL; selected = g_list_next (selected)) {
    if (selected->data == obj) {
      return TRUE;
    }
  }

  return FALSE;
}


/**
 * dia_object_get_parent_with_flags:
 * @obj: An object to start at. If this is %NULL, %NULL is returned.
 * @flags: The flags to check. If 0, the top-most parent is returned.
 * If more than one flag is given, the top-most parent that has all the given
 * flags set is returned.
 *
 * Return the top-most object in the parent chain that has the given
 * flags set.
 *
 * Returns: An object that either has all the flags set or is obj itself. It
 * is guaranteed that no parent of this object has all the given flags set.
 */
DiaObject *
dia_object_get_parent_with_flags (DiaObject *obj, guint flags)
{
  DiaObject *top = obj;

  if (obj == NULL) {
    return NULL;
  }

  while (obj->parent != NULL) {
    obj = obj->parent;
    if ((obj->type->flags & flags) == flags) {
      top = obj;
    }
  }

  return top;
}


/**
 * dia_object_is_selectable:
 * @obj: An object to test
 *
 * Utility function: Checks if an objects can be selected.
 * Reasons for not being selectable include:
 * Being inside a closed group.
 * Being in a non-active layer.
 *
 * Returns: %TRUE if the object is not currently selected, else %FALSE
 */
gboolean
dia_object_is_selectable (DiaObject *obj)
{
  if (obj->parent_layer == NULL) {
    return FALSE;
  }

  return obj->parent_layer == dia_diagram_data_get_active_layer (dia_layer_get_parent_diagram (obj->parent_layer));
}


/****** DiaObject register: **********/


static GHashTable *object_type_table = NULL;

/* Initialize the object registry. */
void
object_registry_init (void)
{
  object_type_table = g_hash_table_new (g_str_hash, g_str_equal);
}


/**
 * object_register_type:
 * @type: The type information.
 *
 * Register the type of an object.
 *
 * This should be called as part of dia_plugin_init calls in modules that
 * define objects for sheets. If an object type with the given name is
 * already registered (typically due to a user saving a local copy), a
 * warning is display to the user.
 */
void
object_register_type (DiaObjectType *type)
{
  if (g_hash_table_lookup (object_type_table, type->name) != NULL) {
    message_warning ("Several object-types were named %s.\n"
                     "Only first one will be used.\n"
                     "Some things might not work as expected.\n",
                     type->name);
    return;
  }
  g_hash_table_insert (object_type_table, type->name, type);
}


/**
 * object_registry_foreach:
 * @func: A function foo(DiaObjectType, gpointer) to call.
 * @user_data: Data passed through to the functions.
 *
 * Performs a function on each registered object type.
 */
void
object_registry_foreach (GHFunc func, gpointer user_data)
{
  g_hash_table_foreach (object_type_table, func, user_data);
}


/**
 * object_get_type:
 * @name: A type name.
 *
 * Get the object type information associated with a name.
 *
 * Returns: A #DiaObjectType for an object type with the given name, or
 *          %NULL if no such type is registered.
 *
 * Since: dawn-of-time
 */
DiaObjectType *
object_get_type (char *name)
{
  /* FIXME: this was added here to get some visibility.  Idealy we should have a common way
   * of ensuring g_hash_table_lookup return a non-NULL. */
  DiaObjectType *type = (DiaObjectType *) g_hash_table_lookup (object_type_table, name);
  if (type == NULL) {
    g_warning ("Unable to find object type: %s", name);
  }
  return type;
}


/**
 * object_flags_set:
 * @obj: An object to test.
 * @flags: Flags to check if they are set.  See definitions in object.h
 *
 * True if all the given flags are set, false otherwise.
 *
 * Returns: %TRUE if all the flags given are set on the object.
 *
 * Since: dawn-of-time
 */
gboolean
object_flags_set (DiaObject *obj, int flags)
{
  return (obj->type->flags & flags) == flags;
}


/**
 * object_load_using_properties:
 * @type: The type of the object, used for creation.
 * @obj_node: The XML node defining the object.
 * @version: The version of the object found in the XML structure.
 * @ctx: The context in which this function is called
 *
 * Load an object from XML based on its properties.
 *
 * This function is suitable for implementing the object load function
 * for an object with normal attributes. Any version-dependent handling
 * should be done after calling this function.
 *
 * Returns: A newly created object with properties loaded.
 *
 * Since: dawn-of-time
 */
DiaObject *
object_load_using_properties (const DiaObjectType *type,
                              ObjectNode           obj_node,
                              int                  version,
                              DiaContext          *ctx)
{
  DiaObject *obj;
  Point startpoint = {0.0,0.0};
  Handle *handle1,*handle2;

  obj = type->ops->create (&startpoint,NULL, &handle1, &handle2);
  object_load_props (obj,obj_node,ctx);
  return obj;
}


/**
 * object_save_using_properties:
 * @obj: The object to save.
 * @obj_node: The XML structure to save into.
 * @ctx: The context to transport error information.
 *
 * Save an object into an XML structure based on its properties.
 * This function is suitable for implementing the object save function
 * for an object with normal attributes.
 *
 * Since: dawn-of-time
 */
void
object_save_using_properties (DiaObject  *obj,
                              ObjectNode  obj_node,
                              DiaContext *ctx)
{
  object_save_props (obj, obj_node, ctx);
}


/**
 * object_copy_using_properties:
 * @obj: An object to copy.
 *
 * Copy an object based solely on its properties.
 *
 * This function is suitable for implementing the object save function
 * for an object with normal attributes.
 *
 * Since: dawn-of-time
 */
DiaObject *
object_copy_using_properties (DiaObject *obj)
{
  Point startpoint = {0.0,0.0};
  Handle *handle1,*handle2;
  DiaObject *newobj = obj->type->ops->create(&startpoint,NULL,
                                          &handle1,&handle2);
  object_copy_props(newobj,obj,FALSE);
  return newobj;
}

/**
 * dia_object_get_bounding_box:
 * @obj: The object to get the bounding box for.
 *
 * Return a box that all 'real' parts of the object is bounded by.
 *  In most cases, this is the same as the enclosing box, but things like
 *  bezier controls would lie outside of this.
 *
 * Returns: A pointer to a #DiaRectangle object.  This object should *not*
 *  be freed after use, as it belongs to the object.
 */
const DiaRectangle *
dia_object_get_bounding_box (const DiaObject *obj)
{
  return &obj->bounding_box;
}

/**
 * dia_object_get_enclosing_box:
 * @obj: The object to get the enclosing box for.
 *
 * Return a box that encloses all interactively rendered parts of the object.
 *
 * Returns: A pointer to a #DiaRectangle object.  This object should *not*
 *  be freed after use, as it belongs to the object.
 */
const DiaRectangle *
dia_object_get_enclosing_box (const DiaObject *obj)
{
  if (!obj->enclosing_box)
    return &obj->bounding_box;
  else
    return obj->enclosing_box;
}

void
dia_object_set_meta (DiaObject *obj, const gchar *key, const gchar *value)
{
  g_return_if_fail (obj != NULL && key != NULL);
  if (!obj->meta)
    obj->meta = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  if (value)
    g_hash_table_insert (obj->meta, g_strdup (key), g_strdup (value));
  else
    g_hash_table_remove (obj->meta, key);
}

gchar *
dia_object_get_meta (DiaObject *obj, const gchar *key)
{
  gchar *val;
  if (!obj->meta)
    return NULL;
  val = g_hash_table_lookup (obj->meta, key);
  return g_strdup (val);
}

/* The below are for debugging purposes only. */

/** Check that a DiaObject maintains its invariants and constrains.
 * @param obj An object to check
 * @param msg Comment on the sanity
 * @return TRUE if the object is OK. */
gboolean
dia_object_sanity_check(const DiaObject *obj, const gchar *msg) {
  int i;
  /* Check the type */
  dia_assert_true(obj->type != NULL,
		  "%s: Object %p has null type\n",
		  msg, obj);
  if (obj != NULL) {
    dia_assert_true(obj->type->name != NULL &&
		    g_utf8_validate(obj->type->name, -1, NULL),
		    "%s: Object %p has illegal type name '%s'\n",
		    msg, obj, obj->type->name);
    /* Check the position vs. the bounding box */
    /* Check the handles */
    dia_assert_true(obj->num_handles >= 0,
		    "%s: Object %p has < 0 (%d) handles\n",
		    msg, obj,  obj->num_handles);
    if (obj->num_handles != 0) {
      dia_assert_true(obj->handles != NULL,
		      "%s: Object %p has null handles\n", msg, obj);
    }
    for (i = 0; i < obj->num_handles; i++) {
      Handle *h = obj->handles[i];
      dia_assert_true(h != NULL, "%s: Object %p handle %d is null\n",
		      msg, obj, i);
      if (h != NULL) {
	/* Check handle id */
	dia_assert_true((h->id <= HANDLE_MOVE_ENDPOINT)
			|| (h->id >= HANDLE_CUSTOM1 && h->id <= HANDLE_CUSTOM9),
			"%s: Object %p handle %d (%p) has wrong id %d\n",
			msg, obj, i, h, h->id);
	/* Check handle type */
	dia_assert_true(h->type <= NUM_HANDLE_TYPES,
			"%s: Object %p handle %d (%p) has wrong type %d\n",
			msg, obj, i, h, h->type);
	/* Check handle pos is legal pos */
	/* Check connect type is legal */
	dia_assert_true(h->connect_type <= HANDLE_CONNECTABLE_NOBREAK,
			"%s: Object %p handle %d (%p) has wrong connect type %d\n",
			msg, obj, i, h, h->connect_type);
	/* Check that if connected, connection makes sense */
	do { /* do...while(FALSE) to make aborting easy */
	  ConnectionPoint *cp = h->connected_to;
	  if (cp != NULL) {
	    gboolean found = FALSE;
	    GList *conns;
	    if (!dia_assert_true(cp->object != NULL,
				 "%s: Handle %d (%p) on object %p connects to CP %p with NULL object\n",
				 msg, i, h, obj, cp)) break;
	    if (!dia_assert_true(cp->object->type != NULL,
				 "%s:  Handle %d (%p) on object %p connects to CP %p with untyped object %p\n",
				 msg, i, h, obj, cp, cp->object)) break;
	    if (!dia_assert_true(cp->object->type->name != NULL &&
				 g_utf8_validate(cp->object->type->name, -1, NULL),
				 "%s:  Handle %d (%p) on object %p connects to CP %p with untyped object %p\n",
				 msg, i, h, obj, cp, cp->object)) break;
	    dia_assert_true(fabs(cp->pos.x - h->pos.x) < 0.0000001 &&
			    fabs(cp->pos.y - h->pos.y) < 0.0000001,
			    "%s: Handle %d (%p) on object %p has pos %f, %f,\nbut its CP %p of object %p has pos %f, %f\n",
			    msg, i, h, obj, h->pos.x, h->pos.y,
			    cp, cp->object, cp->pos.x, cp->pos.y);
	    for (conns = cp->connected; conns != NULL; conns = g_list_next(conns)) {
	      DiaObject *obj2 = (DiaObject *)conns->data;
	      int j;

	      for (j = 0; j < obj2->num_handles; j++) {
		if (obj2->handles[j]->connected_to == cp) found = TRUE;
	      }
	    }
	    dia_assert_true(found == TRUE,
			    "%s: Handle %d (%p) on object %p is connected to %p on object %p, but is not in its connect list\n",
			    msg, i, h, obj, cp, cp->object);
	  }
	} while (FALSE);
      }
    }
    /* Check the connections */
    dia_assert_true(obj->num_connections >= 0,
		    "%s: Object %p has < 0 (%d) connection points\n",
		    msg, obj, obj->num_connections);
    if (obj->num_connections != 0) {
      dia_assert_true(obj->connections != NULL,
		      "%s: Object %p has NULL connections array\n",
		      msg, obj);
    }
    for (i = 0; i < obj->num_connections; i++) {
      GList *connected;
      ConnectionPoint *cp = obj->connections[i];
      int j;
      dia_assert_true(cp != NULL, "%s: Object %p has null CP at %d\n", msg, obj, i);
      if (cp != NULL) {
	dia_assert_true(cp->object == obj,
			"%s: Object %p CP %d (%p) points to other obj %p\n",
			msg, obj, i, cp, cp->object);
	dia_assert_true((cp->directions & (~DIR_ALL)) == 0,
			"%s: Object %p CP %d (%p) has illegal directions %d\n",
			msg, obj, i, cp, cp->directions);
	dia_assert_true((cp->flags & CP_FLAGS_MAIN) == cp->flags,
			"%s: Object %p CP %d (%p) has illegal flags %d\n",
			msg, obj, i, cp, cp->flags);
	j = 0;
	for (connected = cp->connected; connected != NULL;
	     connected = g_list_next(connected)) {
	  DiaObject *obj2;
	  obj2 = connected->data;
	  dia_assert_true(obj2 != NULL, "%s: Object %p CP %d (%p) has NULL object at index %d\n",
			  msg, obj, i, cp, j);
	  if (obj2 != NULL) {
	    int k;
	    gboolean found_handle = FALSE;
	    dia_assert_true(obj2->type->name != NULL &&
			    g_utf8_validate(obj2->type->name, -1, NULL),
			    "%s: Object %p CP %d (%p) connected to untyped object %p (%s) at index %d\n",
			    msg, obj, i, cp, obj2, obj2->type->name, j);
	    /* Check that connected object points back to this CP */
	    for (k = 0; k < obj2->num_handles; k++) {
	      if (obj2->handles[k] != NULL &&
		  obj2->handles[k]->connected_to == cp) {
		found_handle = TRUE;
	      }
	    }
	    dia_assert_true(found_handle,
			    "%s: Object %p CP %d (%p) connected to %p (%s) at index %d, but no handle points back\n",
			    msg, obj, i, cp, obj2, obj2->type->name, j);
	  }
	  j++;
	}
      }
    }
    /* Check the children */
  }
  return TRUE;
}

GdkPixbuf *
dia_object_type_get_icon (const DiaObjectType *type)
{
  GdkPixbuf *pixbuf;
  const gchar **icon_data = NULL;

  if (type == NULL) {
    return NULL;
  }

  icon_data = type->pixmap;

  if (icon_data == NULL && type->pixmap_file == NULL) {
    return NULL;
  }

  if (g_str_has_prefix ((char *) icon_data, "res:")) {
    pixbuf = pixbuf_from_resource (((char *) icon_data) + 4);
  } else if (type->pixmap_file != NULL) {
    GError *error = NULL;
    pixbuf = gdk_pixbuf_new_from_file (type->pixmap_file, &error);
    if (error) {
      g_warning ("%s", error->message);
      g_clear_error (&error);
    }
  } else {
    pixbuf = xpm_pixbuf_load (icon_data);
  }

  return pixbuf;
}

/**
 * dia_object_draw:
 * @self: The object to draw.
 * @renderer: The #DiaRenderer object to draw with.
 *
 * Function responsible for drawing the object.
 *
 * Every drawing must be done through the use of the Renderer, so that we
 * can render the picture on screen, in an eps file, ...
 *
 * Stability: Stable
 *
 * Since: 0.98
 */
void
dia_object_draw (DiaObject   *self,
                 DiaRenderer *renderer)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->ops->draw != NULL);

  self->ops->draw (self, renderer);
}

/**
 * dia_object_distance_from:
 * @self: The object.
 * @point: A #Point to give the distance to.
 *
 * Calculate the distance between the #DiaObject and the #Point.
 *
 * Several functions are provided in geometry.h to facilitate this calculus.
 *
 * Returns: The distance from the point to the nearest part of the object.
 *          If the point is inside a closed object, return 0.0.
 *
 * Stability: Stable
 *
 * Since: 0.98
 */
double
dia_object_distance_from (DiaObject *self,
                          Point     *point)
{
  g_return_val_if_fail (self != NULL, 0.0);
  g_return_val_if_fail (self->ops->distance_from != NULL, 0.0);

  return self->ops->distance_from (self, point);
}


/**
 * dia_object_select:
 * @self: An object that is being selected.
 * @point: is the point on the screen where the user has clicked
 * @renderer: is a renderer that has some extra functions
 *           most notably the possibility to get EXACT
 *           measures of strings. Used to place cursors
 *           and other interactive stuff.
 *           (Don't draw to the renderer)
 *
 * Activate the selected state of the #DiaObject
 *
 * Function called once the object has been selected.
 * Basically, this function should update the object (position of the
 * handles,...)
 * This function should not redraw the object.
 *
 * Stability: Stable
 *
 * Since: 0.98
 */
void
dia_object_select (DiaObject   *self,
                   Point       *point,
                   DiaRenderer *renderer)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->ops->selectf != NULL);

  self->ops->selectf (self, point, renderer);
}

/**
 * dia_object_clone:
 * @self: An object to make a copy of.
 *
 * Copy constructor of #DiaObject.
 *
 * This must be an depth-copy (pointers must be duplicated and so on)
 * as the initial object can be deleted any time.
 *
 * Returns: A newly allocated object copied from @self, but without any
 *          connections to other objects.
 *
 * Stability: Stable
 *
 * Since: 0.98
 */
DiaObject *
dia_object_clone (DiaObject *self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (self->ops->copy != NULL, NULL);

  return self->ops->copy (self);
}


/**
 * dia_object_move:
 * @self: The object being moved.
 * @to: Where the object is being moved to.
 *      Its exact definition depends on the object. It is the point on the
 *      object that 'snaps' to the grid if that is enabled. (generally it
 *      is the upper left corner)
 *
 * Function called to move the entire object.
 *
 * Returns: A #DiaObjectChange with additional undo information, or
 *          (in most cases) %NULL. Undo for moving the object itself is
 *          handled elsewhere.
 *
 * Stability: Stable
 *
 * Since: 0.98
 */
DiaObjectChange *
dia_object_move (DiaObject *self,
                 Point     *to)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (self->ops->move != NULL, NULL);

  return self->ops->move (self, to);
}


/**
 * dia_object_move_handle:
 * @self: The object whose handle is being moved.
 * @handle: The handle being moved.
 * @to: The position it has been moved to (corrected for
 *      vertical/horizontal only movement).
 * @cp: If non-%NULL, the connectionpoint found at this position.
 *      If @a cp is %NULL, there may or may not be a connectionpoint.
 * @reason: The reason the handle was moved.
 * - %HANDLE_MOVE_USER means the user is dragging the point.
 * - %HANDLE_MOVE_USER_FINAL means the user let go of the point.
 * - %HANDLE_MOVE_CONNECTED means it was moved because something
 *   it was connected to moved.
 * - %HANDLE_MOVE_CREATE_FINAL: is given for resizing during creation
 *   None of the given reasons is a reason to decline movement, typical
 *   object implementations can safely ignore this parameter.
 * @modifiers: gives a bitset of modifier keys currently held down
 * - %MODIFIER_SHIFT is either shift key
 * - %MODIFIER_ALT is either alt key
 * - %MODIFIER_CONTROL is either control key
 * Each has MODIFIER_LEFT_* and MODIFIER_RIGHT_* variants
 *
 * Function called to move one of the handles associated with the object.
 *
 * Returns: A #DiaObjectChange with additional undo information, or
 *          (in most cases) %NULL. Undo for moving the handle itself is handled
 *          elsewhere.
 *
 * Stability: Stable
 *
 * Since: 0.98
 */
DiaObjectChange *
dia_object_move_handle (DiaObject              *self,
                        Handle                 *handle,
                        Point                  *to,
                        ConnectionPoint        *cp,
                        HandleMoveReason        reason,
                        ModifierKeys            modifiers)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (self->ops->move_handle != NULL, NULL);

  return self->ops->move_handle (self, handle, to, cp, reason, modifiers);
}


/**
 * dia_object_get_editor:
 * @self: An obj that this dialog is being made for.
 * @is_default: If %TRUE, this dialog is for object defaults, and
 *              the toolbox options should not be shown.
 *
 * Function called when the user has double clicked on an DiaObject.
 *
 * When this function is called and the dialog already is created,
 * make sure to update the values in the widgets so that it
 * accurately describes the current state of the object.
 * Remember to destroy this dialog when the object is destroyed!

 * Note that if you want to use the same dialog multiple times,
 * you should ref it first.  Just run the following on the widget
 * when you create it:
 *   g_object_ref_sink(widget);
 * If you don't do this, the widget will be destroyed when the
 * properties dialog is closed.
 *
 * Returns: A dialog to edit the properties of the object.
 *
 * Stability: Stable
 *
 * Since: 0.98
 */
GtkWidget *
dia_object_get_editor (DiaObject *self,
                       gboolean   is_default)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (self->ops->get_properties != NULL, NULL);

  return self->ops->get_properties (self, is_default);
}


/**
 * dia_object_apply_editor:
 * @self: The object whose dialog has had its Apply button clicked.
 * @editor: The properties dialog being applied.
 *
 * Function is called when the user clicks on the "Apply" button.
 *
 * The widget parameter is the one created by
 * the get_properties function.
 *
 * Returns: a #DiaObjectChange that can be used for undo/redo, The returned
 *          change is already applied.
 *
 * Stability: Stable
 *
 * Since: 0.98
 */
DiaObjectChange *
dia_object_apply_editor (DiaObject *self,
                         GtkWidget *editor)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (self->ops->apply_properties_from_dialog != NULL, NULL);

  return self->ops->apply_properties_from_dialog (self, editor);
}


/**
 * dia_object_get_menu:
 * @self: The object that is selected when the object menu is asked for.
 * @at: Where the user clicked. This can be used to place whatever
 *      the menu point may create, such as new segment corners.
 *
 * Return an object-specific menu with toggles etc. properly set.
 *
 * Returns: A menu description with values set appropriately for this object.
 * The description object must not be freed by the caller.
 *
 * Stability: Stable
 *
 * Since: 0.98
 */
DiaMenu *
dia_object_get_menu (DiaObject *self,
                     Point     *at)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (self->ops->get_object_menu != NULL, NULL);

  return self->ops->get_object_menu (self, at);
}

/**
 * dia_object_describe_properties:
 * @self: The object whose properties we want described.
 *
 * Describe the properties that this object supports.
 *
 * Returns: a %NULL-terminated array of property descriptions.
 * As the const return implies the returned data is not owned by the
 * caller. If this function returns a dynamically created description,
 * then DestroyFunc must free the description.
 *
 * Stability: Stable
 *
 * Since: 0.98
 */
const PropDescription *
dia_object_describe_properties (DiaObject *self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (self->ops->describe_props != NULL, NULL);

  return self->ops->describe_props (self);
}

/**
 * dia_object_get_properties:
 * @self: An object that delivers the values.
 * @list: (out): A list of #Property objects whose values are to be set based
 *         on the objects internal data. The types for the objects are
 *         also being set as a side-effect.
 *
 * Get the actual values of the properties given.
 *
 * Note that the props array need not contain all the properties
 * defined for the object, nor do all the properties in the array need be
 * defined for the object. All properties in the props array that are
 * actually set will be set.
 *
 * Stability: Stable
 *
 * Since: 0.98
 */
void
dia_object_get_properties (DiaObject *self,
                           GPtrArray *list)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->ops->get_props != NULL);

  self->ops->get_props (self, list);
}

/**
 * dia_object_set_properties:
 * @self: An object to update values on.
 * @list: An array of #Property objects whose values are to be set on
 *        the object.
 *
 * Set the object to have the values defined in the properties list.
 *
 * Note that the props array may contain more or fewer properties than the
 * object defines, but only and all the ones defined for the object will
 * be applied to the object.
 *
 * Stability: Stable
 *
 * Since: 0.98
 */
void
dia_object_set_properties (DiaObject *self,
                           GPtrArray *list)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->ops->set_props != NULL);

  self->ops->set_props (self, list);
}


/**
 * dia_object_apply_properties:
 * @self: The object to which properties are to be applied
 * @list: The list of properties that are to be applied
 *
 * Function used to apply a list of properties to the object.
 *
 * It is typically called by ApplyPropertiesDialogFunc. This
 * is different from SetPropsFunc since this is used to implement
 * undo/redo.
 *
 * Returns: a #DiaObjectChange for undo/redo
 *
 * Stability: Stable
 *
 * Since: 0.98
 */
DiaObjectChange *
dia_object_apply_properties (DiaObject *self,
                             GPtrArray *list)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (self->ops->apply_properties_list != NULL, NULL);

  return self->ops->apply_properties_list (self, list);
}

/**
 * dia_object_edit_text:
 * @self: The self object
 * @text: The text entry being edited
 * @state: The state of the editing, either %TEXT_EDIT_START,
 * %TEXT_EDIT_INSERT, %TEXT_EDIT_DELETE, or %TEXT_EDIT_END.
 * @textchange: For %TEXT_EDIT_INSERT, the text about to be inserted.
 * For %TEXT_EDIT_DELETE, the text about to be deleted.
 *
 * Update the text part of an object
 *
 * This function, if not null, will be called every time the text is changed
 * or editing starts or stops.
 *
 * Returns: For %TEXT_EDIT_INSERT and %TEXT_EDIT_DELETE, %TRUE this change
 * will be allowed, %FALSE otherwise. For %TEXT_EDIT_START and %TEXT_EDIT_END,
 * the return value is ignored.
 *
 * Stability: Stable
 *
 * Since: 0.98
 */
gboolean
dia_object_edit_text (DiaObject     *self,
                      Text          *text,
                      TextEditState  state,
                      gchar         *textchange)
{
  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (self->ops->edit_text != NULL, FALSE);

  return self->ops->edit_text (self, text, state, textchange);
}

/**
 * dia_object_transform:
 * @self: Explicit this pointer
 * @m: The transformation matrix
 *
 * Transform the object with the given matrix
 *
 * This function - if not null - will apply the transformation matrix to the
 * object. It should be implemented for every standard object, because it's
 * main use-case is the support of transformations from SVG.
 *
 * Returns: %TRUE if the matrix can be applied to the object, %FALSE otherwise.
 *
 * Stability: Stable
 *
 * Since: 0.98
 */
gboolean
dia_object_transform (DiaObject       *self,
                      const DiaMatrix *m)
{
  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (self->ops->transform != NULL, FALSE);

  return self->ops->transform (self, m);
}


/**
 * dia_object_add_handle:
 * @self: the #DiaObject
 * @handle: the #Handle to register
 * @index: the index of @handle
 * @id: the #HandleId of @handle
 * @type: the #HandleType of @handle
 * @connect_type: the @HandleConnectType of @handle
 *
 * Register @handle with @self, for use in object creation
 *
 * This SHOULD NOT be called on arbitary objects
 *
 * Stability: Stable
 *
 * Since: 0.98
 */
void
dia_object_add_handle (DiaObject               *self,
                       Handle                  *handle,
                       int                      index,
                       HandleId                 id,
                       HandleType               type,
                       HandleConnectType        connect_type)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (handle != NULL);
  g_return_if_fail (index >= 0 && index < self->num_handles);

  self->handles[index] = handle;

  handle->id = id;
  handle->type = type;
  handle->connect_type = connect_type;
  handle->connected_to = NULL;
}


/**
 * dia_object_add_connection_point:
 * @self: the #DiaObject
 * @cp: the #ConnectionPoint to register
 * @index: the index of @cp
 * @flags: the #ConnectionPointFlags for flags
 *
 * Register @cp with @self, for use in object creation
 *
 * This SHOULD NOT be called on arbitary objects
 *
 * Stability: Stable
 *
 * Since: 0.98
 */
void
dia_object_add_connection_point (DiaObject            *self,
                                 ConnectionPoint      *cp,
                                 int                   index,
                                 ConnectionPointFlags  flags)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (cp != NULL);
  g_return_if_fail (index >= 0 && index < self->num_connections);

  self->connections[index] = cp;

  cp->object = self;
  cp->connected = NULL;
  cp->flags = flags;
}
