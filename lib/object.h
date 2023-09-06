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

/*
 * Definitions for Dia objects, in particular the 'virtual'
 * functions and the object and type structures.
 */

#pragma once

#include "diatypes.h"
#include <gtk/gtk.h>

#include "geometry.h"
#include "connectionpoint.h"
#include "handle.h"
#include "diamenu.h"
#include "dia_xml.h"
#include "text.h"
#include "diacontext.h"
#include "diarenderer.h"
#include "dia-object-change.h"
#include "properties.h"
#include "widgets.h"

G_BEGIN_DECLS

#define DIA_TYPE_EXCHANGE_OBJECT_CHANGE dia_exchange_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaExchangeObjectChange,
                      dia_exchange_object_change,
                      DIA, EXCHANGE_OBJECT_CHANGE,
                      DiaObjectChange)


/**
 * DiaObjectFlags:
 *
 * Flags for DiaObject
 */
typedef enum {
  /* Set this if the DiaObject can 'contain' other objects that move with
   * it, a.k.a. be a parent.  A parent moves its children along and
   * constricts its children to live inside its borders.
   */
  DIA_OBJECT_CAN_PARENT = 1,
  /* Set this if the DiaObject uses its user_data to store a 'variant', a
   * unique style represented by an integer (like which logic gate or border
   * style to use).
   */
  DIA_OBJECT_HAS_VARIANTS = 2,
} DiaObjectFlags;


/**
 * ModifierKeys:
 *
 * This enumeration gives a bitset of modifier keys currently held down.
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

/*** Object type (class) operations ***/

/*!
 * \brief Construct _DiaObject from scratch
 *
 *  This function is responsible for allocation memory for the object.
 *  This is one of the object class functions.
 * @param startpoint : initial position for the object
 * @param user_data  : Can be used to pass extra params to the creation
 *                of the object. Can be used to have two icons of
 *                the same object that are instansiated a bit different.
 * @param handle1    : (return) Handle connected to startpoint
 * @param handle2    : (return) Handle dragged on creation
 *  both handle1 and handle2 can be NULL
 * @return A newly created object.
 *
 * \public \memberof _DiaObjectType
 */
typedef DiaObject* (*CreateFunc) (Point *startpoint,
			       void *user_data,
			       Handle **handle1,
			       Handle **handle2);

/*!
 * \brief Construct _DiaObject from storage
 *
 *  This function load the object's data from file. No header has to be
 *  skipped. The data should be read using the functions in lib/files.h
 *  The memory for the object has to be allocated (see CreateFunc)
 *  The version number is the version number of the DiaObjectType that was saved.
 *  This must be used to maintain backwards compatible if you change some
 *  in the save format. All objects must be capable of reading all earlier
 *  version.
 *
 * @param obj_node A node in an XML object to load from.
 * @param version The version of the object found in the XML file.
 * @param filename The name of the file we're loading from, for use in
 *                 error messages.
 *
 * \public \memberof _DiaObjectType
 */
typedef DiaObject* (*LoadFunc) (ObjectNode obj_node, int version,
				DiaContext *ctx);

/*!
 * \brief Save the _DiaObject to file
 *
 *  This function save the object's data to file. No header is required.
 *  The data should be written using the functions in lib/files.h
 *
 * @param obj An object to save.
 * @param obj_node An XML node to save it in.
 * @param filename The name of the file we're saving to, for use in error
 *                 messages.
 *
 * \public \memberof _DiaObjectType
 */
typedef void (*SaveFunc) (DiaObject* obj, ObjectNode obj_node,
			  DiaContext *ctx);

/*!
 * \brief Create a dialog to edit _DiaObject defaults (deprecated)
 *
 *  Function called when the user has double clicked on an Tool.
 *  When this function is called and the dialog already is created,
 *  make sure to update the values in the widgets so that it
 *  accurately describes the current state of the tool.
 *
 * @return a dialog that the user can use to edit the defaults for new
 * objects of this type.
 *
 * \public \memberof _DiaObjectType
 */
typedef GtkWidget *(*GetDefaultsFunc) (void);

