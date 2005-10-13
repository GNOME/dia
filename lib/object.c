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

#include <gtk/gtk.h>

#include "object.h"
#include "message.h"

#include "dummy_dep.h"

#include "debug.h"

void object_init(DiaObject *obj, int num_handles, int num_connections);

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

void
object_destroy(DiaObject *obj)
{
  object_unconnect_all(obj);
  
  if (obj->handles)
    g_free(obj->handles);

  if (obj->connections)
    g_free(obj->connections);

}


/* After this copying you have to fix up:
   handles
   connections
   children/parents
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

  to->can_parent = from->can_parent;
  to->parent = from->parent;;
  to->children = g_list_copy(from->children);
}

static guint
pointer_hash(gpointer some_pointer)
{
  return (guint) some_pointer;
}

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

    if (obj_copy->can_parent && obj_copy->children)
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

	if (other_obj_copy == NULL)
	  break; /* other object was not on list. */
	
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

    if (obj->can_parent && obj->children)
      objchange = object_list_move_delta_r(obj->children, delta, FALSE);

    list = g_list_next(list);
  }
  return objchange;
}

extern ObjectChange*
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

void
object_add_handle(DiaObject *obj, Handle *handle)
{
  obj->num_handles++;

  obj->handles =
    g_realloc(obj->handles, obj->num_handles*sizeof(Handle *));
  
  obj->handles[obj->num_handles-1] = handle;
}

void
object_add_handle_at(DiaObject *obj, Handle *handle, int pos)
{
  int i;
  
  obj->num_handles++;

  obj->handles =
    g_realloc(obj->handles, obj->num_handles*sizeof(Handle *));

  for (i=obj->num_handles-1; i > pos; i--) {
    obj->handles[i] = obj->handles[i-1];
  }
  obj->handles[pos] = handle;
}

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

void
object_add_connectionpoint(DiaObject *obj, ConnectionPoint *conpoint)
{
  obj->num_connections++;

  obj->connections =
    g_realloc(obj->connections, 
	      obj->num_connections*sizeof(ConnectionPoint *));
  
  obj->connections[obj->num_connections-1] = conpoint;
}

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

void
object_remove_connectionpoint(DiaObject *obj, ConnectionPoint *conpoint)
{
  int i, nr;

  object_remove_connections_to(conpoint);

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

  for (i=nr;i<(obj->num_connections-1);i++) {
    obj->connections[i] = obj->connections[i+1];
  }
  obj->connections[obj->num_connections-1] = NULL;
    
  obj->num_connections--;

  obj->connections =
    g_realloc(obj->connections, obj->num_connections*sizeof(ConnectionPoint *));
}


void
object_connect(DiaObject *obj, Handle *handle,
	       ConnectionPoint *connectionpoint)
{
  if (handle->connect_type==HANDLE_NONCONNECTABLE) {
    message_error("Error? trying to connect a non connectable handle.\n"
		  "Check this out...\n");
    return;
  }
  handle->connected_to = connectionpoint;
  connectionpoint->connected =
    g_list_prepend(connectionpoint->connected, obj);
}

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

void object_save(DiaObject *obj, ObjectNode obj_node)
{
  data_add_point(new_attribute(obj_node, "obj_pos"),
		 &obj->position);
  data_add_rectangle(new_attribute(obj_node, "obj_bb"),
		     &obj->bounding_box);
}

void object_load(DiaObject *obj, ObjectNode obj_node)
{
  AttributeNode attr;

  obj->position.x = 0.0;
  obj->position.y = 0.0;
  attr = object_find_attribute(obj_node, "obj_pos");
  if (attr != NULL)
    data_point( attribute_first_data(attr), &obj->position );

  obj->bounding_box.left = obj->bounding_box.right = 0.0;
  obj->bounding_box.top = obj->bounding_box.bottom = 0.0;
  attr = object_find_attribute(obj_node, "obj_bb");
  if (attr != NULL)
    data_rectangle( attribute_first_data(attr), &obj->bounding_box );
}

Layer *dia_object_get_parent_layer(DiaObject *obj) {
  return obj->parent_layer;
}

