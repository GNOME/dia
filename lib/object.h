/* Dia -- an diagram creation/manipulation program -*- c -*-
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

/** @file object.h -- definitions for Dia objects, in particular the 'virtual'
 * functions and the object and type structures.
 */
#ifndef OBJECT_H
#define OBJECT_H

#include "diatypes.h"
#include <gtk/gtk.h>

#include "geometry.h"
#include "connectionpoint.h"
#include "handle.h"
#include "objchange.h"
#include "diamenu.h"
#include "dia_xml.h"
#include "properties.h"
#include "diagramdata.h"
#include "parent.h"

/** This enumeration gives a bitset of modifier keys currently held down.
 */
typedef enum {
  MODIFIER_NONE,
  MODIFIER_LEFT_SHIFT,
  MODIFIER_RIGHT_SHIFT,
  MODIFIER_SHIFT,
  MODIFIER_LEFT_ALT = 4,
  MODIFIER_RIGHT_ALT = 8,
  MODIFIER_ALT = 12,
  MODIFIER_LEFT_CONTROL = 16,
  MODIFIER_RIGHT_CONTROL = 32,
  MODIFIER_CONTROL = 48
} ModifierKeys;

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
typedef DiaObject* (*CreateFunc) (Point *startpoint,
			       void *user_data,
			       Handle **handle1,
			       Handle **handle2);

/*
  This function load the object's data from file fd. No header has to be
  skipped. The data should be read using the functions in lib/files.h

  The memory for the object has to be allocated (see CreateFunc)
  
  The version number is the version number of the DiaObjectType that was saved.
  This must be used to maintain backwards compatible if you change some
  in the save format. All objects must be capable of reading all earlier
  version.
*/
typedef DiaObject* (*LoadFunc) (ObjectNode obj_node, int version,
			     const char *filename);

/*
  This function save the object's data to file fd. No header is required.
  The data should be written using the functions in lib/files.h
*/
typedef void (*SaveFunc) (DiaObject* obj, ObjectNode obj_node,
			  const char *filename);


/*
  Function called before an object is deleted.
  This function must call the parent class's DestroyFunc, and then free
  the memory associated with the object, but not the object itself

  Must also unconnect itself from all other objects.
  (This is by calling object_destroy, or letting the super-class call it)
*/
typedef void (*DestroyFunc) (DiaObject* obj);


/*
  Function responsible for drawing the object.
  Every drawing must be done through the use of the Renderer, so that we
  can render the picture on screen, in an eps file, ...
*/
typedef void (*DrawFunc) (DiaObject* obj, DiaRenderer* ddisp);


/*
  This function must return the distance between the DiaObject and the Point.
  Several functions are provided in geometry.h to facilitate this calculus
*/
typedef real (*DistanceFunc) (DiaObject* obj, Point* point);


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
typedef void (*SelectFunc) (DiaObject*   obj,
			    Point*    clicked_point,
			    DiaRenderer* interactive_renderer);

/*
  Returns a copy of DiaObject.
  This must be an depth-copy (pointers must be duplicated and so on)
  as the initial object can be deleted any time
*/
typedef DiaObject* (*CopyFunc) (DiaObject* obj);

/*
  Function called to move the entire object.
  The new position is given by pos.
  It's exact definition depends on the object. It's the point on the
  object that 'snaps' to the grid if that is enabled. (generally it
  is the upper left corner)
  Returns an ObjectChange* with additional undo information, or
  (in most cases) NULL.  Undo for moving the object itself is handled
  elsewhere.
*/
typedef ObjectChange* (*MoveFunc) (DiaObject* obj, Point * pos);


/**
 *  Function called to move one of the handles associated with the
 *  object. 
 *  @param obj The object whose handle is being moved.
 *  @param handle The handle being moved.
 *  @param pos The position it has been moved to (corrected for
 *   vertical/horizontal only movement).
 *  @param cp If non-NULL, the connectionpoint found at this position.
 *   If @a cp is NULL, there may or may not be a connectionpoint.
 *  @param The reason the handle was moved.
 *     - HANDLE_MOVE_USER means the user is dragging the point.
 *     - HANDLE_MOVE_USER_FINAL means the user let go of the point.
 *     - HANDLE_MOVE_CONNECTED means it was moved because something
 *	    it was connected to moved.
 *  @param modifiers gives a bitset of modifier keys currently held down
 *     - MODIFIER_SHIFT is either shift key
 *     - MODIFIER_ALT is either alt key
 *     - MODIFIER_CONTROL is either control key
 *	    Each has MODIFIER_LEFT_* and MODIFIER_RIGHT_* variants
 *  @return An @a ObjectChange* with additional undo information, or
 *  (in most cases) NULL.  Undo for moving the handle itself is handled
 *  elsewhere.
 */
