/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 2003 Vadim Berezniker
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

#include "geometry.h"
#include "object.h"
#include "parent.h"
#include <glib.h>


/*
  Takes a list of objects and appends all possible children (at any depth) to the list

  Returns TRUE if the list didn't change
*/
gboolean parent_list_expand(GList *obj_list)
{
  gboolean nothing_affected = FALSE;
  GList *list = obj_list;
  while (list)
  {
    DiaObject *obj = (DiaObject *) list->data;

    if (object_flags_set(obj, DIA_OBJECT_CAN_PARENT) && obj->children)
    {
      obj_list = g_list_concat(obj_list, g_list_copy(obj->children));
      nothing_affected = FALSE;
    }

    list = g_list_next(list);
  }

  return nothing_affected;
}


/**
 * parent_list_affected_hierarchy:
 * obj_list: the #DiaObject list
 *
 * Returns the original list minus any items that appear as children
 * (at any depth) of the objects in the original list. This is very
 * different from the parent_list_affected function, which returns a
 * list of ALL objects affected.
 *
 * The caller must call g_list_free() on the returned list
 * when the list is no longer needed.
 */
GList *
parent_list_affected_hierarchy (GList *obj_list)
{
  GHashTable *object_hash = g_hash_table_new(g_direct_hash, g_direct_equal);
  GList *all_list = g_list_copy(obj_list);
  GList *new_list = NULL, *list;
  int orig_length = g_list_length(obj_list);

  if (parent_list_expand(all_list)) /* fast way out */
    return g_list_copy(obj_list);

  /* enter all of the objects (except initial) in a hash */
  list = g_list_nth(all_list, orig_length);
  while (list)
  {
    g_hash_table_insert(object_hash, list->data, (void*) 1);
    list  = g_list_next(list);
  }

  list = obj_list;
  while (list)
  {
    if (!g_hash_table_lookup(object_hash, list->data))
      new_list = g_list_append(new_list, list->data);

    list = g_list_next(list);
  }

  g_list_free(all_list);
  g_hash_table_destroy(object_hash);

  return new_list;
}

/*
   Finds all children of affected objects that are not in the list that should be
   affected by an operation

   The caller must call g_list_free() on the returned list
   when the list is no longer needed

   Any found affected children will be appended to the end of the list,
   thus the front of the list is exactly the same as the passed in list.
 */

GList *parent_list_affected(GList *obj_list)
{
  GHashTable *object_hash = g_hash_table_new(g_direct_hash, g_direct_equal);
  GList *all_list = g_list_copy(obj_list);
  GList *new_list = NULL, *list;

  if (parent_list_expand(all_list)) /* fast way out */
    return g_list_copy(obj_list);

  /* eliminate duplicates */
  list = all_list;
  while (list)
  {
    DiaObject *obj = (DiaObject *) list->data;
    if (!g_hash_table_lookup(object_hash, obj))
    {
      new_list = g_list_append(new_list, obj);
      g_hash_table_insert(object_hash, obj, (void *) 1);
    }

    list  = g_list_next(list);
  }

  g_list_free(all_list);

  return new_list;
}

/* if the object is resizing up and hits its parents' border, prevent the resizing from
  going any further

  returns TRUE if resizing was obstructed*/
gboolean
parent_handle_move_out_check(DiaObject *object, Point *to)
{
  DiaRectangle p_ext, c_ext;
  Point new_delta;

  if (!object->parent)
    return FALSE;

  parent_handle_extents(object->parent, &p_ext);
  parent_point_extents(to, &c_ext);

  new_delta = parent_move_child_delta(&p_ext, &c_ext, NULL);
  point_add(to, &new_delta);

  if (new_delta.x || new_delta.y)
    return TRUE;

  return FALSE;
}

/* if the object resizing down intersects a child, the resizing is prevented from going
   any further

   returns TRUE if resizing was obstructed
   */
