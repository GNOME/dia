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
 *
 * File:    class.c
 *
 * Purpose: This file contains implementation of the "class" code.
 */

/** \file objects/UML/class.c  Implementation of the 'UML - Class' type */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#include "diarenderer.h"
#include "attributes.h"
#include "properties.h"
#include "diamenu.h"
#include "class.h"
#include "dia-state-object-change.h"

#include "pixmaps/umlclass.xpm"

#include "debug.h"

#define UMLCLASS_BORDER 0.1
#define UMLCLASS_UNDERLINEWIDTH 0.05
#define UMLCLASS_TEMPLATE_OVERLAY_X 2.3
#define UMLCLASS_TEMPLATE_OVERLAY_Y 0.3

static real umlclass_distance_from(UMLClass *umlclass, Point *point);
static void umlclass_select(UMLClass *umlclass, Point *clicked_point,
			    DiaRenderer *interactive_renderer);
static DiaObjectChange *umlclass_move_handle         (UMLClass         *umlclass,
                                                      Handle           *handle,
                                                      Point            *to,
                                                      ConnectionPoint  *cp,
                                                      HandleMoveReason  reason,
                                                      ModifierKeys      modifiers);
static DiaObjectChange *umlclass_move                (UMLClass         *umlclass,
                                                      Point            *to);
static void umlclass_draw(UMLClass *umlclass, DiaRenderer *renderer);
static DiaObject *umlclass_create(Point *startpoint,
			       void *user_data,
			       Handle **handle1,
			       Handle **handle2);
static void umlclass_destroy(UMLClass *umlclass);
static DiaObject *umlclass_copy(UMLClass *umlclass);

static void umlclass_save(UMLClass *umlclass, ObjectNode obj_node,
			  DiaContext *ctx);
static DiaObject *umlclass_load(ObjectNode obj_node, int version, DiaContext *ctx);

static DiaMenu * umlclass_object_menu(DiaObject *obj, Point *p);
static DiaObjectChange *umlclass_show_comments_callback   (DiaObject *obj,
                                                           Point     *pos,
                                                           gpointer   data);
static DiaObjectChange *umlclass_allow_resizing_callback  (DiaObject *obj,
                                                           Point     *pos,
                                                           gpointer   data);

static PropDescription *umlclass_describe_props(UMLClass *umlclass);
static void umlclass_get_props(UMLClass *umlclass, GPtrArray *props);
static void umlclass_set_props(UMLClass *umlclass, GPtrArray *props);

static void fill_in_fontdata(UMLClass *umlclass);
static int umlclass_num_dynamic_connectionpoints(UMLClass *class);

static DiaObjectChange *_umlclass_apply_props_from_dialog (UMLClass  *umlclass,
                                                           GtkWidget *widget);

static ObjectTypeOps umlclass_type_ops =
{
  (CreateFunc) umlclass_create,
  (LoadFunc)   umlclass_load,
  (SaveFunc)   umlclass_save
};

/**
 * This is the type descriptor for a UML - Class. It contains the
 * information used by Dia to create an object of this type. The structure
 * of this data type is defined in the header file object.h. When a
 * derivation of class is required, then this type can be copied and then
 * change the name and any other fields that are variances from the base
 * type.
*/
DiaObjectType umlclass_type =
{
  "UML - Class",   /* name */
  0,               /* version */
  umlclass_xpm,    /* pixmap */

  &umlclass_type_ops, /* ops */
  NULL,
  (void*)0,
  NULL, /* prop_descs */
  NULL, /* prop_offsets */
  DIA_OBJECT_HAS_VARIANTS /* flags */
};

