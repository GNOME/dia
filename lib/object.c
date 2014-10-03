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

#include <stdio.h>
#include <string.h>

#include "object.h"
#include "diagramdata.h" /* for Layer */
#include "message.h"
#include "parent.h"

#include "dummy_dep.h"

#include "debug.h"

/** Initialize an already allocated object with the given number of handles
 *  and connections.  This does not create the actual Handle and Connection
 *  objects, which are expected to be added later.
 * @param obj A newly allocated object with no handles or connections 
 *            previously allocated.
 * @param num_handles the number of handles to allocate room for.
 * @param num_connections the number of connections to allocate room for.
 */
void
object_init(DiaObject *obj,
	    int num_handles,
	    int num_connections)
{
  obj->num_handles = num_handles;
  if (num_handles>0)
    obj->handles = g_malloc0(sizeof(Handle *) * num_handles);
  else
    obj->handles = NULL;

  obj->num_connections = num_connections;
  if (num_connections>0)
    obj->connections = g_malloc0(sizeof(ConnectionPoint *) * num_connections);
  else
    obj->connections = NULL;
}

/** Destroy an objects allocations and disconnect it from everything else.
 *  After calling this function, the object is no longer valid for use
 *  in a diagram.  Note that this does not deallocate the object itself.
 * @param obj The object being destroyed.
 */
void
object_destroy(DiaObject *obj)
{
  object_unconnect_all(obj);
  
  if (obj->handles)
    g_free(obj->handles);
  obj->handles = NULL;
  if (obj->connections)
    g_free(obj->connections);
  obj->connections = NULL;
  if (obj->meta)
    g_hash_table_destroy (obj->meta);
  obj->meta = NULL;
}

/** Copy the object-level information of this object.
 *  This includes type, position, bounding box, number of handles and
 *  connections, operations, parentability, parent and children.
 *  After this copying you have to fix up:
    handles
    connections
    children/parents
 * In particular the children lists will contain the same objects, which
 * is not a valid situation.
 * @param from The object being copied from
 * @param to The object being copied to.  This object does not need to
 *           have been object_init'ed, but if it is, its handles and
 *           connections arrays will be freed.
 * @bug Any existing children list will not be freed and will leak.
 */
void
object_copy(DiaObject *from, DiaObject *to)
{
  to->type = from->type;
  to->position = from->position;
  to->bounding_box = from->bounding_box;
  
  to->num_handles = from->num_handles;
  if (to->handles != NULL) g_free(to->handles);
  if (to->num_handles>0)
    to->handles = g_malloc(sizeof(Handle *)*to->num_handles);
  else
    to->handles = NULL;

  to->num_connections = from->num_connections;
  if (to->connections != NULL) g_free(to->connections);
  if (to->num_connections>0)
    to->connections = g_malloc0(sizeof(ConnectionPoint *) * to->num_connections);
  else
    to->connections = NULL;

  to->ops = from->ops;

  to->parent = from->parent;
  to->children = g_list_copy(from->children);
}

/** A hash function of a pointer value.  Not the most well-spreadout
 * function, as it has the same low bits as the pointer.
 */
static guint
pointer_hash(gpointer some_pointer)
{
  return GPOINTER_TO_UINT(some_pointer);
}


/** Copy a list of objects, keeping connections and parent-children
 *  relation ships between the objects.  It is assumed that the 
 *  ops->copy function correctly creates the connections and handles
 *  objects.
 * @param list_orig The original list.  This list will not be changed,
 *                  nor will its objects.
 * @return A list with the list_orig copies copied.
 * @bug Any children of an object in the list that are not themselves
 *       in the list will cause a NULL entry in the children list.
 */
GList *
object_copy_list(GList *list_orig)
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

/** Move a number of objects the same distance.  Any children of objects in
 * the list are moved as well.  This is intended to be called from within
 * object_list_move_delta.
 * @param objects The list of objects to move.  This list must not contain
 *                any object that is a child (at any depth) of another object.
 *                @see parent_list_affected_hierarchy
 * @param delta How far to move the objects.
 * @param affected Whether to check parent boundaries???
 * @return Undo information for the move, or NULL if no objects moved.
 * @bug The return Change object only contains info for a single object.
 */