/*!
 * \brief Apply the defaults from the default object dialog (deprecated)
 *
 * Function called when the user clicks Apply on an edit defaults dialog.
 * This is currently not used by any object.
 *
 * \public \memberof _DiaObjectType
 */
typedef void *(*ApplyDefaultsFunc) (void);

/*** Object operations ***/

/*!
 * \brief DiaObject destructor
 *
 * This function must call the parent class's DestroyFunc, and then free
 * the memory associated with the object, but not the object itself
 * Must also unconnect itself from all other objects.
 * (This is by calling object_destroy, or letting the super-class call it)
 *
 * @param obj Explicit this pointer
 *
 * \public \memberof _DiaObject
 */
typedef void (*DestroyFunc) (DiaObject* obj);

// TODO: Actually check the cast with GType
#define DIA_OBJECT(object) ((DiaObject *) object)

typedef void (*DrawFunc) (DiaObject* obj, DiaRenderer* ddisp);
typedef double (*DistanceFunc) (DiaObject* obj, Point* point);
typedef void (*SelectFunc) (DiaObject*   obj,
			    Point*    clicked_point,
			    DiaRenderer* interactive_renderer);
typedef DiaObject* (*CopyFunc) (DiaObject* obj);
typedef DiaObjectChange       *(*MoveFunc)                  (DiaObject        *obj,
                                                             Point            *pos);
typedef DiaObjectChange       *(*MoveHandleFunc)            (DiaObject        *obj,
                                                             Handle           *handle,
                                                             Point            *pos,
                                                             ConnectionPoint  *cp,
                                                             HandleMoveReason  reason,
                                                             ModifierKeys      modifiers);
typedef GtkWidget             *(*GetPropertiesFunc)         (DiaObject        *obj,
                                                             gboolean          is_default);
typedef DiaObjectChange       *(*ApplyPropertiesDialogFunc) (DiaObject        *obj,
                                                             GtkWidget        *widget);
typedef DiaObjectChange       *(*ApplyPropertiesListFunc)   (DiaObject        *obj,
                                                             GPtrArray        *props);
typedef const PropDescription *(*DescribePropsFunc)         (DiaObject        *obj);
typedef void (* GetPropsFunc) (DiaObject *obj, GPtrArray *props);
typedef void (* SetPropsFunc) (DiaObject *obj, GPtrArray *props);
typedef DiaMenu *(*ObjectMenuFunc) (DiaObject* obj, Point *position);
typedef gboolean (*TextEditFunc) (DiaObject *obj, Text *text, TextEditState state, gchar *textchange);
typedef gboolean (*TransformFunc) (DiaObject *obj, const DiaMatrix *m);




/*************************************
 **  The functions provided in object.c
 *************************************/

void object_init(DiaObject *obj, int num_handles, int num_connections);
void object_destroy(DiaObject *obj); /* Unconnects handles, so don't
					    free handles before calling. */
void object_copy(DiaObject *from, DiaObject *to);

void object_save(DiaObject *obj, ObjectNode obj_node, DiaContext *ctx);
void object_load(DiaObject *obj, ObjectNode obj_node, DiaContext *ctx);

GList *object_copy_list (GList *list_orig );
DiaObjectChange *object_list_move_delta_r (GList     *objects,
                                           Point     *delta,
                                           gboolean   affected);
DiaObjectChange *object_list_move_delta   (GList     *objects,
                                           Point     *delta);
DiaObjectChange *object_substitute        (DiaObject *obj,
                                           DiaObject *subst);

void destroy_object_list (GList *list_to_be_destroyed);
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
gchar *object_get_displayname (DiaObject* obj);
gboolean object_flags_set(DiaObject* obj, gint flags);

/* These functions can be used as a default implementation for an object which
   can be completely described, loaded and saved through standard properties.
*/
DiaObject *object_load_using_properties(const DiaObjectType *type,
					ObjectNode obj_node, int version,
					DiaContext *ctx);
void object_save_using_properties(DiaObject *obj, ObjectNode obj_node,
                                  DiaContext *ctx);
DiaObject *object_copy_using_properties(DiaObject *obj);

