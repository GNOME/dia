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

#include <config.h>

#include "parent.h"
#include "geometry.h"
#include "object.h"
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
 * Returns the original list minus any items that appear as children
 * (at any depth) of the objects in the original list.  This is very
 * different from the parent_list_affected function, which returns a
 * list of ALL objects affected.

 * The caller must call g_list_free() on the returned list
 * when the list is no longer needed.
 */

GList *parent_list_affected_hierarchy(GList *obj_list)
{
  GHashTable *object_hash = g_hash_table_new(g_direct_hash, g_direct_equal);
  GList *all_list = g_list_copy(obj_list);
  GList *new_list = NULL, *list = all_list;
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
  GList *new_list = NULL, *list = all_list;

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
  Rectangle p_ext, c_ext;
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
  Rectangle common_ext;
  Rectangle p_ext;
  Point new_delta;

  if (!object_flags_set(object, DIA_OBJECT_CAN_PARENT) || !object->children)
    return FALSE;

  parent_point_extents(to, &p_ext);
  while (list)
  {
    if (once) {
      parent_handle_extents(list->data, &common_ext);
      once = FALSE;
    } else {
      parent_handle_extents (list->data, &p_ext);
      rectangle_union(&common_ext, &p_ext);
    }
    list = g_list_next(list);
  }
  new_delta = parent_move_child_delta_out(&p_ext, &common_ext, start_at);
  point_add(to, &new_delta);

  if (new_delta.x || new_delta.y)
    return TRUE;

  return FALSE;
}

/* this function is the opposite of parent_move_child_delta()
   this function makes sure that the parent is OUTSIDE the child's "extent"
   and if it's inside, it returns a delta that moves it back out
   Also the last parameter is used as a direction guide FIX ME FIX ME FIX ME FIX ME FIX ME FIX ME
   (the move is performed in the opposite direction of the passed parameter)
   */
Point parent_move_child_delta_out(Rectangle *p_ext, Rectangle *c_ext, const Point *start)
{
  Point direction;
  Point new_delta = {0, 0};

  direction.x = p_ext->left - start->x;
  direction.y = p_ext->top - start->y;

  /* if the start point is to the left of child and we're moving to the right */
  if (start->x <= c_ext->left && direction.x > 0 && p_ext->left > c_ext->left)
     new_delta.x = c_ext->left - p_ext->left;
   /* if the start point is to the right of the child and we're moving left */
   else if (start->x >= c_ext->right && direction.x < 0 && p_ext->left < c_ext->right)
     new_delta.x =  c_ext->right - p_ext->left;

   /* if the start point is above the child and we're moving down */
   if (start->y <= c_ext->top && direction.y > 0 && p_ext->top > c_ext->top)
     new_delta.y = c_ext->top - p_ext->top;
   /* if the start poit is below the child and we're moving up */
   else if (start->y >= c_ext->bottom && direction.y < 0 && p_ext->bottom < c_ext->bottom)
     new_delta.y =  c_ext->bottom - p_ext->bottom;

   return new_delta;
}

    /* p_ext are the "extents" of the parent and c_ext are the "extents" of the child
      The extent is a rectangle that specifies how far an object goes on the grid (based on its handles)
      If delta is not present, these are the extents *before* any moves
      If delta is present, delta is considered into the extents's position
      */
Point parent_move_child_delta(Rectangle *p_ext, Rectangle *c_ext, Point *delta)
{
    Point new_delta = {0, 0};
    gboolean free_delta = FALSE;
    if (delta == NULL)
    {
      delta = g_new0(Point, 1);
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

    if (free_delta)
      g_free(delta);

    return new_delta;
}

/* the caller must free the returned rectangle */
void
parent_point_extents(Point *point, Rectangle *extents)
{
  extents->left = point->x;
  extents->right = point->x;
  extents->top = point->y;
  extents->bottom = point->y;
}

/**
 * the caller must provide the 'returned' rectangle,
 * which is initialized to the biggest rectangle containing
 * all the objects handles
 */
gboolean
parent_handle_extents(DiaObject *obj, Rectangle *extents)
{
  int idx;
  coord *left_most = NULL, *top_most = NULL, *bottom_most = NULL, *right_most = NULL;

  if (obj->num_handles == 0)
    return FALSE;

  for (idx = 0; idx < obj->num_handles; idx++)
  {
    Handle *handle = obj->handles[idx];

    if (!left_most || *left_most > handle->pos.x)
      left_most = &handle->pos.x;
    if (!right_most || *right_most < handle->pos.x)
      right_most = &handle->pos.x;
    if (!top_most || *top_most > handle->pos.y)
      top_most = &handle->pos.y;
    if (!bottom_most || *bottom_most < handle->pos.y)
      bottom_most = &handle->pos.y;
  }

  extents->left = *left_most;
  extents->right = *right_most;
  extents->top = *top_most;
  extents->bottom = *bottom_most;

  return TRUE;
}