ObjectChange*
object_list_move_delta_r(GList *objects, Point *delta, gboolean affected)
{
  GList *list;
  DiaObject *obj;
  Point pos;
  ObjectChange *objchange = NULL;

  if (delta->x == 0 && delta->y == 0)
       return NULL;

  list = objects;
  while (list != NULL) {
    obj = (DiaObject *) list->data;
    
    pos = obj->position;
    point_add(&pos, delta);

    if (obj->parent && affected)
    {
      Rectangle p_ext;
      Rectangle c_ext;
      Point new_delta;

      parent_handle_extents(obj->parent, &p_ext);
      parent_handle_extents(obj, &c_ext);
      new_delta = parent_move_child_delta(&p_ext, &c_ext, delta);
      point_add(&pos, &new_delta);
      point_add(delta, &new_delta);
    }
    objchange = obj->ops->move(obj, &pos);

    if (object_flags_set(obj, DIA_OBJECT_CAN_PARENT) && obj->children)
      objchange = object_list_move_delta_r(obj->children, delta, FALSE);

    list = g_list_next(list);
  }
  return objchange;
}

/** Move a set of objects a given amount.
 * @param objects The list ob objects to move.
 * @param delta The amount to move them.
 */
ObjectChange*
object_list_move_delta(GList *objects, Point *delta)
{
  GList *list;
  DiaObject *obj;
  GList *process;
  ObjectChange *objchange = NULL;

  objects = parent_list_affected_hierarchy(objects);
  list = objects;
  /* The recursive function object_list_move_delta cannot process the toplevel
     (in selection) objects so we have to have this extra loop */
  while (list != NULL)
  {
    obj = (DiaObject *) list->data;

    process = NULL;
    process = g_list_append(process, obj);
    objchange = object_list_move_delta_r(process, delta, (obj->parent != NULL) );
    g_list_free(process);

    list = g_list_next(list);
  }
  return objchange;
}

typedef struct _ObjectChangeExchange
{
  ObjectChange change;
  DiaObject    *orig;
  DiaObject    *subst;
  gboolean      applied;
} ObjectChangeExchange;

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

void
object_copy_style (DiaObject *dest, const DiaObject *src)
{
  GPtrArray *props;

  g_return_if_fail (src && src->ops->get_props != NULL);
  g_return_if_fail (dest && dest->ops->set_props != NULL);

  props = prop_list_from_descs (_style_prop_descs, pdtpp_true);
  src->ops->get_props((DiaObject *)src, props);
  dest->ops->set_props(dest, props);
  prop_list_free(props);
}

static void
_object_exchange (ObjectChange *change, DiaObject *obj)
{
  ObjectChangeExchange *c = (ObjectChangeExchange *)change;
  Layer *layer = dia_object_get_parent_layer (obj);
  DiagramData *dia = layer ? layer_get_parent_diagram(layer) : NULL;
  DiaObject *subst = (obj == c->orig) ? c->subst : c->orig;
  DiaObject *parent_object = obj->parent;
  Handle *h1, *h2;
  int n1 = 0, n2 = 0;
  GPtrArray *props;
  int obj_index = 0;

  props = prop_list_from_descs (_style_prop_descs, pdtpp_true);
  /* removing from the diagram first, to have the right update areas */
  if (layer) {
    obj_index = layer_object_get_index (layer, obj);
    layer_remove_object (layer, obj);
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
    layer_add_object_at (layer, subst, obj_index);
    if (dia)
      data_select(dia, subst);
  }
}
/* It is adviced to not use the passed in object at all */
static void
_object_exchange_apply (ObjectChange *change, DiaObject *obj)
{
  ObjectChangeExchange *c = (ObjectChangeExchange *)change;

  g_return_if_fail (c->applied == 0);
  _object_exchange (change, c->orig);
  c->applied = 1;
}
/* It is adviced to not use the passed in object at all */
static void
_object_exchange_revert (ObjectChange *change, DiaObject *obj)
{
  ObjectChangeExchange *c = (ObjectChangeExchange *)change;

  g_return_if_fail (c->applied != 0);
  _object_exchange (change, c->subst);
  c->applied = 0;
}
static void
_object_exchange_free (ObjectChange *change)
{
  ObjectChangeExchange *c = (ObjectChangeExchange *)change;
  DiaObject *obj = c->applied ? c->orig : c->subst;

  if (obj) {
    obj->ops->destroy(obj);
    g_free(obj);
  }
}
/*!
 * \brief Replace an object with another one
 *
 * The type of an object can not change dynamically. To substitute one
 * object with another this function helps. It does it's best to transfer
 * all the existing object relations, e.g. connections, parent_layer 
 * and parenting information.
 *
 * @param obj   the original object which will be replace
 * @param subst the sunstitute object
 * @return      _ObjectChange containing undo/redo information
 */