/*****************************************
 **  The structures used to define an object
 *****************************************/

/*
  This structure gives access to the functions used to manipulate an object
  See information above on the use of the functions
*/
struct _ObjectOps {
  DestroyFunc         destroy;

  void                   (*draw)                         (DiaObject        *obj,
                                                          DiaRenderer      *ddisp);
  double                 (*distance_from)                (DiaObject        *obj,
                                                          Point            *point);
  void                   (*selectf)                      (DiaObject        *obj,
                                                          Point            *clicked_point,
                                                          DiaRenderer      *interactive_renderer);
  DiaObject             *(*copy)                         (DiaObject        *obj);
  DiaObjectChange       *(*move)                         (DiaObject        *obj,
                                                          Point            *pos);
  DiaObjectChange       *(*move_handle)                  (DiaObject        *obj,
                                                          Handle           *handle,
                                                          Point            *pos,
                                                          ConnectionPoint  *cp,
                                                          HandleMoveReason  reason,
                                                          ModifierKeys      modifiers);
  GtkWidget             *(*get_properties)               (DiaObject        *obj,
                                                          gboolean          is_default);
  DiaObjectChange       *(*apply_properties_from_dialog) (DiaObject        *obj,
                                                          GtkWidget        *widget);
  DiaMenu               *(*get_object_menu)              (DiaObject        *obj,
                                                          Point            *position);
  const PropDescription *(*describe_props)               (DiaObject        *obj);
  void                   (*get_props)                    (DiaObject        *obj,
                                                          GPtrArray        *props);
  void                   (*set_props)                    (DiaObject        *obj,
                                                          GPtrArray        *props);
  gboolean               (*edit_text)                    (DiaObject        *obj,
                                                          Text             *text,
                                                          TextEditState     state,
                                                          gchar            *textchange);
  DiaObjectChange       *(*apply_properties_list)        (DiaObject        *obj,
                                                          GPtrArray        *props);
  gboolean               (*transform)                    (DiaObject        *obj,
                                                          const DiaMatrix  *m);

  /*!
    Unused places (for extension).
    These should be NULL for now. In the future they might be used.
    Then an older object will be binary compatible, because all new code
    checks if new ops are supported (!= NULL)
  */
  void      (*unused[3])(DiaObject *obj,...);
};

// #ifdef _DIA_OBJECT_BUILD
# define _DIA_OBJECT_FIELD(type,name)      type name
// #else
// # define _DIA_OBJECT_FIELD(type,name)      type __graphene_private_##name
// #endif

/*!
  \brief Base class for all of Dia's objects, i.e. diagram building blocks

  The base class in the DiaObject hierarchy.
  All information in this structure read-only
  from the application point of view except
  when connection objects. (Then handles and
  connections are changed).

  position is not necessarily the corner of the object, but rather
  some 'good' spot on it which will be natural to snap to.
*/
struct _DiaObject {
  DiaObjectType    *type; /*!< pointer to the registered type */
  Point             position; /*!<  often but not necessarily the upper left corner of the object */
  /*!
   * \brief DiaRectangle containing the whole object
   *
   * The area that contains all parts of the 'real' object, i.e. the parts
   *  that would be printed, exported to pixmaps etc.  This is also used to
   *  determine the size of autofit scaling, so it should be as large as
   *  the objects without interactive bits and preferably no larger.
   *  The bounding_box will always contain this box.
   *  Do not access this field directly, but use dia_object_get_enclosing_box().
   *
   * \protected Use dia_object_get_bounding_box()
   */
  DiaRectangle      bounding_box;
  /*! Number of Handle(s) of this object */
  int               num_handles;
  /*! Array of handles of this object with fixed index */
  Handle          **handles;
  /*! Number of ConnectionPoint this object has */
  int               num_connections;
  /*! Array of ConnectionPoint* - indexing fixed by meaning */
  ConnectionPoint **connections;

  _DIA_OBJECT_FIELD (ObjectOps *, ops); /* pointer to the vtable */