/** Returns true if `obj' is currently selected.
 * Note that this takes time proportional to the number of selected
 * objects, so don't use it frivolously.
 * Note that if the objects is not in a layer (e.g. grouped), it is never
 * considered selected.
 */
gboolean
dia_object_is_selected (const DiaObject *obj)
{
  Layer *layer = obj->parent_layer;
  DiagramData *diagram = layer ? layer->parent_diagram : NULL;
  GList * selected;

  /* although this is a little bogus, it is better than crashing
   * It appears as if neither group members nor "parented" objects do have their 
   * parent_layer set (but they aren't slected either, are they ? --hb
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

void
object_registry_init(void)
{
  object_type_table = g_hash_table_new( (GHashFunc) hash, (GCompareFunc) compare );
}

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

void
object_registry_foreach (GHFunc func, gpointer user_data)
{
  g_hash_table_foreach (object_type_table, func, user_data);
}

DiaObjectType *
object_get_type(char *name)
{
  return (DiaObjectType *) g_hash_table_lookup(object_type_table, name);
}


int
object_return_false(DiaObject *obj)
{
  return FALSE;
}

void *
object_return_null(DiaObject *obj)
{
  return NULL;
}

void
object_return_void(DiaObject *obj)
{
  return;
}

DiaObject *
object_load_using_properties(const DiaObjectType *type,
                             ObjectNode obj_node, int version,
                             const char *filename)
{
  DiaObject *obj;
  Point startpoint = {0.0,0.0};
  Handle *handle1,*handle2;
  
  obj = type->ops->create(&startpoint,NULL, &handle1,&handle2);
  object_load_props(obj,obj_node);
  return obj;
}

void 
object_save_using_properties(DiaObject *obj, ObjectNode obj_node, 
                             int version, const char *filename)
{
  object_save_props(obj,obj_node);
}

DiaObject *object_copy_using_properties(DiaObject *obj)
{
  Point startpoint = {0.0,0.0};
  Handle *handle1,*handle2;
  DiaObject *newobj = obj->type->ops->create(&startpoint,NULL,
                                          &handle1,&handle2);
  object_copy_props(newobj,obj,FALSE);
  return newobj;
}


/* The below are for debugging purposes only. */

/** Check that a DiaObject maintains its invariants and constrains.
 * @param obj An object to check
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
		    "%s: Object %p has illegal type name %p (%s)\n",
		    msg, obj, obj->type->name);
    /* Check the position vs. the bounding box */
    /* Check the handles */
    dia_assert_true(obj->num_handles >= 0, 
		    "%s: Object %p has < 0 (%d) handles\n", 
		    msg, obj,  obj->num_handles);
    if (obj->num_handles != 0) {
      dia_assert_true(obj->handles != NULL,
		      "%s: Object %p has null handles\n", obj);
    }
    for (i = 0; i < obj->num_handles; i++) {
      Handle *h = obj->handles[i];
      dia_assert_true(h != NULL, "%s: Object %p handle %d is null\n", 
		      msg, obj, i);
      if (h != NULL) {
	/* Check handle id */
	dia_assert_true((h->id >= 0 && h->id <= HANDLE_MOVE_ENDPOINT)
			|| (h->id >= HANDLE_CUSTOM1 && h->id <= HANDLE_CUSTOM9),
			"%s: Object %p handle %d (%p) has wrong id %d\n", 
			msg, obj, i, h, h->id);
	/* Check handle type */
	dia_assert_true(h->type >= 0 && h->type <= NUM_HANDLE_TYPES,
			"%s: Object %p handle %d (%p) has wrong type %d\n", 
			msg, obj, i, h, h->type);
	/* Check handle pos is legal pos */
	/* Check connect type is legal */
	dia_assert_true(h->connect_type >= 0
			&& h->connect_type <= HANDLE_CONNECTABLE_NOBREAK,
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
	    dia_assert_true(cp->pos.x == h->pos.x && cp->pos.y == h->pos.y,
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
	dia_assert_true(cp->name == NULL 
			|| g_utf8_validate(cp->name, -1, NULL),
			"%s: Object %p CP %d (%p) has non-UTF8 name %s\n",
			msg, obj, i, cp, cp->name);
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
