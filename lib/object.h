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
#ifndef OBJECT_H
#define OBJECT_H

#include <gtk/gtk.h>

typedef guint16 ObjectId; /* The id of an objecttype */
typedef struct _Object Object;
typedef struct _ObjectOps ObjectOps;
typedef struct _ObjectType ObjectType;
typedef struct _ObjectTypeOps ObjectTypeOps;

#include "geometry.h"
#include "render.h"
#include "connectionpoint.h"
#include "handle.h"
#include "dia_xml.h"


/************************************
 ** Some general function prototypes
 **
 ** These are the prototypes for the
 ** functions that must be defined for
 ** every object we can insert in a
 ** diagram.
 **
 ************************************/

/*
  Function called to create an object.
  This function is responsible for allocation memory for the object.
  - startpoint : initial position for the object
  - user_data  : Can be used to pass extra params to the creation
                 of the object. Can be used to have two icons of
		 the same object that are instansiated a bit different.
  - handle1    : (return) Handle connected to startpoint
  - handle2    : (return) Handle dragged on creation
  both handle1 and handle2 can be NULL
*/
typedef Object* (*CreateFunc) (Point *startpoint,
			       void *user_data,
			       Handle **handle1,
			       Handle **handle2);

/*
  This function load the object's data from file fd. No header has to be
  skipped. The data should be read using the functions in lib/files.h

  The memory for the object has to be allocated (see CreateFunc)
  
  The version number is the version number of the ObjectType that was saved.
  This must be used to maintain backwards compatible if you change some
  in the save format. All objects must be capable of reading all earlier
  version.
*/
typedef Object* (*LoadFunc) (ObjectNode obj_node, int version);

/*
  This function save the object's data to file fd. No header is required.
  The data should be written using the functions in lib/files.h
*/
typedef void (*SaveFunc) (Object* obj, ObjectNode obj_node);


/*
  Function called before an object is deleted.
  This function must call the parent class's DestroyFunc, and then free
  the memory associated with the object, but not the object itself

  Must also unconnect itself from all other objects.
  (This is by calling object_destroy, or letting the super-class call it)
*/
typedef void (*DestroyFunc) (Object* obj);


/*
  Function responsible for drawing the object.
  Every drawing must be done through the use of the Renderer, so that we
  can render the picture on screen, in an eps file, ...
*/
typedef void (*DrawFunc) (Object* obj, Renderer* ddisp);


/*
  This function must return the distance between the Object and the Point.
  Several functions are provided in geometry.h to facilitate this calculus
*/
typedef real (*DistanceFunc) (Object* obj, Point* point);


/*
  Function called once the object has been selected.
  Basically, this function should update the object (position of the
  handles,...)
  - clicked_point is the point on the screen where the user has clicked
  - interactive_renderer is a renderer that has some extra functions
                         most notably the possibility to get EXACT
			 measures of strings. Used to place cursors
			 and other interactive stuff.
			 (Don't draw to the renderer)
  This function need not redraw the object.
*/
typedef void (*SelectFunc) (Object*   obj,
			    Point*    clicked_point,
			    Renderer* interactive_renderer);

/*
  Returns a copy of Object.
  This must be an depth-copy (pointers must be duplicated and so on)
  as the initial object can be deleted any time
*/
typedef Object* (*CopyFunc) (Object* obj);


/*
  Function called to move the entire object.
  The new position is given by pos.
  It's exact definition depends on the object. It's the point on the
  object that 'snaps' to the grid if that is enabled. (generally it
  is the upper left corner)
*/
typedef void (*MoveFunc) (Object* obj, Point * pos);


/*
  Function called to move one of the handles associated with the
  object. Its new position is given by pos.
  - reason  gives the reason the handle was moved.
            HANDLE_MOVE_USER means the user is dragging the point.
	    HANDLE_MOVE_USER_FINAL means the user let go of the point.
	    HANDLE_MOVE_CONNECTED means it was moved because something
	    it was connected to moved.
	    
*/
typedef void (*MoveHandleFunc) (Object*          obj,
				Handle*          handle,
				Point*           pos,
				HandleMoveReason reason);