typedef ObjectChange* (*MoveHandleFunc) (DiaObject*          obj,
					 Handle*          handle,
					 Point*           pos,
					 ConnectionPoint* cp,
					 HandleMoveReason reason,
					 ModifierKeys     modifiers);

/*
  Function called when the user has double clicked on an DiaObject.
  This function should return a dialog to edit the properties
  of the object.
  When this function is called and the dialog already is created,
  make sure to update the values in the widgets so that it
  accurately describes the current state of the object.
  Remember to destroy this dialog when the object is destroyed!

  Note that if you want to use the same dialog multiple times,
  you should ref it first.  Just run the following on the widget
  when you create it:
    gtk_object_ref(GTK_OBJECT(widget));
    gtk_object_sink(GTK_OBJECT(widget)); / * optional, but recommended * /
  If you don't do this, the widget will be destroyed when the
  properties dialog is closed.

  If is_default is true, this dialog is for object defaults, and
  the toolbox options should not be shown.
*/
typedef GtkWidget *(*GetPropertiesFunc) (DiaObject* obj, gboolean is_default);

/*
  Thiss function is called when the user clicks on
  the "Apply" button.  The widget parameter is the one created by
  the get_properties function.

  Must returns a Change that can be used for undo/redo.
  The returned change is already applied.
*/
typedef ObjectChange *(*ApplyPropertiesFunc) (DiaObject* obj, GtkWidget *widget);

/*
  This function is called to return a list of property
  descriptions the object supports.  The list should be
  NULL terminated.
*/
typedef const PropDescription *(* DescribePropsFunc) (DiaObject *obj);

/*
  This function is called to return the current values
  (and type information) for a number of properties of
  the object.
*/
typedef void (* GetPropsFunc) (DiaObject *obj, GPtrArray *props);

/*
  This function is called to set the value of a number
  of properties of the object.
*/
typedef void (* SetPropsFunc) (DiaObject *obj, GPtrArray *props);


/*
  Function called when the user has double clicked on an Tool.
  This function should return a dialog to edit the defaults
  of the tool.
  When this function is called and the dialog already is created,
  make sure to update the values in the widgets so that it
  accurately describes the current state of the tool.
*/
typedef GtkWidget *(*GetDefaultsFunc) ();

/*
*/
typedef void *(*ApplyDefaultsFunc) ();

/*
  Return an object-specific menu with toggles etc. properly set.
*/
typedef DiaMenu *(*ObjectMenuFunc) (DiaObject* obj, Point *position);

/*************************************
 **  The functions provided in object.c
 *************************************/

void object_init(DiaObject *obj, int num_handles, int num_connections);
void object_destroy(DiaObject *obj); /* Unconnects handles, so don't
					    free handles before calling. */
void object_copy(DiaObject *from, DiaObject *to);

void object_save(DiaObject *obj, ObjectNode obj_node);
void object_load(DiaObject *obj, ObjectNode obj_node);

GList *object_copy_list(GList *list);
ObjectChange* object_list_move_delta_r(GList *objects, Point *delta, gboolean affected);
ObjectChange* object_list_move_delta(GList *objects, Point *delta);
/** Rotate an object around a point.  If center is NULL, the position of
 * the object is used. */
ObjectChange* object_list_rotate(GList *objects, Point *center, real angle);
/** Scale an object around a point.  If center is NULL, the position of
 * the object is used. */
ObjectChange* object_list_scale(GList *objects, Point *center, real factor);
void destroy_object_list(GList *list);
void object_add_handle(DiaObject *obj, Handle *handle);
void object_add_handle_at(DiaObject *obj, Handle *handle, int pos);
void object_remove_handle(DiaObject *obj, Handle *handle);
void object_add_connectionpoint(DiaObject *obj, ConnectionPoint *conpoint);
void object_remove_connectionpoint(DiaObject *obj,
				   ConnectionPoint *conpoint);
void object_add_connectionpoint_at(DiaObject *obj, 
				   ConnectionPoint *conpoint,
				   int pos);
void object_connect(DiaObject *obj, Handle *handle,
		    ConnectionPoint *conpoint);
void object_unconnect(DiaObject *connected_obj, Handle *handle);
void object_remove_connections_to(ConnectionPoint *conpoint);
void object_unconnect_all(DiaObject *connected_obj);
void object_registry_init(void);
void object_register_type(DiaObjectType *type);
void object_registry_foreach(GHFunc func, gpointer  user_data);
DiaObjectType *object_get_type(char *name);

int object_return_false(DiaObject *obj); /* Just returns FALSE */
void *object_return_null(DiaObject *obj); /* Just returns NULL */
void object_return_void(DiaObject *obj); /* Just an empty function */

