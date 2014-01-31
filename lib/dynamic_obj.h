/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * List of "dynamic" (animated) objects, and their refresh rates
 * Copyright (C) 2002 Cyrille Chépélov
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

#ifndef DYNAMIC_OBJ_H
#define DYNAMIC_OBJ_H

#include <glib.h>
#include "object.h"

/* Adds an object to the list of "periodic dynobj" objects. The period
   is an advisory figure in miliseconds; dia can perform dynobjes at a faster
   or at a slower rate, at its own convenience.

   It is strongly advised against dynobj rates below 1000ms. */
void dynobj_list_add_object(DiaObject *obj, guint timeout);

/* Removes an object from the list of "periodic dynobj" objects. It is
   critical that an object gets removed from that list before it is destroyed.

   There is no harmful effect in removing an object which was not in the
   dynobj list beforehand.
*/
void dynobj_list_remove_object(DiaObject *obj);

/* Performs an object dynobj; called by dynobj_list_perform() */
typedef void (*ObjectDynobjFunc) (DiaObject *obj, gpointer data);

/* Performs a dynobj of the whole dynobj list.

Calls (*orf)(obj,data) for each obj.
*/
void dynobj_list_foreach(ObjectDynobjFunc orf, gpointer data);

/* Returns the dynobj rate collectively resulting from the dynamic objects'
   advices. 0 is a special value meaning "no objects, no need
   to have a timer" */ 
guint dynobj_list_get_dynobj_rate(void);


#endif /* DYNAMIC_OBJ_H */