  DiaLayer *parent_layer; /*!< Back-pointer to the owning layer.
			   This may only be set by functions internal to
			   the layer, and accessed via
			   dia_object_get_parent_layer() */
  DiaObject *parent; /*!< Back-pointer to DiaObject which is parenting this object. Can be NULL */
  GList *children; /*!< In case this object is a parent of other object the children are listed here */

  /* The area that contains all parts rendered interactively, so includes
   * handles, bezier controllers etc.  Despite historical difference, this
   * should not be accessed directly, but through dia_object_get_bounding_box().
   * Note that handles and connection points are not included by this, but
   * added by that which needs it.
   * Internal:  If this is set to a NULL, returns bounding_box.  That is for
   * those objects that don't actually calculate it, but can just use the BB.
   * Since handles and CPs are not in the BB, that will be the case for most
   * objects.
   */
  DiaRectangle     *enclosing_box;
  /*! Metainfo of the object, should not be manipulated directly. Use dia_object_set_meta() */
  GHashTable       *meta;
};


/**
 * DiaObjectType:
 * @create: the #CreateFunc
 * @load: the #LoadFunc
 * @save: the #SaveFunc
 * @get_defaults: the #GetDefaultsFunc
 * @apply_defaults: the #ApplyDefaultsFunc
 *
 * Vtable for _DiaObjectType
 *
 * Since: dawn-of-time
 */
struct _ObjectTypeOps {
  CreateFunc        create;
  LoadFunc          load;
  SaveFunc          save;
  GetDefaultsFunc   get_defaults;
  ApplyDefaultsFunc apply_defaults;

  /*< private >*/

  /*
    Unused places (for extension).
    These should be %NULL for now. In the future they might be used.
    Then an older object will be binary compatible, because all new code
    checks if new ops are supported (!= %NULL)
  */
  void      (*unused[10])(DiaObject *obj,...);
};


/*!
   \brief _DiaObject factory

   Structure so that the ObjectFactory can create objects
   of unknown type. (Read in from a shared lib.)
 */

/**
 * DiaObjectType:
 * @name: The type name should follow a pattern of '\<module\> - \<class\>'
 *        like "UML - Class"
 * @version: #DiaObject s must be backward compatible, i.e. support possibly
 *           older versions formats
 * @pixmap: Also put a pixmap in the sheet_object. This one is used if not
 *          in sheet but in toolbar. Stored in xpm format.
 * @ops: Pointer to the vtable
 * @pixmap_file: fallback if pixmap is %NULL
 * @default_user_data: use this if no user data is specified in
 *                     the .sheet file
 * @prop_descs: Property descriptions
 * @prop_offsets: #DiaObject struct offsets
 * @flags: Various flags that can be set for this object, see defines above
 *
 * Since: dawn-of-time
 */
struct _DiaObjectType {
  char                  *name;
  int                    version;
  const char           **pixmap;
  ObjectTypeOps         *ops;
  char                  *pixmap_file;
  void                  *default_user_data;
  const PropDescription *prop_descs;
  const PropOffset      *prop_offsets;
  int                    flags;
};

/* base property stuff ... */
/* Needs to stay ~PROP_FLAG_VISIBLE or the dialogs get seriously messed up.
 * The meta info has special code in property dialog creation.
 */
#define OBJECT_COMMON_PROPERTIES \
  { "obj_pos", PROP_TYPE_POINT, PROP_FLAG_OPTIONAL, \
    "Object position", "Where the object is located"}, \
  { "obj_bb", PROP_TYPE_RECT, PROP_FLAG_OPTIONAL, \
    "Object bounding box", "The bounding box of the object"}, \
  { "meta", PROP_TYPE_DICT, PROP_FLAG_OPTIONAL, \
    "Object meta info", "Some key/value pairs"}

#define OBJECT_COMMON_PROPERTIES_OFFSETS \
  { "obj_pos", PROP_TYPE_POINT, offsetof(DiaObject, position) }, \
  { "obj_bb", PROP_TYPE_RECT, offsetof(DiaObject, bounding_box) }, \
  { "meta", PROP_TYPE_DICT, offsetof(DiaObject, meta) }


void                   dia_object_draw                (DiaObject              *self,
                                                       DiaRenderer            *renderer);