/* These functions can be used as a default implementation for an object which
   can be completely described, loaded and saved through standard properties.
*/
DiaObject *object_load_using_properties(const DiaObjectType *type,
                                     ObjectNode obj_node, int version,
                                     const char *filename);
void object_save_using_properties(DiaObject *obj, ObjectNode obj_node, 
                                  int version, const char *filename);
DiaObject *object_copy_using_properties(DiaObject *obj);

/*****************************************
 **  The structures used to define an object
 *****************************************/

/** This structure defines an affine transformation that has been applied
 * to this object.  Affine transformations done on a group are performed
 * on all objects in the group.
 */

typedef struct _Affine {
  real rotation;
  real scale;
  real translation;
} Affine;


/*
  This structure gives access to the functions used to manipulate an object
  See information above on the use of the functions
*/

struct _ObjectOps {
  DestroyFunc         destroy;
  DrawFunc            draw;
  DistanceFunc        distance_from;
  SelectFunc          selectf;
  CopyFunc            copy;
  MoveFunc            move;
  MoveHandleFunc      move_handle;
  GetPropertiesFunc   get_properties;
  ApplyPropertiesFunc apply_properties;
  ObjectMenuFunc      get_object_menu;

  DescribePropsFunc   describe_props;
  GetPropsFunc        get_props;
  SetPropsFunc        set_props;

  /*
    Unused places (for extension).
    These should be NULL for now. In the future they might be used.
    Then an older object will be binary compatible, because all new code
    checks if new ops are supported (!= NULL)
  */
  void      (*(unused[6]))(DiaObject *obj,...); 
};

/*
  The base class in the DiaObject hierarcy.
  All information in this structure read-only
  from the application point of view except
  when connection objects. (Then handles and
  connections are changed).

  position is not necessarly the corner of the object, but rather
  some 'good' spot on it which will be natural to snap to.
*/

struct _DiaObject {
  DiaObjectType    *type;
  Point             position;
  Rectangle         bounding_box;
  Affine            affine;
  
  int               num_handles;
  Handle          **handles;
  
  int               num_connections;
  ConnectionPoint **connections;
  
  ObjectOps *ops;

  Layer *parent_layer; /* Back-pointer to the owning layer.
			  This may only be set by functions internal to
			  the layer, and accessed via 
			  dia_object_get_parent_layer() */
  DiaObject *parent;
  GList *children;
  gboolean can_parent;

  Color *highlight_color; /* The color that this object is currently
			     highlighted with, or NULL if it is not 
			     highlighted. */
};

struct _ObjectTypeOps {
  CreateFunc        create;
  LoadFunc          load;
  SaveFunc          save;
  GetDefaultsFunc   get_defaults;
  ApplyDefaultsFunc apply_defaults;
  /*
    Unused places (for extension).
    These should be NULL for now. In the future they might be used.
    Then an older object will be binary compatible, because all new code
    checks if new ops are supported (!= NULL)
  */
  void      (*(unused[10]))(DiaObject *obj,...); 
};

/*
   Structure so that the ObjectFactory can create objects
   of unknown type. (Read in from a shared lib.)
 */
struct _DiaObjectType {

  char *name;
  int version;

  char **pixmap; /* Also put a pixmap in the sheet_object.
		    This one is used if not in sheet but in toolbar.
		    Stored in xpm format */
  
  ObjectTypeOps *ops;

  char *pixmap_file; /* fallback if pixmap is NULL */
  void *default_user_data; /* use this if no user data is specified in
			      the .sheet file */
};

/* base property stuff ... */
#define OBJECT_COMMON_PROPERTIES \
  { "obj_pos", PROP_TYPE_POINT, 0, \
    "DiaObject position", "Where the object is located"}, \
  { "obj_bb", PROP_TYPE_RECT, 0, \
    "DiaObject bounding box", "The bounding box of the object"}

#define OBJECT_COMMON_PROPERTIES_OFFSETS \
  { "obj_pos", PROP_TYPE_POINT, offsetof(DiaObject, position) }, \
  { "obj_bb", PROP_TYPE_RECT, offsetof(DiaObject, bounding_box) }


gboolean       dia_object_defaults_load (const gchar *filename,
                                         gboolean create_lazy);
void           dia_object_default_make (const DiaObject *obj_from);
DiaObject  *dia_object_default_get  (const DiaObjectType *type);
DiaObject  *dia_object_default_create (const DiaObjectType *type,
                                    Point *startpoint,
                                    void *user_data,
                                    Handle **handle1,
                                    Handle **handle2);
gboolean       dia_object_defaults_save (const gchar *filename);
Layer         *dia_object_get_parent_layer(DiaObject *obj);
gboolean       dia_object_is_selected (const DiaObject *obj);

G_END_DECLS

#endif /* OBJECT_H */