/*
  Function called when the user has double clicked on an Object.
  This function should return a dialog to edit the properties
  of the object.
  When this function is called and the dialog already is created,
  make sure to update the values in the widgets so that it
  accurately describes the current state of the object.
  Remember to destroy this dialog when the object is destroyed!
*/
typedef GtkWidget *(*GetPropertiesFunc) (Object* obj);

/*
  Thiss function is called when the user clicks on
  the "Apply" button.
*/
typedef void (*ApplyPropertiesFunc) (Object* obj);

/*
  Return TRUE if object does not contain anything
  (i.e. is empty on the screen)
  Then it will be removed, because the user cannot select it to
  remove it himself.

  This is mainly used by TextObj.
*/
typedef int  (*IsEmptyFunc) (Object* obj);



/*************************************
 **  The functions provided in object.c
 *************************************/

extern void object_init(Object *obj, int num_handles, int num_connections);
extern void object_destroy(Object *obj);
extern void object_copy(Object *from, Object *to);

extern void object_save(Object *obj, ObjectNode obj_node);
extern void object_load(Object *obj, ObjectNode obj_node);

extern void destroy_object_list(GList *list);
extern void object_add_handle(Object *obj, Handle *handle);
extern void object_remove_handle(Object *obj, Handle *handle);
extern void object_add_connectionpoint(Object *obj, ConnectionPoint *conpoint);
extern void object_remove_connectionpoint(Object *obj,
					  ConnectionPoint *conpoint);
extern void object_connect(Object *obj, Handle *handle,
			   ConnectionPoint *conpoint);
extern void object_unconnect(Object *connected_obj, Handle *handle);
extern void object_remove_connections_to(ConnectionPoint *conpoint);
extern void object_unconnect_all(Object *connected_obj);
extern void object_registry_init(void);
extern void object_register_type(ObjectType *type);
extern ObjectType *object_get_type(char *name);
extern int object_return_false(Object *obj); /* Just returns FALSE */
extern void *object_return_null(Object *obj); /* Just returns NULL */
extern void object_return_void(Object *obj); /* Just an empty function */

/*****************************************
 **  The structures used to define an object
 *****************************************/
  
/*
  This structure gives access to the functions used to manipulate an object
  See information above on the use of the functions
*/

struct _ObjectOps {
  DestroyFunc         destroy;
  DrawFunc            draw;
  DistanceFunc        distance_from;
  SelectFunc          select;
  CopyFunc            copy;
  MoveFunc            move;
  MoveHandleFunc      move_handle;
  GetPropertiesFunc   get_properties;
  ApplyPropertiesFunc apply_properties;
  IsEmptyFunc         is_empty;
  
  /*
    Unused places (for extension).
    These should be NULL for now. In the future they might be used.
    Then an older object will be binary compatible, because all new code
    checks if new ops are supported (!= NULL)
  */
  void      (*(unused[10]))(Object *obj,...); 
};

/*
  The base class in the Object hierarcy.
  All information in this structure read-only
  from the application point of view except
  when connection objects. (Then handles and
  connections are changed).

  position is not necessarly the corner of the object, but rather
  some 'good' spot on it which will be natural to snap to.
*/

struct _Object {
  ObjectType       *type;
  Point             position;
  Rectangle         bounding_box;
  
  int               num_handles;
  Handle          **handles;
  
  int               num_connections;
  ConnectionPoint **connections;
  
  ObjectOps *ops;
};

struct _ObjectTypeOps {
   CreateFunc create;
   LoadFunc   load;
   SaveFunc   save;
};

/*
   Structure so that the ObjectFactory can create objects
   of unknown type. (Read in from a shared lib.)
 */
struct _ObjectType {
  char *name;
  int version;

  char **pixmap; /* Also put a pixmap in the sheet_object.
		    This one is used if not in sheet but in toolbar.
		    Stored in xpm format */
  
  ObjectTypeOps *ops;
};

#endif /* OBJECT_H */