gboolean parent_handle_move_in_check(DiaObject *object, Point *to, Point *start_at)
{
  GList *list = object->children;
  gboolean once = TRUE;
  DiaRectangle common_ext;
  gboolean restricted = FALSE;

  if (!object_flags_set(object, DIA_OBJECT_CAN_PARENT) || !list)
    return FALSE;

  while (list)
  {
    if (once) {
      parent_handle_extents(list->data, &common_ext);
      once = FALSE;
    } else {
      DiaRectangle c_ext;
      parent_handle_extents (list->data, &c_ext);
      rectangle_union(&common_ext, &c_ext);
    }
    list = g_list_next(list);
  }
  /* The start point gives the decision were to clip to:
   * if it is below the child rect we need to clip to it's bottom.
   * The check has nothing to do with 'to' being in the rectangle,
   * but instead having the right barrier
   */
  if (start_at->y >= common_ext.bottom) {
	if (to->y < common_ext.bottom)
	  to->y = common_ext.bottom, restricted = TRUE;
  } else if (start_at->y <= common_ext.top) {
	if (to->y > common_ext.top)
	  to->y = common_ext.top, restricted = TRUE;
  }

  if (start_at->x >= common_ext.right) {
	if (to->x < common_ext.right)
	  to->x = common_ext.right, restricted = TRUE;
  } else if (start_at->x <= common_ext.left) {
	if (to->x > common_ext.left)
	  to->x = common_ext.left, restricted = TRUE;
  }

  return restricted;
}

    /* p_ext are the "extents" of the parent and c_ext are the "extents" of the child
      The extent is a rectangle that specifies how far an object goes on the grid (based on its handles)
      If delta is not present, these are the extents *before* any moves
      If delta is present, delta is considered into the extents's position
      */
Point
parent_move_child_delta (DiaRectangle *p_ext,
                         DiaRectangle *c_ext,
                         Point        *delta)
{
  Point new_delta = { 0, 0 };
  gboolean free_delta = FALSE;

  if (delta == NULL) {
    delta = g_new0 (Point, 1);
    free_delta = TRUE;
  }

    /* we check if the child extent would be inside the parent extent after the move
      if not, we calculate how far we have to move the extent back to place it back
      inside the parent extent */
    if (c_ext->left + delta->x < p_ext->left )
      new_delta.x = p_ext->left - (c_ext->left + delta->x);
    else if (c_ext->left + delta->x + (c_ext->right - c_ext->left) > p_ext->right)
      new_delta.x = p_ext->right - (c_ext->left + delta->x + (c_ext->right - c_ext->left));

    if (c_ext->top + delta->y < p_ext->top)
      new_delta.y = p_ext->top - (c_ext->top + delta->y);
    else if (c_ext->top + delta->y + (c_ext->bottom - c_ext->top) > p_ext->bottom)
      new_delta.y = p_ext->bottom  - (c_ext->top + delta->y + (c_ext->bottom - c_ext->top));

  if (free_delta) {
    g_clear_pointer (&delta, g_free);
  }

  return new_delta;
}

/* the caller must free the returned rectangle */
void
parent_point_extents(Point *point, DiaRectangle *extents)
{
  extents->left = point->x;
  extents->right = point->x;
  extents->top = point->y;
  extents->bottom = point->y;
}


/**
 * parent_handle_extents:
 * @obj: the #DiaObject
 * @extents: the extend of the object
 *
 * the caller must provide the 'returned' rectangle,
 * which is initialized to the biggest rectangle containing
 * all the objects handles
 *
 * Since: dawn-of-time
 */
void
parent_handle_extents (DiaObject *obj, DiaRectangle *extents)
{
  int idx;

  g_assert (obj->num_handles > 0);
  /* initialize the rectangle with the first handle */
  extents->left = extents->right = obj->handles[0]->pos.x;
  extents->top = extents->bottom = obj->handles[0]->pos.y;
  /* grow it with the other ones */
  for (idx = 1; idx < obj->num_handles; idx++) {
    Handle *handle = obj->handles[idx];
    rectangle_add_point (extents, &handle->pos);
  }
}


/**
 * parent_apply_to_children:
 * @obj: A parent object.
 * @function: A function that takes a single DiaObject as an argument.
 *
 * Apply a function to all children of the given object (recursively,
 * depth-first).
 */
void
parent_apply_to_children (DiaObject *obj, DiaObjectFunc function)
{
  GList *children;
  for (children = obj->children; children != NULL; children = g_list_next (children)) {
    DiaObject *child = children->data;
    (*function) (child);
    parent_apply_to_children (child, function);
  }
}