/** \brief vtable for UMLClass */
static ObjectOps umlclass_ops = {
  (DestroyFunc)         umlclass_destroy,
  (DrawFunc)            umlclass_draw,
  (DistanceFunc)        umlclass_distance_from,
  (SelectFunc)          umlclass_select,
  (CopyFunc)            umlclass_copy,
  (MoveFunc)            umlclass_move,
  (MoveHandleFunc)      umlclass_move_handle,
#if 1
  (GetPropertiesFunc)   umlclass_get_properties,
  (ApplyPropertiesDialogFunc) _umlclass_apply_props_from_dialog,
#else
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
#endif
  (ObjectMenuFunc)      umlclass_object_menu,
  (DescribePropsFunc)   umlclass_describe_props,
  (GetPropsFunc)        umlclass_get_props,
  (SetPropsFunc)        umlclass_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

extern PropDescDArrayExtra umlattribute_extra;
extern PropDescDArrayExtra umloperation_extra;
extern PropDescDArrayExtra umlparameter_extra;
extern PropDescDArrayExtra umlformalparameter_extra;

/** Properties of UMLClass */
static PropDescription umlclass_props[] = {
  ELEMENT_COMMON_PROPERTIES,

  PROP_STD_NOTEBOOK_BEGIN,

  PROP_NOTEBOOK_PAGE("class", PROP_FLAG_DONT_MERGE, N_("Class")),
  { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL | PROP_FLAG_NO_DEFAULTS,
  N_("Name"), NULL, NULL },
  { "stereotype", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Stereotype"), NULL, NULL },
  { "comment", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Comment"), NULL, NULL },

  PROP_MULTICOL_BEGIN("visibilities"),
  PROP_MULTICOL_COLUMN("visible"),
  { "abstract", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Abstract"), NULL, NULL },
  { "visible_attributes", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL | PROP_FLAG_DONT_MERGE,
  N_("Visible Attributes"), NULL, NULL },
  { "visible_operations", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL | PROP_FLAG_DONT_MERGE,
  N_("Visible Operations"), NULL, NULL },
  { "wrap_operations", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Wrap Operations"), NULL, NULL },
  { "visible_comments", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Visible Comments"), NULL, NULL },
  { "comment_tagging", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Comment tagging"), NULL, NULL},
  { "allow_resizing", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Allow resizing"), NULL, NULL},

  PROP_MULTICOL_COLUMN("suppress"),
  { "template", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL | PROP_FLAG_NO_DEFAULTS,
  N_("Template"), NULL, NULL },
  { "suppress_attributes", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL | PROP_FLAG_DONT_MERGE,
  N_("Suppress Attributes"), NULL, NULL },
  { "suppress_operations", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL | PROP_FLAG_DONT_MERGE,
  N_("Suppress Operations"), NULL, NULL },
  { "wrap_after_char", PROP_TYPE_INT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Wrap after char"), NULL, NULL },
  { "comment_line_length", PROP_TYPE_INT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Comment line length"), NULL, NULL},
  { "align_comment_tagging", PROP_TYPE_STATIC, PROP_FLAG_VISIBLE|PROP_FLAG_DONT_SAVE|PROP_FLAG_DONT_MERGE|PROP_FLAG_WIDGET_ONLY,
    "", NULL, NULL },
  { "align_allow_resizing", PROP_TYPE_STATIC, PROP_FLAG_VISIBLE|PROP_FLAG_DONT_SAVE|PROP_FLAG_DONT_MERGE|PROP_FLAG_WIDGET_ONLY,
    "", NULL, NULL },

  PROP_MULTICOL_END("visibilities"),

  PROP_NOTEBOOK_PAGE("attribute", PROP_FLAG_DONT_MERGE | PROP_FLAG_NO_DEFAULTS, N_("Attributes")),
  /* these are used during load, but currently not during save */
  { "attributes", PROP_TYPE_DARRAY, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL | PROP_FLAG_DONT_MERGE | PROP_FLAG_NO_DEFAULTS,
    "", NULL, NULL /* umlattribute_extra */ },

  PROP_NOTEBOOK_PAGE("operations", PROP_FLAG_DONT_MERGE | PROP_FLAG_NO_DEFAULTS, N_("Operations")),
  { "operations", PROP_TYPE_DARRAY, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL | PROP_FLAG_DONT_MERGE | PROP_FLAG_NO_DEFAULTS,
    "", NULL, NULL /* umloperations_extra */ },

  PROP_NOTEBOOK_PAGE("templates", PROP_FLAG_DONT_MERGE | PROP_FLAG_NO_DEFAULTS, N_("Template Parameters")),
  /* the naming is questionable, but kept for compatibility */
  { "templates", PROP_TYPE_DARRAY, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL | PROP_FLAG_DONT_MERGE | PROP_FLAG_NO_DEFAULTS,
    "", NULL, NULL /* umlformalparameters_extra */ },

  /* all this just to make the defaults selectable ... */
  PROP_NOTEBOOK_PAGE("style", PROP_FLAG_DONT_MERGE, N_("Style")),
  PROP_FRAME_BEGIN("fonts", 0, N_("Fonts")),
  PROP_MULTICOL_BEGIN("class"),
  PROP_MULTICOL_COLUMN("font"),
  { "normal_font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Normal"), NULL, NULL },
  { "polymorphic_font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Polymorphic"), NULL, NULL },
  { "abstract_font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Abstract"), NULL, NULL },
  { "classname_font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Class Name"), NULL, NULL },
  { "abstract_classname_font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Abstract Class Name"), NULL, NULL },
  { "comment_font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Comment"), NULL, NULL },

  PROP_MULTICOL_COLUMN("height"),
  { "normal_font_height", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_(" "), NULL, NULL },
  { "polymorphic_font_height", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_(" "), NULL, NULL },
  { "abstract_font_height", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_(" "), NULL, NULL },
  { "classname_font_height", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_(" "), NULL, NULL },
  { "abstract_classname_font_height", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_(" "), NULL, NULL },
  { "comment_font_height", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_(" "), NULL, NULL },
  PROP_MULTICOL_END("class"),
  PROP_FRAME_END("fonts", 0),

  PROP_STD_LINE_WIDTH_OPTIONAL,
  /* can't use PROP_STD_TEXT_COLOUR_OPTIONAL cause it has PROP_FLAG_DONT_SAVE. It is designed to fill the Text object - not some subset */
  PROP_STD_TEXT_COLOUR_OPTIONS(PROP_FLAG_VISIBLE|PROP_FLAG_STANDARD|PROP_FLAG_OPTIONAL),
  PROP_STD_LINE_COLOUR_OPTIONAL,
  PROP_STD_FILL_COLOUR_OPTIONAL,

  PROP_STD_NOTEBOOK_END,

  PROP_DESC_END
};


DiaObjectChange *
_umlclass_apply_props_from_dialog(UMLClass *umlclass, GtkWidget *widget)
{
  DiaObject *obj = &umlclass->element.object;
  /* fallback, if it isn't our dialog, e.g. during multiple selection change */
  if (!umlclass->properties_dialog)
    return object_apply_props_from_dialog (obj, widget);
  else
    return umlclass_apply_props_from_dialog (umlclass, widget);
}


static PropDescription *
umlclass_describe_props(UMLClass *umlclass)
{
 if (umlclass_props[0].quark == 0) {
    int i = 0;

    prop_desc_list_calculate_quarks(umlclass_props);
    while (umlclass_props[i].name != NULL) {
      /* can't do this static, at least not on win32
       * due to relocation (initializer not a constant)
       */
      if (0 == strcmp(umlclass_props[i].name, "attributes"))
        umlclass_props[i].extra_data = &umlattribute_extra;
      else if (0 == strcmp(umlclass_props[i].name, "operations")) {
        PropDescription *records = umloperation_extra.common.record;
        int j = 0;

        umlclass_props[i].extra_data = &umloperation_extra;
	while (records[j].name != NULL) {
          if (0 == strcmp(records[j].name, "parameters"))
	    records[j].extra_data = &umlparameter_extra;
	  j++;
	}
      }
      else if (0 == strcmp(umlclass_props[i].name, "templates"))
        umlclass_props[i].extra_data = &umlformalparameter_extra;

      i++;
    }
  }
  return umlclass_props;
}

static PropOffset umlclass_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,

  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(UMLClass, line_width) },
  { "text_colour", PROP_TYPE_COLOUR, offsetof(UMLClass, text_color) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(UMLClass, line_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(UMLClass, fill_color) },
  { "name", PROP_TYPE_STRING, offsetof(UMLClass, name) },
  { "stereotype", PROP_TYPE_STRING, offsetof(UMLClass, stereotype) },
  { "comment", PROP_TYPE_STRING, offsetof(UMLClass, comment) },
  { "abstract", PROP_TYPE_BOOL, offsetof(UMLClass, abstract) },
  { "template", PROP_TYPE_BOOL, offsetof(UMLClass, template) },
  { "suppress_attributes", PROP_TYPE_BOOL, offsetof(UMLClass , suppress_attributes) },
  { "visible_attributes", PROP_TYPE_BOOL, offsetof(UMLClass , visible_attributes) },
  { "visible_comments", PROP_TYPE_BOOL, offsetof(UMLClass , visible_comments) },
  { "suppress_operations", PROP_TYPE_BOOL, offsetof(UMLClass , suppress_operations) },
  { "visible_operations", PROP_TYPE_BOOL, offsetof(UMLClass , visible_operations) },
  { "visible_comments", PROP_TYPE_BOOL, offsetof(UMLClass , visible_comments) },
  { "wrap_operations", PROP_TYPE_BOOL, offsetof(UMLClass , wrap_operations) },
  { "wrap_after_char", PROP_TYPE_INT, offsetof(UMLClass , wrap_after_char) },
  { "comment_line_length", PROP_TYPE_INT, offsetof(UMLClass, comment_line_length) },
  { "comment_tagging", PROP_TYPE_BOOL, offsetof(UMLClass, comment_tagging) },
  { "allow_resizing", PROP_TYPE_BOOL, offsetof(UMLClass, allow_resizing) },

  /* all this just to make the defaults selectable ... */
  PROP_OFFSET_MULTICOL_BEGIN("class"),
  PROP_OFFSET_MULTICOL_COLUMN("font"),
  { "normal_font", PROP_TYPE_FONT, offsetof(UMLClass, normal_font) },
  { "abstract_font", PROP_TYPE_FONT, offsetof(UMLClass, abstract_font) },
  { "polymorphic_font", PROP_TYPE_FONT, offsetof(UMLClass, polymorphic_font) },
  { "classname_font", PROP_TYPE_FONT, offsetof(UMLClass, classname_font) },
  { "abstract_classname_font", PROP_TYPE_FONT, offsetof(UMLClass, abstract_classname_font) },
  { "comment_font", PROP_TYPE_FONT, offsetof(UMLClass, comment_font) },

  PROP_OFFSET_MULTICOL_COLUMN("height"),
  { "normal_font_height", PROP_TYPE_REAL, offsetof(UMLClass, font_height) },
  { "abstract_font_height", PROP_TYPE_REAL, offsetof(UMLClass, abstract_font_height) },
  { "polymorphic_font_height", PROP_TYPE_REAL, offsetof(UMLClass, polymorphic_font_height) },
  { "classname_font_height", PROP_TYPE_REAL, offsetof(UMLClass, classname_font_height) },
  { "abstract_classname_font_height", PROP_TYPE_REAL, offsetof(UMLClass, abstract_classname_font_height) },
  { "comment_font_height", PROP_TYPE_REAL, offsetof(UMLClass, comment_font_height) },
  PROP_OFFSET_MULTICOL_END("class"),

  { "operations", PROP_TYPE_DARRAY, offsetof(UMLClass , operations) },
  { "attributes", PROP_TYPE_DARRAY, offsetof(UMLClass , attributes) } ,
  { "templates",  PROP_TYPE_DARRAY, offsetof(UMLClass , formal_params) } ,

  { NULL, 0, 0 },
};

static void
umlclass_get_props(UMLClass * umlclass, GPtrArray *props)
{
  object_get_props_from_offsets(&umlclass->element.object,
                                umlclass_offsets, props);
}

static DiaMenuItem umlclass_menu_items[] = {
        { N_("Show comments"), umlclass_show_comments_callback, NULL,
          DIAMENU_ACTIVE|DIAMENU_TOGGLE },
        { N_("Allow resizing"), umlclass_allow_resizing_callback, NULL,
          DIAMENU_ACTIVE|DIAMENU_TOGGLE },
};

static DiaMenu umlclass_menu = {
        N_("Class"),
        sizeof(umlclass_menu_items)/sizeof(DiaMenuItem),
        umlclass_menu_items,
        NULL
};

DiaMenu *
umlclass_object_menu(DiaObject *obj, Point *p)
{
        umlclass_menu_items[0].active = DIAMENU_ACTIVE|DIAMENU_TOGGLE|
                (((UMLClass *)obj)->visible_comments?DIAMENU_TOGGLE_ON:0);
        umlclass_menu_items[1].active = DIAMENU_ACTIVE|DIAMENU_TOGGLE|
                (((UMLClass *)obj)->allow_resizing?DIAMENU_TOGGLE_ON:0);

        return &umlclass_menu;
}


typedef struct _CommentState {
  ObjectState state;
  gboolean    visible_comments;
} CommentState;


static ObjectState*
_comment_get_state (DiaObject *obj)
{
  CommentState *state = g_new (CommentState,1);
  state->state.free = NULL; /* we don't have any pointers to free */
  state->visible_comments = ((UMLClass *)obj)->visible_comments;
  return (ObjectState *)state;
}


static void
_comment_set_state (DiaObject *obj, ObjectState *state)
{
  ((UMLClass *) obj)->visible_comments = ((CommentState *) state)->visible_comments;
  g_free (state); /* rather strange convention set_state consumes the state */
  umlclass_calculate_data ((UMLClass *)obj);
  umlclass_update_data ((UMLClass *)obj);
}


static DiaObjectChange *
umlclass_show_comments_callback (DiaObject *obj, Point *pos, gpointer data)
{
  ObjectState *old_state = _comment_get_state (obj);
  DiaObjectChange *change = dia_state_object_change_new (obj,
                                                         old_state,
                                                         _comment_get_state,
                                                         _comment_set_state);

  ((UMLClass *) obj)->visible_comments = !((UMLClass *) obj)->visible_comments;
  umlclass_calculate_data ((UMLClass *) obj);
  umlclass_update_data ((UMLClass *) obj);

  return change;
}


static DiaObjectChange *
umlclass_allow_resizing_callback (DiaObject *obj,
                                  Point     *pos G_GNUC_UNUSED,
                                  gpointer   data G_GNUC_UNUSED)
{
  return object_toggle_prop (obj,
                             "allow_resizing",
                             !((UMLClass *) obj)->allow_resizing);
}


static void
umlclass_reflect_resizing(UMLClass *umlclass)
{
  Element *elem = &umlclass->element;

  element_update_handles(elem);

  g_assert (elem->resize_handles[3].id == HANDLE_RESIZE_W);
  g_assert (elem->resize_handles[4].id == HANDLE_RESIZE_E);

  elem->resize_handles[3].type = umlclass->allow_resizing ? HANDLE_MAJOR_CONTROL : HANDLE_NON_MOVABLE;
  elem->resize_handles[4].type = umlclass->allow_resizing ? HANDLE_MAJOR_CONTROL : HANDLE_NON_MOVABLE;
}

static void
umlclass_set_props(UMLClass *umlclass, GPtrArray *props)
{
  /* now that operations/attributes can be set here as well we need to
   * take for the number of connections update as well
   * Note that due to a hack in umlclass_load, this is called before
   * the normal connection points are set up.
   */
  DiaObject *obj = &umlclass->element.object;
  GList *list;
  int num;

  object_set_props_from_offsets(&umlclass->element.object, umlclass_offsets,
                                props);

  num = UMLCLASS_CONNECTIONPOINTS + umlclass_num_dynamic_connectionpoints(umlclass);

#ifdef UML_MAINPOINT
  obj->num_connections = num + 1;
#else
  obj->num_connections = num;
#endif

  obj->connections = g_renew (ConnectionPoint *,
                              obj->connections,
                              obj->num_connections);

  /* Update data: */
  if (num > UMLCLASS_CONNECTIONPOINTS) {
    int i;
    /* this is just updating pointers to ConnectionPoint, the real connection handling is elsewhere.
     * Note: Can't optimize here on number change cause the ops/attribs may have changed regardless of that.
     */
    i = UMLCLASS_CONNECTIONPOINTS;
    list = (!umlclass->visible_attributes || umlclass->suppress_attributes) ? NULL : umlclass->attributes;
    while (list != NULL) {
      UMLAttribute *attr = (UMLAttribute *)list->data;

      uml_attribute_ensure_connection_points (attr, obj);
      obj->connections[i] = attr->left_connection;
      obj->connections[i]->object = obj;
      i++;
      obj->connections[i] = attr->right_connection;
      obj->connections[i]->object = obj;
      i++;
      list = g_list_next(list);
    }
    list = (!umlclass->visible_operations || umlclass->suppress_operations) ? NULL : umlclass->operations;
    while (list != NULL) {
      UMLOperation *op = (UMLOperation *)list->data;

      uml_operation_ensure_connection_points (op, obj);
      obj->connections[i] = op->left_connection;
      obj->connections[i]->object = obj;
      i++;
      obj->connections[i] = op->right_connection;
      obj->connections[i]->object = obj;
      i++;
      list = g_list_next(list);
    }
  }
#ifdef UML_MAINPOINT
  obj->connections[num] = &umlclass->connections[UMLCLASS_CONNECTIONPOINTS];
  obj->connections[num]->object = obj;
#endif

  umlclass_reflect_resizing(umlclass);

  umlclass_calculate_data(umlclass);
  umlclass_update_data(umlclass);
#ifdef DEBUG
  /* Would like to sanity check here, but the call to object_load_props
   * in umlclass_load means we will be called with inconsistent data. */
  umlclass_sanity_check(umlclass, "After updating data");
#endif
}

static real
umlclass_distance_from(UMLClass *umlclass, Point *point)
{
  DiaObject *obj = &umlclass->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
umlclass_select(UMLClass *umlclass, Point *clicked_point,
	       DiaRenderer *interactive_renderer)
{
  element_update_handles(&umlclass->element);
}


static DiaObjectChange *
umlclass_move_handle (UMLClass         *umlclass,
                      Handle           *handle,
                      Point            *to,
                      ConnectionPoint  *cp,
                      HandleMoveReason  reason,
                      ModifierKeys      modifiers)
{
  Element *elem = &umlclass->element;

  g_return_val_if_fail (umlclass != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);
  g_return_val_if_fail (handle->id < UMLCLASS_CONNECTIONPOINTS, NULL);

  if (handle->type != HANDLE_NON_MOVABLE) {
    if (handle->id == HANDLE_RESIZE_E || handle->id == HANDLE_RESIZE_W) {
      double dist = (handle->id == HANDLE_RESIZE_E) ?
                        to->x - elem->resize_handles[3].pos.x :
                        elem->resize_handles[4].pos.x - to->x;
      if (umlclass->min_width <= dist) {
        DiaObjectChange *oc = element_move_handle (elem,
                                                   handle->id,
                                                   to,
                                                   cp,
                                                   reason,
                                                   modifiers);
        umlclass_update_data (umlclass);
        return oc;
      }
    }
  }

  return NULL;
}


static DiaObjectChange *
umlclass_move (UMLClass *umlclass, Point *to)
{
  umlclass->element.corner = *to;
  umlclass_update_data (umlclass);

  return NULL;
}


/**
 * underlines the text at the start point using the text to determine
 * the length of the underline. Draw a line under the text represented by
 * string using the selected renderer, color, and line width.  Since
 * drawing this line will change the line width used by DIA, the current
 * line width that DIA is using is also passed so it can be restored once
 * the line has been drawn.
 *
 * @param  renderer     the renderer that will draw the line
 * @param  StartPoint   the start of the line to be drawn
 * @param  font         the font used to draw the text being underlined
 * @param  font_height  the size in the y direction of the font used to draw the text
 * @param  string       the text string that is to be underlined
 * @param  color        the color of the line to draw
 * @param  line_width   default line thickness
 * @param  underline_width   the thickness of the line to draw
 *
 */
static void
uml_underline_text (DiaRenderer  *renderer,
                    Point         StartPoint,
                    DiaFont      *font,
                    real          font_height,
                    gchar        *string,
                    Color        *color,
                    real          line_width,
                    real          underline_width)
{
  Point UnderlineStartPoint;
  Point UnderlineEndPoint;
  char *whitespaces;
  int first_non_whitespace = 0;

  UnderlineStartPoint = StartPoint;
  UnderlineStartPoint.y += font_height * 0.1;
  UnderlineEndPoint = UnderlineStartPoint;

  whitespaces = string;
  while (whitespaces &&
         g_unichar_isspace (g_utf8_get_char (whitespaces))) {
    whitespaces = g_utf8_next_char (whitespaces);
  }
  first_non_whitespace = whitespaces - string;
  whitespaces = g_strdup (string);
  whitespaces[first_non_whitespace] = '\0';
  UnderlineStartPoint.x += dia_font_string_width (whitespaces, font, font_height);
  g_clear_pointer (&whitespaces, g_free);
  UnderlineEndPoint.x += dia_font_string_width (string, font, font_height);
  dia_renderer_set_linewidth (renderer, underline_width);
  dia_renderer_draw_line (renderer, &UnderlineStartPoint, &UnderlineEndPoint, color);
  dia_renderer_set_linewidth (renderer, line_width);
}

/**
 * Create a documentation tag from a comment.
 *
 * First a string is created containing only the text
 * "{documentation = ". Then the contents of the comment string
 * are added but wrapped. This is done by first looking for any
 * New Line characters. If the line segment is longer than the
 * WrapPoint would allow, the line is broken at either the
 * first whitespace before the WrapPoint or if there are no
 * whitespaces in the segment, at the WrapPoint.  This
 * continues until the entire string has been processed and
 * then the resulting new string is returned. No attempt is
 * made to rejoin any of the segments, that is all New Lines
 * are treated as hard newlines. No syllable matching is done
 * either so breaks in words will sometimes not make real
 * sense.
 * <p>
 * Finally, since this function returns newly created dynamic
 * memory the caller must free the memory to prevent memory
 * leaks.
 *
 * @param  comment       The comment to be wrapped to the line length limit
 * @param  WrapPoint     The maximum line length allowed for the line.
 * @param  NumberOfLines The number of comment lines after the wrapping.
 * @return               a pointer to the wrapped documentation
 *
 *  NOTE:
 *      This function should most likely be move to a source file for
 *      handling global UML functionallity at some point.
 */
static gchar *
uml_create_documentation_tag (gchar * comment,
                              gboolean tagging,
			      gint WrapPoint,
			      gint *NumberOfLines)
{
  gchar  *CommentTag           = tagging ? "{documentation = " : "";
  gint   TagLength             = strlen(CommentTag);
  /* Make sure that there is at least some value greater then zero for the WrapPoint to
   * support diagrams from earlier versions of Dia. So if the WrapPoint is zero then use
   * the taglength as the WrapPoint. If the Tag has been changed such that it has a length
   * of 0 then use 1.
   */
  gint     WorkingWrapPoint = (TagLength<WrapPoint) ? WrapPoint : ((TagLength<=0)?1:TagLength);
  gint     RawLength        = TagLength + strlen(comment) + (tagging?1:0);
  gint     MaxCookedLength  = RawLength + RawLength/WorkingWrapPoint;
  gchar    *WrappedComment  = g_new0 (char, MaxCookedLength + 1);
  gint     AvailSpace       = WorkingWrapPoint - TagLength;
  gchar    *Scan;
  gchar    *BreakCandidate;
  gunichar ScanChar;
  gboolean AddNL            = FALSE;

  if (tagging)
    strcat(WrappedComment, CommentTag);
  *NumberOfLines = 1;

  while ( *comment ) {
    /* Skip spaces */
    while ( *comment && g_unichar_isspace(g_utf8_get_char(comment)) ) {
        comment = g_utf8_next_char(comment);
    }
    /* Copy chars */
    if ( *comment ){
      /* Scan to \n or avalable space exhausted */
      Scan = comment;
      BreakCandidate = NULL;
      while ( *Scan && *Scan != '\n' && AvailSpace > 0 ) {
        ScanChar = g_utf8_get_char(Scan);
        /* We known, that g_unichar_isspace() is not recommended for word breaking;
         * but Pango usage seems too complex.
         */
        if ( g_unichar_isspace(ScanChar) )
          BreakCandidate = Scan;
        AvailSpace--; /* not valid for nonspacing marks */
        Scan = g_utf8_next_char(Scan);
      }
      if ( AvailSpace==0 && BreakCandidate != NULL )
        Scan = BreakCandidate;
      if ( AddNL ){
        strcat(WrappedComment, "\n");
        *NumberOfLines+=1;
      }
      AddNL = TRUE;
      strncat(WrappedComment, comment, Scan-comment);
        AvailSpace = WorkingWrapPoint;
      comment = Scan;
    }
  }
  if (tagging)
    strcat(WrappedComment, "}");

  g_return_val_if_fail (strlen (WrappedComment) <= MaxCookedLength, NULL);

  return WrappedComment;
}

/**
 * Draw the comment at the point, p, using the comment font from the
 * class defined by umlclass. When complete update the point to reflect
 * the size of data drawn.
 * The comment will have been word wrapped using the function
 * uml_create_documentation_tag, so it may have more than one line on the
 * display.
 *
 * @param   renderer            The Renderer on which the comment is being drawn
 * @param   font                The font to render the comment in.
 * @param   font_height         The Y size of the font used to render the comment
 * @param   text_color          A pointer to the color to use to render the comment
 * @param   comment             The comment string to render
 * @param   comment_tagging     If the {documentation = } tag should be enforced
 * @param   Comment_line_length The maximum length of any one line in the comment
 * @param   p                   The point at which the comment is to start
 * @param   alignment           The method to use for alignment of the font
 * @see   uml_create_documentation
 */
static void
uml_draw_comments (DiaRenderer *renderer,
                   DiaFont     *font,
                   real         font_height,
                   Color       *text_color,
                   gchar       *comment,
                   gboolean     comment_tagging,
                   gint         Comment_line_length,
                   Point       *p,
                   gint         alignment)
{
  gint      NumberOfLines = 0;
  gint      Index;
  real      ascent;
  gchar     *CommentString = 0;
  gchar     *NewLineP= NULL;
  gchar     *RenderP;

  CommentString =
        uml_create_documentation_tag (comment, comment_tagging, Comment_line_length, &NumberOfLines);
  RenderP = CommentString;
  dia_renderer_set_font (renderer, font, font_height);
  ascent = dia_font_ascent (RenderP, font, font_height);
  for ( Index=0; Index < NumberOfLines; Index++) {
    NewLineP = strchr (RenderP, '\n');
    if ( NewLineP != NULL) {
      *NewLineP++ = '\0';
    }
    if (Index == 0) {
      p->y += ascent;
    } else {
      p->y += font_height;                    /* Advance to the next line */
    }
    dia_renderer_draw_string (renderer, RenderP, p, alignment, text_color);
    RenderP = NewLineP;
    if (NewLineP == NULL) {
        break;
    }
  }
  p->y += font_height - ascent;
  g_clear_pointer (&CommentString, g_free);
}


/**
 * Draw the name box of the class icon. According to the UML specification,
 * the Name box or compartment is the top most compartment of the class
 * icon. It may contain one or more stereotype declarations, followed by
 * the name of the class. The name may be rendered to indicate abstraction
 * for abstract classes. Following the name is any tagged values such as
 * the {documentation = } tag.
 * <p>
 * Because the start point is the upper left of the class box, templates
 * tend to get lost when created. By applying an offset, they will not be
 * lost. The offset should only be added if the elem->corner.y = 0.
 *
 * @param umlclass  The pointer to the class being drawn
 * @param renderer  The pointer to the rendering object used to draw
 * @param elem      The pointer to the element within the class to be drawn
 * @param offset    offset from start point
 * @return  The offset from the start of the class to the bottom of the namebox
 *
 */
static real
umlclass_draw_namebox (UMLClass    *umlclass,
                       DiaRenderer *renderer,
                       Element     *elem)
{
  real     font_height;
  real     ascent;
  DiaFont *font;
  Point   StartPoint;
  Point   LowerRightPoint;
  real    Yoffset;
  Color   *text_color = &umlclass->text_color;

  StartPoint.x = elem->corner.x;
  StartPoint.y = elem->corner.y;

  Yoffset = elem->corner.y + umlclass->namebox_height;

  LowerRightPoint = StartPoint;
  LowerRightPoint.x += elem->width;
  LowerRightPoint.y  = Yoffset;

  /* First draw the outer box and fill color for the class name object */
  dia_renderer_draw_rect (renderer,
                          &StartPoint,
                          &LowerRightPoint,
                          &umlclass->fill_color,
                          &umlclass->line_color);

  /* Start at the midpoint on the X axis */
  StartPoint.x += elem->width / 2.0;
  StartPoint.y += 0.2;

  /* stereotype: */
  if (umlclass->stereotype != NULL && umlclass->stereotype[0] != '\0') {
    char *string = umlclass->stereotype_string;
    ascent = dia_font_ascent (string,
                              umlclass->normal_font,
                              umlclass->font_height);
    StartPoint.y += ascent;
    dia_renderer_set_font (renderer,
                           umlclass->normal_font,
                           umlclass->font_height);
    dia_renderer_draw_string (renderer,
                              string,
                              &StartPoint,
                              DIA_ALIGN_CENTRE,
                              text_color);
    StartPoint.y += umlclass->font_height - ascent;
  }

  /* name: */
  if (umlclass->name != NULL) {
    if (umlclass->abstract) {
      font = umlclass->abstract_classname_font;
      font_height = umlclass->abstract_classname_font_height;
    } else {
      font = umlclass->classname_font;
      font_height = umlclass->classname_font_height;
    }
    ascent = dia_font_ascent (umlclass->name, font, font_height);
    StartPoint.y += ascent;

    dia_renderer_set_font (renderer, font, font_height);
    dia_renderer_draw_string (renderer,
                              umlclass->name,
                              &StartPoint,
                              DIA_ALIGN_CENTRE,
                              text_color);
    StartPoint.y += font_height - ascent;
  }

  /* comment */
  if (umlclass->visible_comments && umlclass->comment != NULL && umlclass->comment[0] != '\0'){
    uml_draw_comments (renderer,
                       umlclass->comment_font,
                       umlclass->comment_font_height,
                       &umlclass->text_color,
                       umlclass->comment,
                       umlclass->comment_tagging,
                       umlclass->comment_line_length,
                       &StartPoint,
                       DIA_ALIGN_CENTRE);
  }
  return Yoffset;
}

/**
 * Draw the attribute box.
 * This attribute box follows the name box in the class icon. If the
 * attributes are not suppress, draw each of the attributes following the
 * UML specification for displaying attributes. Each attribute is preceded
 * by the visibility character, +, - or # depending on whether it is public
 * private or protected. If the attribute is "abstract" it will be rendered
 * using the abstract font otherwise it will be rendered using the normal
 * font. If the attribute is of class scope, static in C++, then it will be
 * underlined. If there is a comment associated with the attribute, that is
 * within the class description, it will be rendered as a uml comment.
 *
 * @param umlclass   The pointer to the class being drawn
 * @param renderer   The pointer to the rendering object used to draw
 * @param elem       The pointer to the element within the class to be drawn
 * @param Yoffset    The Y offset from the start of the class at which to draw the attributebox
 * @return           The offset from the start of the class to the bottom of the attributebox
 * @see uml_draw_comments
 */
static real
umlclass_draw_attributebox (UMLClass    *umlclass,
                            DiaRenderer *renderer,
                            Element     *elem,
                            real         Yoffset)
{
  real     font_height;
  real     ascent;
  Point    StartPoint;
  Point    LowerRight;
  DiaFont *font;
  Color   *fill_color = &umlclass->fill_color;
  Color   *line_color = &umlclass->line_color;
  Color   *text_color = &umlclass->text_color;
  GList   *list;

  StartPoint.x = elem->corner.x;
  StartPoint.y = Yoffset;
  Yoffset   += umlclass->attributesbox_height;

  LowerRight   = StartPoint;
  LowerRight.x += elem->width;
  LowerRight.y = Yoffset;

  dia_renderer_draw_rect (renderer, &StartPoint, &LowerRight, fill_color, line_color);

  if (!umlclass->suppress_attributes) {
    gint i = 0;
    StartPoint.x += (umlclass->line_width/2.0 + 0.1);
    StartPoint.y += 0.1;

    list = umlclass->attributes;
    while (list != NULL) {
      UMLAttribute *attr   = (UMLAttribute *)list->data;
      gchar        *attstr = uml_attribute_get_string (attr);

      if (attr->abstract)  {
        font = umlclass->abstract_font;
        font_height = umlclass->abstract_font_height;
      } else {
        font = umlclass->normal_font;
        font_height = umlclass->font_height;
      }
      ascent = dia_font_ascent (attstr, font, font_height);
      StartPoint.y += ascent;
      dia_renderer_set_font (renderer, font, font_height);
      dia_renderer_draw_string (renderer,
                                attstr,
                                &StartPoint,
                                DIA_ALIGN_LEFT,
                                text_color);
      StartPoint.y += font_height - ascent;

      if (attr->class_scope) {
        uml_underline_text (renderer,
                            StartPoint,
                            font,
                            font_height,
                            attstr,
                            line_color,
                            umlclass->line_width,
                            UMLCLASS_UNDERLINEWIDTH);
      }

      if (umlclass->visible_comments && attr->comment != NULL && attr->comment[0] != '\0') {
        uml_draw_comments (renderer,
                           umlclass->comment_font,
                           umlclass->comment_font_height,
                           &umlclass->text_color,
                           attr->comment,
                           umlclass->comment_tagging,
                           umlclass->comment_line_length,
                           &StartPoint,
                           DIA_ALIGN_LEFT);
        StartPoint.y += umlclass->comment_font_height/2;
      }
      list = g_list_next (list);
      i++;
      g_clear_pointer (&attstr, g_free);
    }
  }
  return Yoffset;
}


/**
 * Draw the operations box. The operations block follows the attribute box
 * if it is visible. If the operations are not suppressed, they are
 * displayed in the operations box. Like the attributes, operations have
 * visibility characters, +,-, and # indicating whether the are public,
 * private or protected. The operations are rendered in different fonts
 * depending on whether they are abstract (pure virtual), polymorphic
 * (virtual) or leaf (final virtual or non-virtual). The parameters to the
 * operation may be displayed and if they are they may be conditionally
 * wrapped to reduce horizontial size of the icon.
 *
 * @param umlclass  The pointer to the class being drawn
 * @param renderer  The pointer to the rendering object used to draw
 * @param elem      The pointer to the element within the class to be drawn
 * @param Yoffset   The Y offset from the start of the class at which to draw the operationbox
 * @return          The offset from the start of the class to the bottom of the operationbox
 *
 */
static real
umlclass_draw_operationbox (UMLClass    *umlclass,
                            DiaRenderer *renderer,
                            Element     *elem,
                            real         Yoffset)
{
  real     font_height;
  Point    StartPoint;
  Point    LowerRight;
  DiaFont *font;
  GList   *list;
  Color   *fill_color = &umlclass->fill_color;
  Color   *line_color = &umlclass->line_color;
  Color   *text_color = &umlclass->text_color;


  StartPoint.x = elem->corner.x;
  StartPoint.y = Yoffset;
  Yoffset   += umlclass->operationsbox_height;

  LowerRight   = StartPoint;
  LowerRight.x += elem->width;
  LowerRight.y = Yoffset;

  dia_renderer_draw_rect (renderer, &StartPoint, &LowerRight, fill_color, line_color);

  if (!umlclass->suppress_operations) {
    gint i = 0;
    GList *wrapsublist = NULL;
    gchar *part_opstr = NULL;
    int wrap_pos, last_wrap_pos, ident;
    int part_opstr_len = 0, part_opstr_need = 0;

    StartPoint.x += (umlclass->line_width/2.0 + 0.1);
    StartPoint.y += 0.1;

    list = umlclass->operations;
    while (list != NULL) {
      UMLOperation *op = (UMLOperation *)list->data;
      gchar* opstr = uml_get_operation_string(op);
      real ascent;

      switch (op->inheritance_type) {
        case DIA_UML_ABSTRACT:
          font = umlclass->abstract_font;
          font_height = umlclass->abstract_font_height;
          break;
        case DIA_UML_POLYMORPHIC:
          font = umlclass->polymorphic_font;
          font_height = umlclass->polymorphic_font_height;
          break;
        case DIA_UML_LEAF:
        default:
          font = umlclass->normal_font;
          font_height = umlclass->font_height;
      }

      ascent = dia_font_ascent (opstr, font, font_height);
      op->ascent = ascent;
      dia_renderer_set_font (renderer, font, font_height);

      if( umlclass->wrap_operations && op->needs_wrapping) {
        ident = op->wrap_indent;
        wrapsublist = op->wrappos;
        last_wrap_pos = 0;

        while( wrapsublist != NULL) {
          wrap_pos = GPOINTER_TO_INT (wrapsublist->data);

          if (last_wrap_pos == 0) {
            part_opstr_need = wrap_pos + 1;
            if (part_opstr_len < part_opstr_need) {
              part_opstr_len = part_opstr_need;
              part_opstr = g_renew (char, part_opstr, part_opstr_need);
            } else {
              /* ensure to never strncpy to NULL and not shrink */
              part_opstr = g_renew (char,
                                    part_opstr,
                                    MAX (part_opstr_need, part_opstr_len));
            }
            strncpy (part_opstr, opstr, wrap_pos);
            memset (part_opstr+wrap_pos, '\0', 1);
          } else   {
            part_opstr_need = ident + wrap_pos - last_wrap_pos + 1;
            if (part_opstr_len < part_opstr_need) {
              part_opstr_len = part_opstr_need;
              part_opstr = g_renew (char, part_opstr, part_opstr_need);
            }
            memset (part_opstr, ' ', ident);
            memset (part_opstr+ident, '\0', 1);
            strncat (part_opstr, opstr+last_wrap_pos, wrap_pos-last_wrap_pos);
          }

          if( last_wrap_pos == 0 ) {
            StartPoint.y += ascent;
          } else {
            StartPoint.y += font_height;
          }
          dia_renderer_draw_string (renderer,
                                    part_opstr,
                                    &StartPoint,
                                    DIA_ALIGN_LEFT,
                                    text_color);
          if (op->class_scope) {
            uml_underline_text (renderer,
                                StartPoint,
                                font,
                                font_height,
                                part_opstr,
                                line_color,
                                umlclass->line_width,
                                UMLCLASS_UNDERLINEWIDTH);
          }
          last_wrap_pos = wrap_pos;
          wrapsublist = g_list_next (wrapsublist);
        }
      } else {
        StartPoint.y += ascent;
        dia_renderer_draw_string (renderer,
                                  opstr,
                                  &StartPoint,
                                  DIA_ALIGN_LEFT,
                                  text_color);
        if (op->class_scope) {
          uml_underline_text (renderer,
                              StartPoint,
                              font,
                              font_height,
                              opstr,
                              line_color,
                              umlclass->line_width,
                              UMLCLASS_UNDERLINEWIDTH);
        }
      }


      StartPoint.y += font_height - ascent;

      if (umlclass->visible_comments && op->comment != NULL && op->comment[0] != '\0') {
        uml_draw_comments (renderer,
                           umlclass->comment_font,
                           umlclass->comment_font_height,
                           &umlclass->text_color,
                           op->comment,
                           umlclass->comment_tagging,
                           umlclass->comment_line_length,
                           &StartPoint,
                           DIA_ALIGN_LEFT);
        StartPoint.y += umlclass->comment_font_height / 2;
      }

      list = g_list_next (list);
      i++;
      g_clear_pointer (&opstr, g_free);
    }
    g_clear_pointer (&part_opstr, g_free);
  }
  return Yoffset;
}

/**
 * Draw the template parameters box in the upper right hand corner of the
 * class box for paramertize classes (aka template classes). Fill in this
 * box with the parameters for the class.
 * <p>
 * At this time there is no provision for adding comments or documentation
 * to the display.
 *
 * @param umlclass  The pointer to the class being drawn
 * @param renderer  The pointer to the rendering object used to draw
 * @param elem      The pointer to the element within the class to be drawn
 *
 */
static void
umlclass_draw_template_parameters_box (UMLClass    *umlclass,
                                       DiaRenderer *renderer,
                                       Element     *elem)
{
  Point UpperLeft;
  Point LowerRight;
  Point TextInsert;
  GList *list;
  gint   i;
  DiaFont *font = umlclass->normal_font;
  real     font_height = umlclass->font_height;
  real     ascent;
  Color   *fill_color = &umlclass->fill_color;
  Color   *line_color = &umlclass->line_color;
  Color   *text_color = &umlclass->text_color;


  /*
   * Adjust for the overlay of the template on the class icon
   */
  UpperLeft.x = elem->corner.x + elem->width - UMLCLASS_TEMPLATE_OVERLAY_X;
  UpperLeft.y =  elem->corner.y - umlclass->templates_height + UMLCLASS_TEMPLATE_OVERLAY_Y;
  TextInsert = UpperLeft;
  LowerRight = UpperLeft;
  LowerRight.x += umlclass->templates_width;
  LowerRight.y += umlclass->templates_height;

  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_DASHED, 0.3);
  dia_renderer_draw_rect (renderer, &UpperLeft, &LowerRight, fill_color, line_color);

  TextInsert.x += 0.3;
  TextInsert.y += 0.1;
  dia_renderer_set_font (renderer, font, font_height);
  i = 0;
  list = umlclass->formal_params;
  while (list != NULL) {
    char *paramstr = uml_formal_parameter_get_string ((UMLFormalParameter *) list->data);

    ascent = dia_font_ascent (paramstr, font, font_height);
    TextInsert.y += ascent;
    dia_renderer_draw_string (renderer,
                              paramstr,
                              &TextInsert,
                              DIA_ALIGN_LEFT,
                              text_color);
    TextInsert.y += font_height - ascent;

    list = g_list_next (list);
    i++;
    g_clear_pointer (&paramstr, g_free);
  }
}


/**
 * Draw the class icon for the specified UMLClass object.
 * Set the renderer to the correct fill and line styles and the appropriate
 * line width.  The object is drawn by the umlclass_draw_namebox,
 * umlclass_draw_attributebox, umlclass_draw_operationbox and
 * umlclass_draw_template_parameters_box.
 *
 * @param  umlclass   object based on the uml class that is being rendered
 * @param   DiaRenderer  renderer used to draw the object
 *
 */
static void
umlclass_draw (UMLClass *umlclass, DiaRenderer *renderer)
{
  double y = 0.0;
  Element *elem;

  g_return_if_fail (umlclass != NULL);
  g_return_if_fail (renderer != NULL);

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linewidth (renderer, umlclass->line_width);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

  elem = &umlclass->element;

  y = umlclass_draw_namebox (umlclass, renderer, elem);
  if (umlclass->visible_attributes) {
    y = umlclass_draw_attributebox (umlclass, renderer, elem, y);
  }
  if (umlclass->visible_operations) {
    umlclass_draw_operationbox (umlclass, renderer, elem, y);
  }
  if (umlclass->template) {
    umlclass_draw_template_parameters_box (umlclass, renderer, elem);
  }
}

void
umlclass_update_data(UMLClass *umlclass)
{
  Element *elem = &umlclass->element;
  DiaObject *obj = &elem->object;
  real x,y;
  GList *list;
  int i;
  int pointswide;
  int lowerleftcorner;
  real pointspacing;

  x = elem->corner.x;
  y = elem->corner.y;

  /* Update connections: */
  umlclass->connections[0].pos = elem->corner;
  umlclass->connections[0].directions = DIR_NORTH|DIR_WEST;

  /* there are four corner points and two side points, thus all
   * remaining points are on the top/bottom width
   */
  pointswide = (UMLCLASS_CONNECTIONPOINTS - 6) / 2;
  pointspacing = elem->width / (pointswide + 1.0);

  /* across the top connection points */
  for (i=1;i<=pointswide;i++) {
    umlclass->connections[i].pos.x = x + (pointspacing * i);
    umlclass->connections[i].pos.y = y;
    umlclass->connections[i].directions = DIR_NORTH;
  }

  i = (UMLCLASS_CONNECTIONPOINTS / 2) - 2;
  umlclass->connections[i].pos.x = x + elem->width;
  umlclass->connections[i].pos.y = y;
  umlclass->connections[i].directions = DIR_NORTH|DIR_EAST;

  i = (UMLCLASS_CONNECTIONPOINTS / 2) - 1;
  umlclass->connections[i].pos.x = x;
  umlclass->connections[i].pos.y = y + umlclass->namebox_height / 2.0;
  umlclass->connections[i].directions = DIR_WEST;

  i = (UMLCLASS_CONNECTIONPOINTS / 2);
  umlclass->connections[i].pos.x = x + elem->width;
  umlclass->connections[i].pos.y = y + umlclass->namebox_height / 2.0;
  umlclass->connections[i].directions = DIR_EAST;

  i = (UMLCLASS_CONNECTIONPOINTS / 2) + 1;
  umlclass->connections[i].pos.x = x;
  umlclass->connections[i].pos.y = y + elem->height;
  umlclass->connections[i].directions = DIR_WEST|DIR_SOUTH;

  /* across the bottom connection points */
  lowerleftcorner = (UMLCLASS_CONNECTIONPOINTS / 2) + 1;
  for (i=1;i<=pointswide;i++) {
    umlclass->connections[lowerleftcorner + i].pos.x = x + (pointspacing * i);
    umlclass->connections[lowerleftcorner + i].pos.y = y + elem->height;
    umlclass->connections[lowerleftcorner + i].directions = DIR_SOUTH;
  }

  /* bottom-right corner */
  i = (UMLCLASS_CONNECTIONPOINTS) - 1;
  umlclass->connections[i].pos.x = x + elem->width;
  umlclass->connections[i].pos.y = y + elem->height;
  umlclass->connections[i].directions = DIR_EAST|DIR_SOUTH;

#ifdef UML_MAINPOINT
  /* Main point -- lives just after fixed connpoints in umlclass array */
  i = UMLCLASS_CONNECTIONPOINTS;
  umlclass->connections[i].pos.x = x + elem->width / 2;
  umlclass->connections[i].pos.y = y + elem->height / 2;
  umlclass->connections[i].directions = DIR_ALL;
  umlclass->connections[i].flags = CP_FLAGS_MAIN;
#endif

  y += umlclass->namebox_height + 0.1 + umlclass->font_height/2;

  list = (!umlclass->visible_attributes || umlclass->suppress_attributes) ? NULL : umlclass->attributes;
  while (list != NULL) {
    UMLAttribute *attr = (UMLAttribute *)list->data;

    attr->left_connection->pos.x = x;
    attr->left_connection->pos.y = y;
    attr->left_connection->directions = DIR_WEST;
    attr->right_connection->pos.x = x + elem->width;
    attr->right_connection->pos.y = y;
    attr->right_connection->directions = DIR_EAST;

    y += umlclass->font_height;
    if (umlclass->visible_comments && attr->comment != NULL && attr->comment[0] != '\0') {
      int NumberOfLines = 0;
      char *CommentString = 0;

      CommentString =
        uml_create_documentation_tag (attr->comment, umlclass->comment_tagging, umlclass->comment_line_length, &NumberOfLines);
      g_clear_pointer (&CommentString, g_free);
      y += umlclass->comment_font_height*NumberOfLines + umlclass->comment_font_height/2;
    }

    list = g_list_next(list);
  }

  y = elem->corner.y + umlclass->namebox_height + 0.1 + umlclass->font_height/2;
  if (umlclass->visible_attributes) {
    y += umlclass->attributesbox_height;
  }

  list = (!umlclass->visible_operations || umlclass->suppress_operations) ? NULL : umlclass->operations;
  while (list != NULL) {
    UMLOperation *op = (UMLOperation *)list->data;

    op->left_connection->pos.x = x;
    op->left_connection->pos.y = y;
    op->left_connection->directions = DIR_WEST;
    op->right_connection->pos.x = x + elem->width;
    op->right_connection->pos.y = y;
    op->right_connection->directions = DIR_EAST;

    if (op->needs_wrapping) { /* Wrapped */
      int lines = g_list_length(op->wrappos);
      y += umlclass->font_height * lines;
    } else {
      y += umlclass->font_height;
    }
    if (umlclass->visible_comments && op->comment != NULL && op->comment[0] != '\0') {
      int NumberOfLines = 0;
      char *CommentString = 0;

      CommentString =
        uml_create_documentation_tag(op->comment, umlclass->comment_tagging, umlclass->comment_line_length, &NumberOfLines);
      g_clear_pointer (&CommentString, g_free);
      y += umlclass->comment_font_height*NumberOfLines + umlclass->comment_font_height/2;
    }
    list = g_list_next(list);
  }

  element_update_boundingbox(elem);

  if (umlclass->template) {
    /* fix boundingumlclass for templates: */
    obj->bounding_box.top -= (umlclass->templates_height  - UMLCLASS_TEMPLATE_OVERLAY_Y) ;
    obj->bounding_box.right += (umlclass->templates_width - UMLCLASS_TEMPLATE_OVERLAY_X);
    obj->bounding_box.left  -= (elem->width < UMLCLASS_TEMPLATE_OVERLAY_X) ?
				(UMLCLASS_TEMPLATE_OVERLAY_X - elem->width) : 0;
  }

  obj->position = elem->corner;

  element_update_handles(elem);

#ifdef DEBUG
  umlclass_sanity_check(umlclass, "After updating data");
#endif
}

/**
 * Calculate the dimensions of the class icons namebox for a given object of UMLClass.
 * The height is stored in the class structure. When calculating the
 * comment, if any, the comment is word wrapped and the resulting number of
 * lines is then used to calculate the height of the bounding box.
 *
 * @param   umlclass  pointer to the object of UMLClass to calculate
 * @return            the horizontal size of the name box.
 *
 */

static real
umlclass_calculate_name_data(UMLClass *umlclass)
{
  real   maxwidth = 0.0;
  real   width = 0.0;
  /* name box: */

  if (umlclass->name != NULL && umlclass->name[0] != '\0') {
    if (umlclass->abstract) {
      maxwidth = dia_font_string_width(umlclass->name,
                                       umlclass->abstract_classname_font,
                                       umlclass->abstract_classname_font_height);
    } else {
      maxwidth = dia_font_string_width(umlclass->name,
                                       umlclass->classname_font,
                                       umlclass->classname_font_height);
    }
  }

  umlclass->namebox_height = umlclass->classname_font_height + 4*0.1;
  g_clear_pointer (&umlclass->stereotype_string, g_free);

  if (umlclass->stereotype != NULL && umlclass->stereotype[0] != '\0') {
    umlclass->namebox_height += umlclass->font_height;
    umlclass->stereotype_string = g_strconcat ( UML_STEREOTYPE_START,
			                                    umlclass->stereotype,
			                                    UML_STEREOTYPE_END,
			                                    NULL);

    width = dia_font_string_width (umlclass->stereotype_string,
                                   umlclass->normal_font,
                                   umlclass->font_height);
    maxwidth = MAX(width, maxwidth);
  } else {
    umlclass->stereotype_string = NULL;
  }

  if (umlclass->visible_comments && umlclass->comment != NULL && umlclass->comment[0] != '\0')
  {
    int NumberOfLines = 0;
    gchar *CommentString = uml_create_documentation_tag (umlclass->comment,
                                                         umlclass->comment_tagging,
                                                         umlclass->comment_line_length,
                                                         &NumberOfLines);
    width = dia_font_string_width (CommentString,
                                   umlclass->comment_font,
                                   umlclass->comment_font_height);

    g_clear_pointer (&CommentString, g_free);
    umlclass->namebox_height += umlclass->comment_font_height * NumberOfLines;
    maxwidth = MAX (width, maxwidth);
  }
  return maxwidth;
}

/**
 * Calculate the dimensions of the attribute box on an object of type UMLClass.
 * @param   umlclass  a pointer to an object of UMLClass
 * @return            the horizontal size of the attribute box
 *
 */

static real
umlclass_calculate_attribute_data(UMLClass *umlclass)
{
  int    i;
  real   maxwidth = 0.0;
  real   width    = 0.0;
  GList *list;

  umlclass->attributesbox_height = 2*0.1;

  if (g_list_length(umlclass->attributes) != 0)
  {
    i = 0;
    list = umlclass->attributes;
    while (list != NULL)
    {
      UMLAttribute *attr   = (UMLAttribute *) list->data;
      gchar        *attstr = uml_attribute_get_string (attr);

      if (attr->abstract)
      {
        width = dia_font_string_width(attstr,
                                      umlclass->abstract_font,
                                      umlclass->abstract_font_height);
        umlclass->attributesbox_height += umlclass->abstract_font_height;
      }
      else
      {
        width = dia_font_string_width(attstr,
                                      umlclass->normal_font,
                                      umlclass->font_height);
        umlclass->attributesbox_height += umlclass->font_height;
      }
      maxwidth = MAX(width, maxwidth);

      if (umlclass->visible_comments && attr->comment != NULL && attr->comment[0] != '\0')
      {
        int NumberOfLines = 0;
        char *CommentString = uml_create_documentation_tag (attr->comment,
                                                            umlclass->comment_tagging,
                                                            umlclass->comment_line_length,
                                                            &NumberOfLines);
        width = dia_font_string_width(CommentString,
                                       umlclass->comment_font,
                                       umlclass->comment_font_height);
        g_clear_pointer (&CommentString, g_free);
        umlclass->attributesbox_height += umlclass->comment_font_height * NumberOfLines + umlclass->comment_font_height/2;
        maxwidth = MAX(width, maxwidth);
      }

      i++;
      list = g_list_next(list);
      g_clear_pointer (&attstr, g_free);
    }
  }

  if ((umlclass->attributesbox_height<0.4)|| umlclass->suppress_attributes )
  {
    umlclass->attributesbox_height = 0.4;
  }
  return maxwidth;
}

/**
 * Calculate the dimensions of the operations box of an object of  UMLClass.
 * The vertical size or height is stored in the object.
 * @param   umlclass  a pointer to an object of UMLClass
 * @return         the horizontial size of the operations box
 *
 */

static real
umlclass_calculate_operation_data(UMLClass *umlclass)
{
  int    i;
  int    pos_brace;
  int    wrap_pos;
  int    last_wrap_pos;
  int    indent;
  int    offset;
  int    maxlinewidth;
  int    length;
  real   maxwidth = 0.0;
  real   width    = 0.0;
  GList *list;
  GList *wrapsublist;

  /* operations box: */
  umlclass->operationsbox_height = 2*0.1;

  if (0 != g_list_length(umlclass->operations))
  {
    i = 0;
    list = umlclass->operations;
    while (list != NULL)
    {
      UMLOperation *op = (UMLOperation *) list->data;
      gchar *opstr = uml_get_operation_string(op);
      DiaFont   *Font;
      real       FontHeight;

      length = strlen( (const gchar*)opstr);

      if (op->wrappos != NULL) {
	g_list_free(op->wrappos);
      }
      op->wrappos = NULL;

      switch(op->inheritance_type)
      {
	  case DIA_UML_ABSTRACT:
	    Font       =  umlclass->abstract_font;
	    FontHeight =  umlclass->abstract_font_height;
	    break;
	  case DIA_UML_POLYMORPHIC:
	    Font       =  umlclass->polymorphic_font;
	    FontHeight =  umlclass->polymorphic_font_height;
	    break;
	  case DIA_UML_LEAF:
	  default:
	    Font       = umlclass->normal_font;
	    FontHeight = umlclass->font_height;
      }
      op->ascent = dia_font_ascent(opstr, Font, FontHeight);

      if( umlclass->wrap_operations )
      {
        if( length > umlclass->wrap_after_char)
        {
          gchar *part_opstr;
	  op->needs_wrapping = TRUE;

          /* count maximal line width to create a secure buffer (part_opstr)
          and build the sublist with the wrapping data for the current operation, which will be used by umlclass_draw(), too.
	  */
          pos_brace = wrap_pos = offset
	    = maxlinewidth = umlclass->max_wrapped_line_width = 0;
          while( wrap_pos + offset < length)
          {
            do
            {
              int pos_next_comma = strcspn( (const gchar*)opstr + wrap_pos + offset, ",");
              wrap_pos += pos_next_comma + 1;
            } while( wrap_pos < umlclass->wrap_after_char - pos_brace
		     && wrap_pos + offset < length);

            if( offset == 0){
              pos_brace = strcspn( opstr, "(");
	      op->wrap_indent = pos_brace + 1;
            }
	    op->wrappos = g_list_append(op->wrappos,
					GINT_TO_POINTER(wrap_pos + offset));

            maxlinewidth = MAX(maxlinewidth, wrap_pos);

            offset += wrap_pos;
            wrap_pos = 0;
          }
          umlclass->max_wrapped_line_width = MAX( umlclass->max_wrapped_line_width, maxlinewidth+1);

	  indent = op->wrap_indent;
          part_opstr = g_alloca(umlclass->max_wrapped_line_width+indent+1);

	  wrapsublist = op->wrappos;
          last_wrap_pos = 0;

          while( wrapsublist != NULL){
            wrap_pos = GPOINTER_TO_INT( wrapsublist->data);
            if( last_wrap_pos == 0){
              strncpy( part_opstr, opstr, wrap_pos);
              memset( part_opstr+wrap_pos, '\0', 1);
            }
            else
            {
              memset( part_opstr, ' ', indent);
              memset( part_opstr+indent, '\0', 1);
              strncat( part_opstr, opstr+last_wrap_pos, wrap_pos-last_wrap_pos);
            }

            width = dia_font_string_width(part_opstr,Font,FontHeight);
            umlclass->operationsbox_height += FontHeight;

            maxwidth = MAX(width, maxwidth);
            last_wrap_pos = wrap_pos;
            wrapsublist = g_list_next( wrapsublist);
          }
        }
        else
        {
	  op->needs_wrapping = FALSE;
        }
      }

      if (!(umlclass->wrap_operations && length > umlclass->wrap_after_char)) {
        switch(op->inheritance_type)
        {
        case DIA_UML_ABSTRACT:
          Font       =  umlclass->abstract_font;
          FontHeight =  umlclass->abstract_font_height;
          break;
        case DIA_UML_POLYMORPHIC:
          Font       =  umlclass->polymorphic_font;
          FontHeight =  umlclass->polymorphic_font_height;
          break;
        case DIA_UML_LEAF:
        default:
          Font       = umlclass->normal_font;
          FontHeight = umlclass->font_height;
        }
        width = dia_font_string_width(opstr,Font,FontHeight);
        umlclass->operationsbox_height += FontHeight;

        maxwidth = MAX(width, maxwidth);
      }

      if (umlclass->visible_comments && op->comment != NULL && op->comment[0] != '\0'){
        int NumberOfLines = 0;
        char *CommentString = uml_create_documentation_tag (op->comment,
                                                            umlclass->comment_tagging,
                                                            umlclass->comment_line_length,
                                                            &NumberOfLines);
        width = dia_font_string_width(CommentString,
                                       umlclass->comment_font,
                                       umlclass->comment_font_height);
        g_clear_pointer (&CommentString, g_free);
        umlclass->operationsbox_height += umlclass->comment_font_height * NumberOfLines + umlclass->comment_font_height/2;
        maxwidth = MAX(width, maxwidth);
      }

      i++;
      list = g_list_next(list);
      g_clear_pointer (&opstr, g_free);
    }
  }

  if (!umlclass->allow_resizing)
    umlclass->element.width = maxwidth + 2*0.3;
  else {
    umlclass->min_width = maxwidth + 2*0.3;
    umlclass->element.width = MAX(umlclass->element.width, umlclass->min_width);
  }

  if ((umlclass->operationsbox_height<0.4) || umlclass->suppress_operations ) {
    umlclass->operationsbox_height = 0.4;
  }

  return maxwidth;
}

/**
 * calculate the size of the class icon for an object of UMLClass.
 * This is done by calculating the size of the text to be displayed within
 * each of the contained bounding boxes, name, attributes and operations.
 * Because the comments may require wrapping, each comment is wrapped and
 * the resulting number of lines is used to calculate the size of the
 * comment within the box. The various font settings with in the class
 * properties contribute to the overall size of the resulting bounding box.
 *
 * @param   umlclass  a pointer to an object of UMLClass
 *
 * @return  the horizontial size of the box
 *
 */
void
umlclass_calculate_data(UMLClass *umlclass)
{
  int    i;
  int    num_templates;
  real   maxwidth = 0.0;
  real   width;
  GList *list;

  if (!umlclass->destroyed)
  {
    maxwidth = MAX(umlclass_calculate_name_data(umlclass),      maxwidth);

    umlclass->element.height = umlclass->namebox_height;

    if (umlclass->visible_attributes){
      maxwidth = MAX(umlclass_calculate_attribute_data(umlclass), maxwidth);
      umlclass->element.height += umlclass->attributesbox_height;
    }
    if (umlclass->visible_operations){
      maxwidth = MAX(umlclass_calculate_operation_data(umlclass), maxwidth);
      umlclass->element.height += umlclass->operationsbox_height;
    }
    if (!umlclass->allow_resizing)
      umlclass->element.width  = maxwidth+0.5;
    else {
      umlclass->min_width = maxwidth+0.5;
      umlclass->element.width = MAX(umlclass->element.width, umlclass->min_width);
    }
    /* templates box: */
    num_templates = g_list_length(umlclass->formal_params);

    umlclass->templates_height =
      umlclass->font_height * num_templates + 2*0.1;
    umlclass->templates_height = MAX(umlclass->templates_height, 0.4);


    maxwidth = UMLCLASS_TEMPLATE_OVERLAY_X;
    if (num_templates != 0)
    {
      i = 0;
      list = umlclass->formal_params;
      while (list != NULL)
      {
        UMLFormalParameter *param = (UMLFormalParameter *) list->data;
        gchar *paramstr = uml_formal_parameter_get_string (param);

        width = dia_font_string_width(paramstr,
                                      umlclass->normal_font,
                                      umlclass->font_height);
        maxwidth = MAX(width, maxwidth);

        i++;
        list = g_list_next(list);
        g_clear_pointer (&paramstr, g_free);
      }
    }
    umlclass->templates_width = maxwidth + 2*0.2;
  }
}

static void
fill_in_fontdata(UMLClass *umlclass)
{
   if (umlclass->normal_font == NULL) {
     umlclass->font_height = 0.8;
     umlclass->normal_font = dia_font_new_from_style(DIA_FONT_MONOSPACE, 0.8);
   }
   if (umlclass->abstract_font == NULL) {
     umlclass->abstract_font_height = 0.8;
     umlclass->abstract_font =
       dia_font_new_from_style(DIA_FONT_MONOSPACE | DIA_FONT_ITALIC | DIA_FONT_BOLD, 0.8);
   }
   if (umlclass->polymorphic_font == NULL) {
     umlclass->polymorphic_font_height = 0.8;
     umlclass->polymorphic_font =
       dia_font_new_from_style(DIA_FONT_MONOSPACE | DIA_FONT_ITALIC, 0.8);
   }
   if (umlclass->classname_font == NULL) {
     umlclass->classname_font_height = 1.0;
     umlclass->classname_font =
       dia_font_new_from_style(DIA_FONT_SANS | DIA_FONT_BOLD, 1.0);
   }
   if (umlclass->abstract_classname_font == NULL) {
     umlclass->abstract_classname_font_height = 1.0;
     umlclass->abstract_classname_font =
       dia_font_new_from_style(DIA_FONT_SANS | DIA_FONT_BOLD | DIA_FONT_ITALIC, 1.0);
   }
   if (umlclass->comment_font == NULL) {
     umlclass->comment_font_height = 0.7;
     umlclass->comment_font = dia_font_new_from_style(DIA_FONT_SANS | DIA_FONT_ITALIC, 0.7);
   }
}
/**
 * Create an object of type class
 * By default this will create a object of class UMLClass. Howerver there
 * are at least two types of UMLClass objects, so the user_data is selects
 * the correct UMLClass object. Other than that this is quite straight
 * forward. The key to the polymorphic nature of this object is the use of
 * the DiaObjectType record which in conjunction with the user_data
 * controls the specific derived object type.
 *
 * @param  startpoint   the origin of the object being created
 * @param  user_data	Information used by this routine to create the appropriate object
 * @param  handle1		ignored when creating a class object
 * @param  handle2      ignored when creating a class object
 * @return               a pointer to the object created
 *
 *  NOTE:
 *      This function should most likely be move to a source file for
 *      handling global UML functionallity at some point.
 */

static DiaObject *
umlclass_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  UMLClass *umlclass;
  Element *elem;
  DiaObject *obj;
  int i;

  umlclass = g_new0 (UMLClass, 1);
  elem = &umlclass->element;
  obj = &elem->object;


  elem->corner = *startpoint;

#ifdef UML_MAINPOINT
  element_init(elem, 8, UMLCLASS_CONNECTIONPOINTS + 1); /* No attribs or ops => 0 extra connectionpoints. */
#else
  element_init(elem, 8, UMLCLASS_CONNECTIONPOINTS); /* No attribs or ops => 0 extra connectionpoints. */
#endif

  umlclass->properties_dialog = NULL;
  fill_in_fontdata(umlclass);


  /*
   * The following block of code may need to be converted to a switch statement if more than
   * two types of objects can be made - Dave Klotzbach
   */
  umlclass->template = (GPOINTER_TO_INT(user_data)==1);

  if (umlclass->template){
    umlclass->name = g_strdup (_("Template"));
  }
  else {
    umlclass->name = g_strdup (_("Class"));
  }
  obj->type = &umlclass_type;
  obj->ops = &umlclass_ops;

  umlclass->stereotype = NULL;
  umlclass->comment = NULL;

  umlclass->abstract = FALSE;

  umlclass->suppress_attributes = FALSE;
  umlclass->suppress_operations = FALSE;

  umlclass->visible_attributes = TRUE;
  umlclass->visible_operations = TRUE;
  umlclass->visible_comments = FALSE;

  umlclass->wrap_operations = TRUE;
  umlclass->wrap_after_char = UMLCLASS_WRAP_AFTER_CHAR;

  umlclass->attributes = NULL;

  umlclass->operations = NULL;

  umlclass->formal_params = NULL;

  umlclass->stereotype_string = NULL;

  umlclass->line_width = attributes_get_default_linewidth();
  umlclass->text_color = color_black;
  umlclass->line_color = attributes_get_foreground();
  umlclass->fill_color = attributes_get_background();

  umlclass_calculate_data(umlclass);

  for (i=0;i<UMLCLASS_CONNECTIONPOINTS;i++) {
    obj->connections[i] = &umlclass->connections[i];
    umlclass->connections[i].object = obj;
    umlclass->connections[i].connected = NULL;
  }
#ifdef UML_MAINPOINT
  /* Put mainpoint at the end, after conditional attr/oprn points,
   * but store it in the local connectionpoint array. */
  i += umlclass_num_dynamic_connectionpoints(umlclass);
  obj->connections[i] = &umlclass->connections[UMLCLASS_CONNECTIONPOINTS];
  umlclass->connections[UMLCLASS_CONNECTIONPOINTS].object = obj;
  umlclass->connections[UMLCLASS_CONNECTIONPOINTS].connected = NULL;
#endif

  elem->extra_spacing.border_trans = umlclass->line_width/2.0;
  umlclass_update_data(umlclass);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;
  return &umlclass->element.object;
}

static void
umlclass_destroy(UMLClass *umlclass)
{
  GList *list;
  UMLAttribute *attr;
  UMLOperation *op;

#ifdef DEBUG
  umlclass_sanity_check(umlclass, "Destroying");
#endif

  umlclass->destroyed = TRUE;

  g_clear_object (&umlclass->normal_font);
  g_clear_object (&umlclass->abstract_font);
  g_clear_object (&umlclass->polymorphic_font);
  g_clear_object (&umlclass->classname_font);
  g_clear_object (&umlclass->abstract_classname_font);
  g_clear_object (&umlclass->comment_font);

  element_destroy (&umlclass->element);

  g_clear_pointer (&umlclass->name, g_free);
  g_clear_pointer (&umlclass->stereotype, g_free);
  g_clear_pointer (&umlclass->comment, g_free);

  list = umlclass->attributes;
  while (list != NULL) {
    attr = (UMLAttribute *)list->data;
    g_clear_pointer (&attr->left_connection, g_free);
    g_clear_pointer (&attr->right_connection, g_free);
    uml_attribute_unref (attr);
    list = g_list_next (list);
  }
  g_list_free(umlclass->attributes);

  list = umlclass->operations;
  while (list != NULL) {
    op = (UMLOperation *)list->data;
    g_clear_pointer (&op->left_connection, g_free);
    g_clear_pointer (&op->right_connection, g_free);
    uml_operation_unref (op);
    list = g_list_next (list);
  }
  g_list_free (umlclass->operations);

  g_list_free_full (umlclass->formal_params, (GDestroyNotify) uml_formal_parameter_unref);
  umlclass->formal_params = NULL;

  g_clear_pointer (&umlclass->stereotype_string, g_free);

  if (umlclass->properties_dialog != NULL) {
    umlclass_dialog_free (umlclass->properties_dialog);
  }
}

static DiaObject *
umlclass_copy(UMLClass *umlclass)
{
  int i;
  UMLClass *newumlclass;
  Element *elem, *newelem;
  DiaObject *newobj;
  GList *list;
  UMLFormalParameter *param;

  elem = &umlclass->element;

  newumlclass = g_new0 (UMLClass, 1);
  newelem = &newumlclass->element;
  newobj = &newelem->object;

  element_copy(elem, newelem);

  newumlclass->font_height = umlclass->font_height;
  newumlclass->abstract_font_height = umlclass->abstract_font_height;
  newumlclass->polymorphic_font_height = umlclass->polymorphic_font_height;
  newumlclass->classname_font_height = umlclass->classname_font_height;
  newumlclass->abstract_classname_font_height =
          umlclass->abstract_classname_font_height;
  newumlclass->comment_font_height =
          umlclass->comment_font_height;

  newumlclass->normal_font =
          dia_font_copy(umlclass->normal_font);
  newumlclass->abstract_font =
          dia_font_copy(umlclass->abstract_font);
  newumlclass->polymorphic_font =
          dia_font_copy(umlclass->polymorphic_font);
  newumlclass->classname_font =
          dia_font_copy(umlclass->classname_font);
  newumlclass->abstract_classname_font =
          dia_font_copy(umlclass->abstract_classname_font);
  newumlclass->comment_font =
          dia_font_copy(umlclass->comment_font);

  newumlclass->name = g_strdup(umlclass->name);
  if (umlclass->stereotype != NULL && umlclass->stereotype[0] != '\0')
    newumlclass->stereotype = g_strdup(umlclass->stereotype);
  else
    newumlclass->stereotype = NULL;

  if (umlclass->comment != NULL)
    newumlclass->comment = g_strdup(umlclass->comment);
  else
    newumlclass->comment = NULL;

  newumlclass->abstract = umlclass->abstract;
  newumlclass->suppress_attributes = umlclass->suppress_attributes;
  newumlclass->suppress_operations = umlclass->suppress_operations;
  newumlclass->visible_attributes = umlclass->visible_attributes;
  newumlclass->visible_operations = umlclass->visible_operations;
  newumlclass->visible_comments = umlclass->visible_comments;
  newumlclass->wrap_operations = umlclass->wrap_operations;
  newumlclass->wrap_after_char = umlclass->wrap_after_char;
  newumlclass->comment_line_length = umlclass->comment_line_length;
  newumlclass->comment_tagging = umlclass->comment_tagging;
  newumlclass->allow_resizing = umlclass->allow_resizing;
  newumlclass->line_width = umlclass->line_width;
  newumlclass->text_color = umlclass->text_color;
  newumlclass->line_color = umlclass->line_color;
  newumlclass->fill_color = umlclass->fill_color;

  newumlclass->attributes = NULL;
  list = umlclass->attributes;
  while (list != NULL) {
    UMLAttribute *attr = (UMLAttribute *)list->data;
    /* not copying the connection, if there was one */
    UMLAttribute *newattr = uml_attribute_copy(attr);
    uml_attribute_ensure_connection_points (newattr, newobj);

    newumlclass->attributes = g_list_append(newumlclass->attributes,
					    newattr);
    list = g_list_next(list);
  }

  newumlclass->operations = NULL;
  list = umlclass->operations;
  while (list != NULL) {
    UMLOperation *op = (UMLOperation *)list->data;
    UMLOperation *newop = uml_operation_copy(op);
    uml_operation_ensure_connection_points (newop, newobj);

    newumlclass->operations = g_list_append(newumlclass->operations,
					     newop);
    list = g_list_next(list);
  }

  newumlclass->template = umlclass->template;

  newumlclass->formal_params = NULL;
  list = umlclass->formal_params;
  while (list != NULL) {
    param = (UMLFormalParameter *)list->data;
    newumlclass->formal_params =
      g_list_append (newumlclass->formal_params,
                     uml_formal_parameter_copy (param));
    list = g_list_next (list);
  }

  newumlclass->properties_dialog = NULL;

  newumlclass->stereotype_string = NULL;

  for (i=0;i<UMLCLASS_CONNECTIONPOINTS;i++) {
    newobj->connections[i] = &newumlclass->connections[i];
    newumlclass->connections[i].object = newobj;
    newumlclass->connections[i].connected = NULL;
    newumlclass->connections[i].pos = umlclass->connections[i].pos;
  }

  umlclass_calculate_data(newumlclass);

  i = UMLCLASS_CONNECTIONPOINTS;
  if ( (newumlclass->visible_attributes) &&
       (!newumlclass->suppress_attributes)) {
    list = newumlclass->attributes;
    while (list != NULL) {
      UMLAttribute *attr = (UMLAttribute *)list->data;
      newobj->connections[i++] = attr->left_connection;
      newobj->connections[i++] = attr->right_connection;

      list = g_list_next(list);
    }
  }

  if ( (newumlclass->visible_operations) &&
       (!newumlclass->suppress_operations)) {
    list = newumlclass->operations;
    while (list != NULL) {
      UMLOperation *op = (UMLOperation *)list->data;
      newobj->connections[i++] = op->left_connection;
      newobj->connections[i++] = op->right_connection;

      list = g_list_next(list);
    }
  }

#ifdef UML_MAINPOINT
  newobj->connections[i] = &newumlclass->connections[UMLCLASS_CONNECTIONPOINTS];
  newumlclass->connections[UMLCLASS_CONNECTIONPOINTS].object = newobj;
  newumlclass->connections[UMLCLASS_CONNECTIONPOINTS].connected = NULL;
  newumlclass->connections[UMLCLASS_CONNECTIONPOINTS].pos =
    umlclass->connections[UMLCLASS_CONNECTIONPOINTS].pos;
  newumlclass->connections[UMLCLASS_CONNECTIONPOINTS].flags =
    umlclass->connections[UMLCLASS_CONNECTIONPOINTS].flags;
#endif

  umlclass_update_data(newumlclass);

#ifdef DEBUG
  umlclass_sanity_check(newumlclass, "Copied");
#endif

  return &newumlclass->element.object;
}


static void
umlclass_save(UMLClass *umlclass, ObjectNode obj_node,
	      DiaContext *ctx)
{
  UMLAttribute *attr;
  UMLOperation *op;
  UMLFormalParameter *formal_param;
  GList *list;
  AttributeNode attr_node;

#ifdef DEBUG
  umlclass_sanity_check(umlclass, "Saving");
#endif

  element_save(&umlclass->element, obj_node, ctx);

  /* Class info: */
  data_add_string(new_attribute(obj_node, "name"),
		  umlclass->name, ctx);
  data_add_string(new_attribute(obj_node, "stereotype"),
		  umlclass->stereotype, ctx);
  data_add_string(new_attribute(obj_node, "comment"),
                  umlclass->comment, ctx);
  data_add_boolean(new_attribute(obj_node, "abstract"),
		   umlclass->abstract, ctx);
  data_add_boolean(new_attribute(obj_node, "suppress_attributes"),
		   umlclass->suppress_attributes, ctx);
  data_add_boolean(new_attribute(obj_node, "suppress_operations"),
		   umlclass->suppress_operations, ctx);
  data_add_boolean(new_attribute(obj_node, "visible_attributes"),
		   umlclass->visible_attributes, ctx);
  data_add_boolean(new_attribute(obj_node, "visible_operations"),
		   umlclass->visible_operations, ctx);
  data_add_boolean(new_attribute(obj_node, "visible_comments"),
		   umlclass->visible_comments, ctx);
  data_add_boolean(new_attribute(obj_node, "wrap_operations"),
		   umlclass->wrap_operations, ctx);
  data_add_int(new_attribute(obj_node, "wrap_after_char"),
		   umlclass->wrap_after_char, ctx);
  data_add_int(new_attribute(obj_node, "comment_line_length"),
                   umlclass->comment_line_length, ctx);
  data_add_boolean(new_attribute(obj_node, "comment_tagging"),
                   umlclass->comment_tagging, ctx);
  data_add_boolean(new_attribute(obj_node, "allow_resizing"),
                   umlclass->allow_resizing, ctx);
  data_add_real(new_attribute(obj_node, PROP_STDNAME_LINE_WIDTH),
		   umlclass->line_width, ctx);
  data_add_color(new_attribute(obj_node, "line_color"),
		   &umlclass->line_color, ctx);
  data_add_color(new_attribute(obj_node, "fill_color"),
		   &umlclass->fill_color, ctx);
  data_add_color(new_attribute(obj_node, "text_color"),
		   &umlclass->text_color, ctx);
  data_add_font (new_attribute (obj_node, "normal_font"),
                 umlclass->normal_font, ctx);
  data_add_font (new_attribute (obj_node, "abstract_font"),
                 umlclass->abstract_font, ctx);
  data_add_font (new_attribute (obj_node, "polymorphic_font"),
                 umlclass->polymorphic_font, ctx);
  data_add_font (new_attribute (obj_node, "classname_font"),
                 umlclass->classname_font, ctx);
  data_add_font (new_attribute (obj_node, "abstract_classname_font"),
                 umlclass->abstract_classname_font, ctx);
  data_add_font (new_attribute (obj_node, "comment_font"),
                 umlclass->comment_font, ctx);
  data_add_real (new_attribute (obj_node, "normal_font_height"),
                 umlclass->font_height, ctx);
  data_add_real (new_attribute (obj_node, "polymorphic_font_height"),
                 umlclass->polymorphic_font_height, ctx);
  data_add_real (new_attribute (obj_node, "abstract_font_height"),
                 umlclass->abstract_font_height, ctx);
  data_add_real (new_attribute (obj_node, "classname_font_height"),
                 umlclass->classname_font_height, ctx);
  data_add_real (new_attribute (obj_node, "abstract_classname_font_height"),
                 umlclass->abstract_classname_font_height, ctx);
  data_add_real (new_attribute (obj_node, "comment_font_height"),
                 umlclass->comment_font_height, ctx);

  /* Attribute info: */
  attr_node = new_attribute (obj_node, "attributes");
  list = umlclass->attributes;
  while (list != NULL) {
    attr = (UMLAttribute *) list->data;

    uml_attribute_write (attr_node, attr, ctx);

    list = g_list_next (list);
  }

  /* Operations info: */
  attr_node = new_attribute (obj_node, "operations");
  list = umlclass->operations;
  while (list != NULL) {
    op = (UMLOperation *) list->data;

    uml_operation_write (attr_node, op, ctx);

    list = g_list_next (list);
  }

  /* Template info: */
  data_add_boolean (new_attribute (obj_node, "template"),
                    umlclass->template, ctx);

  attr_node = new_attribute (obj_node, "templates");
  list = umlclass->formal_params;
  while (list != NULL) {
    formal_param = (UMLFormalParameter *) list->data;

    uml_formal_parameter_write (attr_node, formal_param, ctx);

    list = g_list_next (list);
  }
}


static DiaObject *
umlclass_load (ObjectNode obj_node, int version, DiaContext *ctx)
{
  UMLClass *umlclass;
  Element *elem;
  DiaObject *obj;
  AttributeNode attr_node;
  int i;
  GList *list;


  umlclass = g_new0 (UMLClass, 1);
  elem = &umlclass->element;
  obj = &elem->object;

  obj->type = &umlclass_type;
  obj->ops = &umlclass_ops;

  element_load(elem, obj_node, ctx);

#ifdef UML_MAINPOINT
  element_init(elem, 8, UMLCLASS_CONNECTIONPOINTS + 1);
#else
  element_init(elem, 8, UMLCLASS_CONNECTIONPOINTS);
#endif

  umlclass->properties_dialog = NULL;

  for (i=0;i<UMLCLASS_CONNECTIONPOINTS;i++) {
    obj->connections[i] = &umlclass->connections[i];
    umlclass->connections[i].object = obj;
    umlclass->connections[i].connected = NULL;
  }

  fill_in_fontdata(umlclass);

  /* kind of dirty, object_load_props() may leave us in an inconsistent state --hb */
  object_load_props(obj,obj_node, ctx);

  /* parameters loaded via StdProp dont belong here anymore. In case of strings they
   * will produce leaks. Otherwise they are just wasting time (at runtime and while
   * reading the code). Except maybe for some compatibility stuff.
   * Although that *could* probably done via StdProp too.                      --hb
   */

  /* new since 0.94, don't wrap by default to keep old diagrams intact */
  umlclass->wrap_operations = FALSE;
  attr_node = object_find_attribute(obj_node, "wrap_operations");
  if (attr_node != NULL)
    umlclass->wrap_operations = data_boolean(attribute_first_data(attr_node), ctx);

  umlclass->wrap_after_char = UMLCLASS_WRAP_AFTER_CHAR;
  attr_node = object_find_attribute(obj_node, "wrap_after_char");
  if (attr_node != NULL)
    umlclass->wrap_after_char = data_int(attribute_first_data(attr_node), ctx);

  /* if it uses the new name the value is already set by object_load_props() above */
  umlclass->comment_line_length = UMLCLASS_COMMENT_LINE_LENGTH;
  attr_node = object_find_attribute(obj_node,"comment_line_length");
  /* support the unusal cased name, although it only existed in cvs version */
  if (attr_node == NULL)
    attr_node = object_find_attribute(obj_node,"Comment_line_length");
  if (attr_node != NULL)
    umlclass->comment_line_length = data_int(attribute_first_data(attr_node), ctx);

  /* compatibility with 0.94 and before as well as the temporary state with only 'comment_line_length' */
  umlclass->comment_tagging = (attr_node != NULL);
  attr_node = object_find_attribute(obj_node, "comment_tagging");
  if (attr_node != NULL)
    umlclass->comment_tagging = data_boolean(attribute_first_data(attr_node), ctx);

  /* Loads the line width */
  umlclass->line_width = UMLCLASS_BORDER;
  attr_node = object_find_attribute(obj_node, PROP_STDNAME_LINE_WIDTH);
  if(attr_node != NULL)
    umlclass->line_width = data_real(attribute_first_data(attr_node), ctx);

  umlclass->line_color = color_black;
  /* support the old name ... */
  attr_node = object_find_attribute(obj_node, "foreground_color");
  if(attr_node != NULL)
    data_color(attribute_first_data(attr_node), &umlclass->line_color, ctx);
  umlclass->text_color = umlclass->line_color;
  /* ... but prefer the new one */
  attr_node = object_find_attribute(obj_node, "line_color");
  if(attr_node != NULL)
    data_color(attribute_first_data(attr_node), &umlclass->line_color, ctx);
  attr_node = object_find_attribute(obj_node, "text_color");
  if(attr_node != NULL)
    data_color(attribute_first_data(attr_node), &umlclass->text_color, ctx);

  umlclass->fill_color = color_white;
  /* support the old name ... */
  attr_node = object_find_attribute(obj_node, "background_color");
  if(attr_node != NULL)
    data_color(attribute_first_data(attr_node), &umlclass->fill_color, ctx);
  /* ... but prefer the new one */
  attr_node = object_find_attribute(obj_node, "fill_color");
  if(attr_node != NULL)
    data_color(attribute_first_data(attr_node), &umlclass->fill_color, ctx);

  /* Attribute info: */
  list = umlclass->attributes;
  while (list) {
    UMLAttribute *attr = list->data;
    g_assert(attr);

    uml_attribute_ensure_connection_points (attr, obj);
    list = g_list_next(list);
  }

  /* Operations info: */
  list = umlclass->operations;
  while (list) {
    UMLOperation *op = (UMLOperation *)list->data;
    g_assert(op);

    uml_operation_ensure_connection_points (op, obj);
    list = g_list_next(list);
  }

  /* Template info: */
  umlclass->template = FALSE;
  attr_node = object_find_attribute(obj_node, "template");
  if (attr_node != NULL)
    umlclass->template = data_boolean(attribute_first_data(attr_node), ctx);

  fill_in_fontdata(umlclass);

  umlclass->stereotype_string = NULL;

  umlclass_calculate_data(umlclass);

  elem->extra_spacing.border_trans = umlclass->line_width/2.0;
  umlclass_update_data(umlclass);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }
  umlclass_reflect_resizing(umlclass);

#ifdef DEBUG
  umlclass_sanity_check(umlclass, "Loaded class");
#endif

  return &umlclass->element.object;
}

/** Returns the number of connection points used by the attributes and
 * connections in the current state of the object.
 */
static int
umlclass_num_dynamic_connectionpoints(UMLClass *umlclass) {
  int num = 0;
  if ( (umlclass->visible_attributes) &&
       (!umlclass->suppress_attributes)) {
    GList *list = umlclass->attributes;
    num += 2 * g_list_length(list);
  }

  if ( (umlclass->visible_operations) &&
       (!umlclass->suppress_operations)) {
    GList *list = umlclass->operations;
    num += 2 * g_list_length(list);
  }
  return num;
}

void
umlclass_sanity_check(UMLClass *c, gchar *msg)
{
#ifdef UML_MAINPOINT
  int num_fixed_connections = UMLCLASS_CONNECTIONPOINTS + 1;
#else
  int num_fixed_connections = UMLCLASS_CONNECTIONPOINTS;
#endif
  DiaObject *obj = (DiaObject*)c;
  GList *attrs;
  int i;

  dia_object_sanity_check((DiaObject *)c, msg);

  /* Check that num_connections is correct */
  dia_assert_true(num_fixed_connections + umlclass_num_dynamic_connectionpoints(c)
		  == obj->num_connections,
		  "%s: Class %p has %d connections, but %d fixed and %d dynamic\n",
		  msg, c, obj->num_connections, num_fixed_connections,
		  umlclass_num_dynamic_connectionpoints(c));

  for (i = 0; i < UMLCLASS_CONNECTIONPOINTS; i++) {
    dia_assert_true(&c->connections[i] == obj->connections[i],
		    "%s: Class %p connection mismatch at %d: %p != %p\n",
		    msg, c, i, &c->connections[i], obj->connections[i]);
  }

#ifdef UML_MAINPOINT
  dia_assert_true(&c->connections[i] ==
		  obj->connections[i + umlclass_num_dynamic_connectionpoints(c)],
		  "%s: Class %p mainpoint mismatch: %p != %p (at %d)\n",
		  msg, c, &c->connections[i],
		  obj->connections[i + umlclass_num_dynamic_connectionpoints(c)],
		  i + umlclass_num_dynamic_connectionpoints(c));
#endif

  /* Check that attributes are set up right. */
  i = 0;
  for (attrs = c->attributes; attrs != NULL; attrs = g_list_next(attrs)) {
    UMLAttribute *attr = (UMLAttribute *)attrs->data;

    dia_assert_true(attr->name != NULL,
		    "%s: %p attr %d has null name\n",
		    msg, c, i);
    dia_assert_true(attr->type != NULL,
		    "%s: %p attr %d has null type\n",
		    msg, c, i);
#if 0 /* attr->comment == NULL is fine everywhere else */
    dia_assert_true(attr->comment != NULL,
		    "%s: %p attr %d has null comment\n",
		    msg, c, i);
#endif

    /* the following checks are only right with visible attributes */
    if (c->visible_attributes && !c->suppress_attributes) {
      int conn_offset = UMLCLASS_CONNECTIONPOINTS + 2 * i;

      dia_assert_true(attr->left_connection != NULL,
		      "%s: %p attr %d has null left connection\n",
		      msg, c, i);
      dia_assert_true(attr->right_connection != NULL,
		      "%s: %p attr %d has null right connection\n",
		      msg, c, i);

      dia_assert_true(attr->left_connection == obj->connections[conn_offset],
		      "%s: %p attr %d left conn %p doesn't match obj conn %d: %p\n",
		      msg, c, i, attr->left_connection,
		      conn_offset, obj->connections[conn_offset]);
      dia_assert_true(attr->right_connection == obj->connections[conn_offset + 1],
		      "%s: %p attr %d right conn %p doesn't match obj conn %d: %p\n",
		      msg, c, i, attr->right_connection,
		      conn_offset + 1, obj->connections[conn_offset + 1]);
      i++;
    }
  }
  /* Check that operations are set up right. */
}