ObjectChange *
object_substitute (DiaObject *obj, DiaObject *subst)
{
  ObjectChangeExchange *change = g_new0(ObjectChangeExchange, 1);

  change->change.apply  = _object_exchange_apply;
  change->change.revert = _object_exchange_revert;
  change->change.free   = _object_exchange_free;
  change->orig  = obj;
  change->subst = subst;

  _object_exchange_apply ((ObjectChange*)change, obj);

  return (ObjectChange*)change;
}

/** Destroy a list of objects by calling ops->destroy on each in turn.
 * @param list_to_be_destroyed A of objects list to destroy.  The list itself
 *                             will also be freed.
 */
void
destroy_object_list(GList *list_to_be_destroyed)
{
  GList *list;
  DiaObject *obj;
  
  list = list_to_be_destroyed;
  while (list != NULL) {
    obj = (DiaObject *)list->data;

    obj->ops->destroy(obj);
    g_free(obj);
    
    list = g_list_next(list);
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

/*!
 * \brief Add a new handle to an object at a given position.
 *
 * @param obj The object to add a handle to.  This object must have been
 *            object_init()ed.
 * @param handle The new handle to add.
 * @param pos Where in the list of handles (0 <= pos <= obj->num_handles) to
 *            add the handle.
 */
void
object_add_handle_at(DiaObject *obj, Handle *handle, int pos)
{
  int i;
  
  g_assert(0 <= pos && pos <= obj->num_handles);

  obj->num_handles++;

  obj->handles =
    g_realloc(obj->handles, obj->num_handles*sizeof(Handle *));

  for (i=obj->num_handles-1; i > pos; i--) {
    obj->handles[i] = obj->handles[i-1];
  }
  obj->handles[pos] = handle;
}

/** Remove a handle from an object.
 * @param obj The object to remove a handle from.
 * @param handle The handle to remove.  If the handle does not exist on this
 *               object, an error message is displayed.  The handle is not
 *               freed by this call.
 */
void
object_remove_handle(DiaObject *obj, Handle *handle)
{
  int i, handle_nr;

  handle_nr = -1;
  for (i=0;i<obj->num_handles;i++) {
    if (obj->handles[i] == handle)
      handle_nr = i;
  }

  if (handle_nr < 0) {
    message_error("Internal error, object_remove_handle: Handle doesn't exist");
    return;
  }

  for (i=handle_nr;i<(obj->num_handles-1);i++) {
    obj->handles[i] = obj->handles[i+1];
  }
  obj->handles[obj->num_handles-1] = NULL;
    
  obj->num_handles--;

  obj->handles =
    g_realloc(obj->handles, obj->num_handles*sizeof(Handle *));
}

/*!
 * \brief Add a new connection point to an object.
 * This is merely a convenience handler to add a connection point at the
 * end of an objects connection point list.
 * @see object_add_connectionpoint_at.
 */
void
object_add_connectionpoint(DiaObject *obj, ConnectionPoint *conpoint)
{
  object_add_connectionpoint_at(obj, conpoint, obj->num_connections);
}

/*!
 * \brief Add a new connectionpoint to an object.
 * @param obj The object to add the connectionpoint to.
 * @param conpoint The connectionpoiint to add.
 * @param pos Where in the list to add the connectionpoint 
 * (0 <= pos <= obj->num_connections).
 */
void 
object_add_connectionpoint_at(DiaObject *obj, 
			      ConnectionPoint *conpoint, int pos)
{
  int i;
  
  obj->num_connections++;

  obj->connections =
    g_realloc(obj->connections, 
	      obj->num_connections*sizeof(ConnectionPoint *));

  for (i=obj->num_connections-1; i > pos; i--) {
    obj->connections[i] = obj->connections[i-1];
  }
  obj->connections[pos] = conpoint;
}

/** Remove an existing connectionpoint from and object.
 * @param obj The object to remove the connectionpoint from.
 * @param conpoint The connectionpoint to remove.  The connectionpoint
 *                 will not be freed by this function, but any handles
 *                 connected to the connectionpoint will be
 *                 disconnected.
 *                 If the connectionpoint does not exist on the object, 
 *                 an error message is displayed. 
 */
void
object_remove_connectionpoint(DiaObject *obj, ConnectionPoint *conpoint)
{
  int i, nr;

  nr = -1;
  for (i=0;i<obj->num_connections;i++) {
    if (obj->connections[i] == conpoint)
      nr = i;
  }

  if (nr < 0) {
    message_error("Internal error, object_remove_connectionpoint: "
                  "ConnectionPoint doesn't exist");
    return;
  }

  object_remove_connections_to(conpoint);

  for (i=nr;i<(obj->num_connections-1);i++) {
    obj->connections[i] = obj->connections[i+1];
  }
  obj->connections[obj->num_connections-1] = NULL;
    
  obj->num_connections--;

  obj->connections =
    g_realloc(obj->connections, obj->num_connections*sizeof(ConnectionPoint *));
}


/** Make a connection between the handle and the connectionpoint.
 * @param obj The object having the handle.
 * @param handle The handle being connected.  This handle must not have
 *               connect_type HANDLE_NONCONNECTABLE, or an incomprehensible
 *               error message is displayed to the user.
 * @param connectionpoint The connection point to connect to.
 */
void
object_connect(DiaObject *obj, Handle *handle,
	       ConnectionPoint *connectionpoint)
{
  g_return_if_fail (obj && obj->type && obj->type->name);
  g_return_if_fail (connectionpoint && connectionpoint->object && 
                    connectionpoint->object->type && connectionpoint->object->type->name);
  if (handle->connect_type==HANDLE_NONCONNECTABLE) {
    message_error("Error? trying to connect a non connectable handle.\n"
                  "'%s' -> '%s'\n"
		  "Check this out...\n",
		  obj->type->name,
		  connectionpoint->object->type->name);
    return;
  }
  handle->connected_to = connectionpoint;
  connectionpoint->connected =
    g_list_prepend(connectionpoint->connected, obj);
}

/** Disconnect handle from whatever it may be connected to.
 * @param connected_obj The object having the handle.
 * @param handle The handle to disconnect
 */
void
object_unconnect(DiaObject *connected_obj, Handle *handle)
{
  ConnectionPoint *connectionpoint;

  connectionpoint = handle->connected_to;

  if (connectionpoint!=NULL) {
    connectionpoint->connected =
      g_list_remove(connectionpoint->connected, connected_obj);
    handle->connected_to = NULL;
  }
}

/** Remove all connections to the given connectionpoint.
 * After this call, the connectionpoints connected field will be NULL,
 * the list will have been freed, and no handles will be connected to the
 * connectionpoint.
 * @param conpoint A connectionpoint.
 */
void
object_remove_connections_to(ConnectionPoint *conpoint)
{
  GList *list;
  DiaObject *connected_obj;
  int i;
  
  list = conpoint->connected;
  while (list != NULL) {
    connected_obj = (DiaObject *)list->data;

    for (i=0;i<connected_obj->num_handles;i++) {
      if (connected_obj->handles[i]->connected_to == conpoint) {
	connected_obj->handles[i]->connected_to = NULL;
      }
    }
    list = g_list_next(list);
  }
  g_list_free(conpoint->connected);
  conpoint->connected = NULL;
}

/** Remove all connections to and from an object.
 * @param obj An object to disconnect from all connectionpoints and handles.
 */
void
object_unconnect_all(DiaObject *obj)
{
  int i;
  
  for (i=0;i<obj->num_handles;i++) {
    object_unconnect(obj, obj->handles[i]);
  }
  for (i=0;i<obj->num_connections;i++) {
    object_remove_connections_to(obj->connections[i]);
  }
}

/** Save the object-specific parts of an object.
 *  Note that this does not save the attributes of an object, merely the
 *  basic data (currently position and bounding box).
 * @param obj An object to save.
 * @param obj_node An XML node to save the data to.
 * @param ctx The context to transport error information.
 */
void 
object_save(DiaObject *obj, ObjectNode obj_node, DiaContext *ctx)
{
  data_add_point(new_attribute(obj_node, "obj_pos"),
		 &obj->position, ctx);
  data_add_rectangle(new_attribute(obj_node, "obj_bb"),
		     &obj->bounding_box, ctx);
  if (obj->meta && g_hash_table_size (obj->meta) > 0)
    data_add_dict (new_attribute(obj_node, "meta"), obj->meta, ctx);
}

/** Load the object-specific parts of an object.
 *  Note that this does not load the attributes of an object, merely the
 *  basic data (currently position and bounding box).
 * @param obj An object to load into.
 * @param obj_node An XML node to load the data from.
 * @param ctx The context in which this function is called
 */
void 
object_load(DiaObject *obj, ObjectNode obj_node, DiaContext *ctx)
{
  AttributeNode attr;

  obj->position.x = 0.0;
  obj->position.y = 0.0;
  attr = object_find_attribute(obj_node, "obj_pos");
  if (attr != NULL)
    data_point(attribute_first_data(attr), &obj->position, ctx);

  obj->bounding_box.left = obj->bounding_box.right = 0.0;
  obj->bounding_box.top = obj->bounding_box.bottom = 0.0;
  attr = object_find_attribute(obj_node, "obj_bb");
  if (attr != NULL)
    data_rectangle(attribute_first_data(attr), &obj->bounding_box, ctx);

  attr = object_find_attribute(obj_node, "meta");
  if (attr != NULL)
    obj->meta = data_dict (attribute_first_data(attr), ctx);
}

/** Returns the layer that the given object belongs to.
 * @param obj An object.
 * @return The layer that contains the object, or NULL if the object is
 * not in any layer.
 */
Layer *
dia_object_get_parent_layer(DiaObject *obj) {
  return obj->parent_layer;
}

/** Returns true if `obj' is currently selected.
 * Note that this takes time proportional to the number of selected
 * objects, so don't use it frivolously.
 * Note that if the objects is not in a layer (e.g. grouped), it is never
 * considered selected.
 * @param obj An object to test for selectedness.
 * @return TRUE if the object is selected.
 */
gboolean
dia_object_is_selected (const DiaObject *obj)
{
  Layer *layer = obj->parent_layer;
  DiagramData *diagram = layer ? layer->parent_diagram : NULL;
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
  for (; selected != NULL; selected = g_list_next(selected)) {
    if (selected->data == obj) return TRUE;
  }
  return FALSE;
}

/** Return the top-most object in the parent chain that has the given
 * flags set.
 * @param obj An object to start at.  If this is NULL, NULL is returned.
 * @param flags The flags to check.  If 0, the top-most parent is returned.
 * If more than one flag is given, the top-most parent that has all the given
 * flags set is returned.
 * @returns An object that either has all the flags set or
 * is obj itself.  It is guaranteed that no parent of this object has all the
 * given flags set.
 */
DiaObject *
dia_object_get_parent_with_flags(DiaObject *obj, guint flags)
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

/** Utility function: Checks if an objects can be selected.
 * Reasons for not being selectable include:
 * Being inside a closed group.
 * Being in a non-active layer.
 *
 * @param obj An object to test.
 * @returns TRUE if the object is not currently selected.
 */
gboolean
dia_object_is_selectable(DiaObject *obj)
{
  if (obj->parent_layer == NULL) {
    return FALSE;
  }
  return obj->parent_layer == obj->parent_layer->parent_diagram->active_layer;
}


/****** DiaObject register: **********/

static guint hash(gpointer key)
{
  char *string = (char *)key;
  int sum;

  sum = 0;
  while (*string) {
    sum += (*string);
    string++;
  }

  return sum;
}

static gint compare(gpointer a, gpointer b)
{
  return strcmp((char *)a, (char *)b)==0;
}

static GHashTable *object_type_table = NULL;

/** Initialize the object registry. */
void
object_registry_init(void)
{
  object_type_table = g_hash_table_new( (GHashFunc) hash, (GCompareFunc) compare );
}

/** Register the type of an object.
 *  This should be called as part of dia_plugin_init calls in modules that
 *  define objects for sheets.  If an object type with the given name is
 *  already registered (typically due to a user saving a local copy), a
 *  warning is display to the user.
 * @param type The type information.
 */
void
object_register_type(DiaObjectType *type)
{
  if (g_hash_table_lookup(object_type_table, type->name) != NULL) {
    message_warning("Several object-types were named %s.\n"
		    "Only first one will be used.\n"
		    "Some things might not work as expected.\n",
		    type->name);
    return;
  }
  g_hash_table_insert(object_type_table, type->name, type);
}


/** Performs a function on each registered object type.
 * @param func A function foo(DiaObjectType, gpointer) to call.
 * @param user_data Data passed through to the functions.
 */
void
object_registry_foreach (GHFunc func, gpointer user_data)
{
  g_hash_table_foreach (object_type_table, func, user_data);
}

/** Get the object type information associated with a name.
 * @param name A type name.
 * @return A DiaObjectType for an object type with the given name, or 
 *         NULL if no such type is registered.
 */
DiaObjectType *
object_get_type(char *name)
{
  return (DiaObjectType *) g_hash_table_lookup(object_type_table, name);
}

/** True if all the given flags are set, false otherwise.
 * @param obj An object to test.
 * @param flags Flags to check if they are set.  See definitions in object.h
 * @return TRUE if all the flags given are set on the object.
 */
gboolean
object_flags_set(DiaObject *obj, gint flags)
{
  return (obj->type->flags & flags) == flags;
}

/** Load an object from XML based on its properties.
 *  This function is suitable for implementing the object load function
 *  for an object with normal attributes.  Any version-dependent handling
 *  should be done after calling this function.
 * @param type The type of the object, used for creation.
 * @param obj_node The XML node defining the object.
 * @param version The version of the object found in the XML structure.
 * @param ctx The context in which this function is called
 * @return A newly created object with properties loaded.
 */
DiaObject *
object_load_using_properties(const DiaObjectType *type,
                             ObjectNode obj_node, int version,
                             DiaContext *ctx)
{
  DiaObject *obj;
  Point startpoint = {0.0,0.0};
  Handle *handle1,*handle2;
  
  obj = type->ops->create(&startpoint,NULL, &handle1,&handle2);
  object_load_props(obj,obj_node,ctx);
  return obj;
}

/** Save an object into an XML structure based on its properties.
 *  This function is suitable for implementing the object save function
 *  for an object with normal attributes.
 * @param obj The object to save.
 * @param obj_node The XML structure to save into.
 * @param ctx The context to transport error information.
 */
void 
object_save_using_properties(DiaObject *obj, ObjectNode obj_node, 
                             DiaContext *ctx)
{
  object_save_props (obj, obj_node, ctx);
}

/** Copy an object based solely on its properties.
 *  This function is suitable for implementing the object save function
 *  for an object with normal attributes.
 * @param obj An object to copy.
 */
DiaObject *object_copy_using_properties(DiaObject *obj)
{
  Point startpoint = {0.0,0.0};
  Handle *handle1,*handle2;
  DiaObject *newobj = obj->type->ops->create(&startpoint,NULL,
                                          &handle1,&handle2);
  object_copy_props(newobj,obj,FALSE);
  return newobj;
}

/** Return a box that all 'real' parts of the object is bounded by.
 *  In most cases, this is the same as the enclosing box, but things like
 *  bezier controls would lie outside of this.
 * @param obj The object to get the bounding box for.
 * @return A pointer to a Rectangle object.  This object should *not*
 *  be freed after use, as it belongs to the object.
 */
const Rectangle *
dia_object_get_bounding_box(const DiaObject *obj) {
  return &obj->bounding_box;
}

/** Return a box that encloses all interactively rendered parts of the object.
 * @param obj The object to get the enclosing box for.
 * @return A pointer to a Rectangle object.  This object should *not*
 *  be freed after use, as it belongs to the object.
 */
const Rectangle *
dia_object_get_enclosing_box(const DiaObject *obj)
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