double                 dia_object_distance_from       (DiaObject              *self,
                                                       Point                  *point);
// TODO: Probably shouldn't pass renderer here
void                   dia_object_select              (DiaObject              *self,
                                                       Point                  *point,
                                                       DiaRenderer            *renderer);
// Note: wraps copy
DiaObject             *dia_object_clone               (DiaObject              *self);
DiaObjectChange       *dia_object_move                (DiaObject              *self,
                                                       Point                  *to);
DiaObjectChange       *dia_object_move_handle         (DiaObject              *self,
                                                       Handle                 *handle,
                                                       Point                  *to,
                                                       ConnectionPoint        *cp,
                                                       HandleMoveReason        reason,
                                                       ModifierKeys            modifiers);
// Note: Wraps get_properties
GtkWidget             *dia_object_get_editor          (DiaObject              *self,
                                                       gboolean                is_default);
DiaObjectChange       *dia_object_apply_editor        (DiaObject              *self,
                                                       GtkWidget              *editor);
DiaMenu               *dia_object_get_menu            (DiaObject              *self,
                                                       Point                  *at);
const PropDescription *dia_object_describe_properties (DiaObject              *self);
void                   dia_object_get_properties      (DiaObject              *self,
                                                       GPtrArray              *list);
void                   dia_object_set_properties      (DiaObject              *self,
                                                       GPtrArray              *list);
DiaObjectChange       *dia_object_apply_properties    (DiaObject              *self,
                                                       GPtrArray              *list);
gboolean               dia_object_edit_text           (DiaObject              *self,
                                                       Text                   *text,
                                                       TextEditState           state,
                                                       gchar                  *textchange);
gboolean               dia_object_transform           (DiaObject              *self,
                                                       const DiaMatrix        *m);
void                   dia_object_add_handle           (DiaObject               *self,
                                                        Handle                  *handle,
                                                        int                      index,
                                                        HandleId                 id,
                                                        HandleType               type,
                                                        HandleConnectType        connect_type);
void                   dia_object_add_connection_point (DiaObject               *self,
                                                        ConnectionPoint         *cp,
                                                        int                      index,
                                                        ConnectionPointFlags     flags);


gboolean       dia_object_defaults_load (const gchar *filename,
                                         gboolean create_lazy,
					 DiaContext *ctx);
DiaObject  *dia_object_default_get  (const DiaObjectType *type, gpointer user_data);
DiaObject  *dia_object_default_create (const DiaObjectType *type,
                                    Point *startpoint,
                                    void *user_data,
                                    Handle **handle1,
                                    Handle **handle2);
gboolean         dia_object_defaults_save (const gchar *filename, DiaContext *ctx);
DiaLayer        *dia_object_get_parent_layer(DiaObject *obj);
gboolean         dia_object_is_selected (const DiaObject *obj);
const DiaRectangle *dia_object_get_bounding_box(const DiaObject *obj);
const DiaRectangle *dia_object_get_enclosing_box(const DiaObject *obj);
DiaObject       *dia_object_get_parent_with_flags(DiaObject *obj, guint flags);
gboolean         dia_object_is_selectable(DiaObject *obj);
/* The below is for debugging purposes only. */
gboolean   dia_object_sanity_check(const DiaObject *obj, const gchar *msg);

/** convenience functions for meta info */
void   dia_object_set_meta (DiaObject *obj, const gchar *key, const gchar *value);
gchar *dia_object_get_meta (DiaObject *obj, const gchar *key);

int dia_object_get_num_connections (DiaObject *obj);

/* standard way to load/save properties of an object */
void          object_load_props(DiaObject *obj, ObjectNode obj_node, DiaContext *ctx);
void          object_save_props(DiaObject *obj, ObjectNode obj_node, DiaContext *ctx);

/* standard way to copy the properties of an object into another (of the
   same type) */
void          object_copy_props(DiaObject *dest, const DiaObject *src,
                                gboolean is_default);
void          object_copy_style(DiaObject *dest, const DiaObject *src);

GdkPixbuf    *dia_object_type_get_icon (const DiaObjectType *type);

G_END_DECLS
