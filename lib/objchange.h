/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
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
#ifndef CHANGE_H
#define CHANGE_H

typedef struct _ObjectState ObjectState;
typedef struct _ObjectChange ObjectChange;

typedef void (*ObjectChangeApplyFunc)(ObjectChange *change, Object *obj);
typedef void (*ObjectChangeRevertFunc)(ObjectChange *change, Object *obj);
typedef void (*ObjectChangeFreeFunc)(ObjectChange *change);

struct _ObjectChange {
  /* If apply == transaction_point_pointer then this is a transaction
     point */
  ObjectChangeApplyFunc  apply;
  ObjectChangeRevertFunc revert;
  ObjectChangeFreeFunc   free; /* Remove extra data. Then this object is freed */
};

/******** Helper functions of objects: *************/

struct _ObjectState {
  void (*free)(ObjectState *state); /* Frees pointers in the state,
				       not called if NULL */
};

/*
  Gets the internal state from the object.
  This is used to snapshot the object state
  so that it can be stored for undo/redo.

  Need not save state that only depens on
  the object and it's handles positions.
  
  The calling function owns the returned reference.
*/
typedef ObjectState * (*GetStateFunc) (Object* obj);

/*
  Sets the internal state from the object.
  This is used to snapshot the object state
  so that it can be stored for undo/redo.

  The called function owns the reference and is
  responsible for freeing it.
*/
typedef void (*SetStateFunc) (Object* obj, ObjectState *state);


ObjectChange *new_object_state_change(Object *obj,
				      ObjectState *old_state,
				      GetStateFunc get_state,
				      SetStateFunc set_state );

#endif /* CHANGE_H */


