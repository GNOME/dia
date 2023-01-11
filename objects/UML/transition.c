/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
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

#include <string.h>

#include "orth_conn.h"
#include "diarenderer.h"
#include "attributes.h"
#include "properties.h"

#include "pixmaps/transition.xpm"

#include "uml.h"

typedef struct _Transition Transition;

struct _Transition {
  OrthConn orth;

  Color text_color;
  Color line_color;

  Handle trigger_text_handle;
  Point trigger_text_pos;
  char *trigger_text;
  char *action_text;
  Handle guard_text_handle;
  Point guard_text_pos;
  char *guard_text;
  gboolean direction_inverted;
};


#define TRANSITION_WIDTH 0.1
#define TRANSITION_FONTHEIGHT 0.8
#define TRANSITION_ARROWLEN 0.5
#define TRANSITION_ARROWWIDTH 0.5
#define HANDLE_MOVE_TRIGGER_TEXT (HANDLE_CUSTOM2)
#define HANDLE_MOVE_GUARD_TEXT (HANDLE_CUSTOM3)
#define TEXT_HANDLE_DISTANCE_FROM_STARTPOINT 0.5

static DiaFont *transition_font = NULL;

static DiaObject *transition_create(Point *startpoint,
                                    void *user_data,
                                    Handle **handle1,
                                    Handle **handle2);
static DiaObject *transition_load(ObjectNode obj_node, int version,DiaContext *ctx);

static void transition_destroy(Transition* transition);
static void transition_draw(Transition* transition, DiaRenderer* ddisp);
static real transition_distance(Transition* transition, Point* point);
static DiaMenu* transition_get_object_menu(Transition* transition,
                                           Point* clickedpoint);
static void transition_select(Transition* obj,
                              Point* clicked_point,
                              DiaRenderer* interactive_renderer);
static DiaObjectChange *transition_move           (Transition       *transition,
                                                   Point            *pos);
static DiaObjectChange *transition_move_handle    (Transition       *transition,
                                                   Handle           *handle,
                                                   Point            *pos,
                                                   ConnectionPoint  *cp,
                                                   HandleMoveReason  reason,
                                                   ModifierKeys      modifiers);
static void transition_set_props(Transition *transition, GPtrArray *props);
static void transition_get_props(Transition *transition, GPtrArray *props);
static PropDescription *transition_describe_props(Transition *transition);
static void uml_transition_update_data(Transition *transition);
static DiaObjectChange* transition_add_segment_cb (DiaObject        *obj,
                                                   Point            *clicked_point,
                                                   gpointer          data);
static DiaObjectChange* transition_del_segment_cb (DiaObject        *obj,
                                                   Point            *clicked_point,
                                                   gpointer          data);

static ObjectTypeOps uml_transition_type_ops = {
  (CreateFunc)transition_create,
  (LoadFunc)transition_load,
  (SaveFunc)object_save_using_properties,
  (GetDefaultsFunc)NULL,
  (ApplyDefaultsFunc)NULL
};

/* The "uml_" prefix is used to avoid clashing with the
   GRAFCET transition */
DiaObjectType uml_transition_type = {
  "UML - Transition",       /* Name */
  /* Version 0 had no autorouting and so shouldn't have it set by default. */
  /* version 0 and 1 expects the arrow to be drawn on the wrong end */
  2,                      /* version */
  transition_xpm, /* Pixmap */
  &uml_transition_type_ops
};


/* These methods are described in the object.h file */
static ObjectOps uml_transition_ops = {
  (DestroyFunc)         transition_destroy,
  (DrawFunc)            transition_draw,
  (DistanceFunc)        transition_distance,
  (SelectFunc)          transition_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            transition_move,
  (MoveHandleFunc)      transition_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      transition_get_object_menu,
  (DescribePropsFunc)   transition_describe_props,
  (GetPropsFunc)        transition_get_props,
  (SetPropsFunc)        transition_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription transition_props[] = {
  ORTHCONN_COMMON_PROPERTIES,
  /* can't use PROP_STD_TEXT_COLOUR_OPTIONAL cause it has PROP_FLAG_DONT_SAVE. It is designed to fill the Text object - not some subset */
  PROP_STD_TEXT_COLOUR_OPTIONS(PROP_FLAG_VISIBLE|PROP_FLAG_STANDARD|PROP_FLAG_OPTIONAL),
  PROP_STD_LINE_COLOUR_OPTIONAL,
  { "trigger", PROP_TYPE_STRING, PROP_FLAG_VISIBLE, N_("Trigger"),
    N_("The event that causes this transition to be taken"), NULL },
  { "action", PROP_TYPE_STRING, PROP_FLAG_VISIBLE, N_("Action"),
    N_("Action to perform when this transition is taken"), NULL },
  { "guard", PROP_TYPE_STRING, PROP_FLAG_VISIBLE, N_("Guard"),
    N_("Condition for taking this transition when the event is fired"), NULL },
  { "trigger_text_pos", PROP_TYPE_POINT, 0, "trigger_text_pos:", NULL, NULL },
  { "guard_text_pos", PROP_TYPE_POINT, 0, "guard_text_pos:", NULL, NULL },
  { "direction_inverted", PROP_TYPE_BOOL, PROP_FLAG_OPTIONAL, "direction_inverted:", NULL, NULL },
  PROP_DESC_END
};


static PropOffset transition_offsets[] = {
  ORTHCONN_COMMON_PROPERTIES_OFFSETS,
  { "text_colour",PROP_TYPE_COLOUR,offsetof(Transition, text_color) },
  { "line_colour",PROP_TYPE_COLOUR,offsetof(Transition, line_color) },
  { "trigger", PROP_TYPE_STRING, offsetof(Transition, trigger_text) },
  { "action", PROP_TYPE_STRING, offsetof(Transition, action_text) },
  { "guard", PROP_TYPE_STRING, offsetof(Transition, guard_text) },
  { "trigger_text_pos", PROP_TYPE_POINT, offsetof(Transition,trigger_text_pos)},
  { "guard_text_pos", PROP_TYPE_POINT,   offsetof(Transition,guard_text_pos)},
  { "direction_inverted", PROP_TYPE_BOOL, offsetof(Transition,direction_inverted)},
  { NULL, 0, 0 }
};


#define TRANSITION_MENU_ADD_SEGMENT_OFFSET 0
#define TRANSITION_MENU_DEL_SEGMENT_OFFSET 1
#define TRANSITION_MENU_OFFSET_TO_ORTH_COMMON 2


static DiaMenuItem transition_menu_items[] = {
  { N_("Add segment"), transition_add_segment_cb, NULL, 0 } ,
  { N_("Delete segment"), transition_del_segment_cb, NULL, 0 } ,
  ORTHCONN_COMMON_MENUS
};


static DiaMenu transition_menu = {
  "Transition",
  sizeof (transition_menu_items) / sizeof(DiaMenuItem),
  transition_menu_items,
  NULL
};


static DiaObject *
transition_create (Point   *startpoint,
                   void    *user_data,
                   Handle **handle1,
                   Handle **handle2)
{
  Transition *transition;
  OrthConn *orth;
  DiaObject *obj;
  Point temp_point;

  if (transition_font == NULL) {
    transition_font =
      dia_font_new_from_style (DIA_FONT_SANS, TRANSITION_FONTHEIGHT);
  }

  transition = g_new0 (Transition, 1);

  orth = &transition->orth;
  obj = &orth->object;
  obj->type = &uml_transition_type;
  obj->ops = &uml_transition_ops;

  orthconn_init (orth, startpoint);

  transition->text_color = color_black;
  transition->line_color = attributes_get_foreground ();
  /* Prepare the handles for trigger and guard text */
  transition->trigger_text_handle.id = HANDLE_MOVE_TRIGGER_TEXT;
  transition->trigger_text_handle.type = HANDLE_MINOR_CONTROL;
  transition->trigger_text_handle.connect_type = HANDLE_NONCONNECTABLE;
  transition->trigger_text_handle.connected_to = NULL;
  temp_point = *startpoint;
  temp_point.y -= TEXT_HANDLE_DISTANCE_FROM_STARTPOINT;
  transition->trigger_text_pos = temp_point;
  transition->trigger_text_handle.pos = temp_point;
  object_add_handle (obj, &transition->trigger_text_handle);

  transition->guard_text_handle.id = HANDLE_MOVE_GUARD_TEXT;
  transition->guard_text_handle.type = HANDLE_MINOR_CONTROL;
  transition->guard_text_handle.connect_type = HANDLE_NONCONNECTABLE;
  transition->guard_text_handle.connected_to = NULL;
  temp_point = *startpoint;
  temp_point.y += TEXT_HANDLE_DISTANCE_FROM_STARTPOINT;
  transition->guard_text_pos = transition->guard_text_handle.pos = temp_point;
  object_add_handle (obj, &transition->guard_text_handle);

  transition->guard_text = NULL;
  transition->trigger_text = NULL;
  transition->action_text = NULL;

  uml_transition_update_data (transition);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return obj;
}


static DiaObject *
transition_load (ObjectNode obj_node, int version, DiaContext *ctx)
{
  DiaObject *obj = object_load_using_properties (&uml_transition_type,
                                                 obj_node,version,ctx);
  if (version == 0) {
    AttributeNode attr;
    /* In old objects with no autorouting, set it to false. */
    attr = object_find_attribute (obj_node, "autorouting");
    if (attr == NULL) {
      ((OrthConn*) obj)->autorouting = FALSE;
    }
  }
  if (version < 2) {
      /* Versions prior to 2 have the arrowheads inverted */
      ((Transition*) obj)->direction_inverted = TRUE;
  }
  return obj;
}


static void
transition_set_props (Transition *transition, GPtrArray *props)
{
  object_set_props_from_offsets (&transition->orth.object,
                                 transition_offsets, props);
  uml_transition_update_data (transition);
}


static void
transition_get_props (Transition *transition, GPtrArray *props)
{
  object_get_props_from_offsets (&transition->orth.object,
                                 transition_offsets, props);
}


static PropDescription *
transition_describe_props (Transition *transition)
{
  if (transition_props[0].quark == 0)
    prop_desc_list_calculate_quarks (transition_props);
  return transition_props;

}


static void
transition_destroy (Transition* transition)
{
  g_clear_pointer (&transition->trigger_text, g_free);
  g_clear_pointer (&transition->action_text, g_free);
  g_clear_pointer (&transition->guard_text, g_free);
  orthconn_destroy (&transition->orth);
}


static DiaMenu*
transition_get_object_menu (Transition *transition, Point *clickedpoint)
{
  OrthConn *orth = &transition->orth;
  /* Set/clear the active flag of the add/remove segment according to the
     placement of mouse pointer and placement of the transition and its handles */
  transition_menu_items[TRANSITION_MENU_ADD_SEGMENT_OFFSET].active =
      orthconn_can_add_segment (orth, clickedpoint);
  transition_menu_items[TRANSITION_MENU_DEL_SEGMENT_OFFSET].active =
      orthconn_can_delete_segment (orth, clickedpoint);

  orthconn_update_object_menu (orth, clickedpoint,
                               &transition_menu_items[TRANSITION_MENU_OFFSET_TO_ORTH_COMMON]);
  return &transition_menu;
}


static char *
create_event_action_text (Transition* transition)
{
  char *temp_text;

  if (transition->action_text && strlen (transition->action_text) != 0) {
    temp_text = g_strdup_printf ("%s/%s", transition->trigger_text,
                                          transition->action_text);
  } else {
    temp_text = g_strdup_printf ("%s",
                                 transition->trigger_text ? transition->trigger_text : "");
  }

  return temp_text;
}


static char *
create_guard_text (Transition* transition)
{
  return g_strdup_printf ("[%s]", transition->guard_text);
}


static void
transition_draw (Transition *transition, DiaRenderer *renderer)
{
  Arrow arrow;
  Arrow *start_arrow;
  Arrow *end_arrow;
  Point* points;
  OrthConn *orth = &transition->orth;
  int num_points;

  g_return_if_fail (transition != NULL);
  g_return_if_fail (renderer != NULL);


  /* Draw the arrow / line */
  points = &orth->points[0];
  num_points = orth->numpoints;

  arrow.type = ARROW_LINES;
  arrow.length = TRANSITION_ARROWLEN;
  arrow.width = TRANSITION_ARROWWIDTH;


  /* Is it necessary to call set_linewidth? The draw_line_with_arrows() method
     got a linewidth parameter... */
  dia_renderer_set_linewidth (renderer, TRANSITION_WIDTH);
  /* TODO, find out about the meaning of this... */
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

  if (transition->direction_inverted) {
    start_arrow = &arrow;
    end_arrow = NULL;
  } else {
    start_arrow = NULL;
    end_arrow = & arrow;
  }
  dia_renderer_draw_polyline_with_arrows (renderer,
                                          points,
                                          num_points,
                                          TRANSITION_WIDTH,
                                          &transition->line_color,
                                          start_arrow, end_arrow);


  dia_renderer_set_font (renderer,
                         transition_font,
                         TRANSITION_FONTHEIGHT);

  /* Draw the guard text */
  if (transition->guard_text && strlen (transition->guard_text) != 0) {
    char *text = create_guard_text (transition);
    dia_renderer_draw_string (renderer,
                              text,
                              &transition->guard_text_pos,
                              DIA_ALIGN_CENTRE,
                              &transition->text_color);
    g_clear_pointer (&text, g_free);
  }

  /* Draw the trigger text */
  if (transition->trigger_text && strlen (transition->trigger_text) != 0) {
    char *text = create_event_action_text (transition);
    dia_renderer_draw_string (renderer,
                              text,
                              &transition->trigger_text_pos,
                              DIA_ALIGN_CENTRE,
                              &transition->text_color);
    g_clear_pointer (&text, g_free);
  }
}


static double
transition_distance (Transition *transition, Point *point)
{
  return orthconn_distance_from (&transition->orth, point, TRANSITION_WIDTH);
}


static void
transition_select (Transition *transition,
                   Point       *clicked_point,
                   DiaRenderer *interactive_renderer)
{
  uml_transition_update_data(transition);
}


static DiaObjectChange *
transition_move (Transition* transition, Point* newpos)
{
  Point delta;
  DiaObjectChange *change;

  /* Find a delta in order to move the text handles along with the transition */
  delta = *newpos;
  point_sub (&delta, &transition->orth.points[0]);

  change = orthconn_move (&transition->orth, newpos);

  /* Move the text handles */
  point_add (&transition->trigger_text_pos, &delta);
  point_add (&transition->guard_text_pos, &delta);

  uml_transition_update_data (transition);

  return change;
}


static DiaObjectChange *
transition_move_handle (Transition       *transition,
                        Handle           *handle,
                        Point            *newpos,
                        ConnectionPoint  *cp,
                        HandleMoveReason  reason,
                        ModifierKeys      modifiers)
{
  DiaObjectChange *change = NULL;

  g_return_val_if_fail (transition != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (newpos != NULL, NULL);

  switch (handle->id) {
    case HANDLE_MOVE_TRIGGER_TEXT:
      /* The trigger text is being moved */
      transition->trigger_text_pos = *newpos;
      break;

    case HANDLE_MOVE_GUARD_TEXT:
      /* The guard text is being moved */
      transition->guard_text_pos = *newpos;
      break;

    case HANDLE_RESIZE_NW:
    case HANDLE_RESIZE_N:
    case HANDLE_RESIZE_NE:
    case HANDLE_RESIZE_W:
    case HANDLE_RESIZE_E:
    case HANDLE_RESIZE_SW:
    case HANDLE_RESIZE_S:
    case HANDLE_RESIZE_SE:
    case HANDLE_MOVE_STARTPOINT:
    case HANDLE_MOVE_ENDPOINT:
    case HANDLE_CUSTOM1:
    case HANDLE_CUSTOM4:
    case HANDLE_CUSTOM5:
    case HANDLE_CUSTOM6:
    case HANDLE_CUSTOM7:
    case HANDLE_CUSTOM8:
    case HANDLE_CUSTOM9:
    default:
      {
        int n = transition->orth.numpoints / 2;
        Point p1, p2;

        p1.x = 0.5 * (transition->orth.points[n-1].x + transition->orth.points[n].x);
        p1.y = 0.5 * (transition->orth.points[n-1].y + transition->orth.points[n].y);

        /* Tell the connection that one of its handles is being moved */
        change = orthconn_move_handle (&transition->orth,
                                       handle,
                                       newpos,
                                       cp,
                                       reason,
                                       modifiers);
        /* with auto-routing the number of points may have changed */
        n = transition->orth.numpoints/2;
        p2.x = 0.5 * (transition->orth.points[n-1].x + transition->orth.points[n].x);
        p2.y = 0.5 * (transition->orth.points[n-1].y + transition->orth.points[n].y);
        point_sub (&p2, &p1);

        point_add (&transition->trigger_text_pos, &p2);
        point_add (&transition->guard_text_pos, &p2);
      }
      break;
  }

  /* Update ourselves to reflect the new handle position */
  uml_transition_update_data (transition);

  return change;
}


static DiaObjectChange *
transition_add_segment_cb (DiaObject *obj,
                           Point     *clickedpoint,
                           gpointer   data)
{
  DiaObjectChange *change;

  change = orthconn_add_segment ((OrthConn *) obj, clickedpoint);
  uml_transition_update_data ((Transition *) obj);

  return change;
}


static DiaObjectChange *
transition_del_segment_cb (DiaObject *obj,
                           Point     *clickedpoint,
                           gpointer   data)
{
  DiaObjectChange *change;

  change = orthconn_delete_segment ((OrthConn *) obj, clickedpoint);
  uml_transition_update_data ((Transition *) obj);

  return change;
}


static void
expand_bbox_for_text (DiaRectangle *bbox,
                      Point        *text_pos,
                      char         *text)
{
  DiaRectangle text_box;
  double text_width;

  text_width = dia_font_string_width (text, transition_font,
                                      TRANSITION_FONTHEIGHT);
  text_box.left = text_pos->x - text_width/2;
  text_box.right = text_box.left + text_width;
  text_box.top = text_pos->y - dia_font_ascent (text, transition_font,
                                                TRANSITION_FONTHEIGHT);
  text_box.bottom = text_box.top + TRANSITION_FONTHEIGHT;
  rectangle_union (bbox, &text_box);
}


static void
uml_transition_update_data (Transition *transition)
{
  char *temp_text;
  Point *points;

  /* Setup helpful pointers as shortcuts */
  OrthConn *orth = &transition->orth;
  DiaObject *obj = &orth->object;
  PolyBBExtras *extra = &orth->extra_spacing;
  points = &orth->points[0];

  /* Set the transitions position */
  obj->position = points[0];
  transition->trigger_text_handle.pos = transition->trigger_text_pos;
  transition->guard_text_handle.pos = transition->guard_text_pos;

  /* Update the orthogonal connection to match the new data */
  orthconn_update_data (orth);

  extra->start_long = extra->end_long
                    = extra->middle_trans
                    = TRANSITION_WIDTH/2.0;
  extra->start_trans = extra->end_trans
                     = MAX (TRANSITION_ARROWLEN, TRANSITION_WIDTH/2.0);

  /* Update the bounding box to match the new connection data */
  orthconn_update_boundingbox (orth);
  /* Update the bounding box to match the new trigger text size and position */
  temp_text = create_event_action_text (transition);
  expand_bbox_for_text (&obj->bounding_box, &transition->trigger_text_pos,
                        temp_text);
  g_clear_pointer (&temp_text, g_free);
  /* Update the bounding box to match the new guard text size and position */
  temp_text = g_strdup_printf ("[%s]", transition->guard_text ? transition->guard_text : "");
  expand_bbox_for_text (&obj->bounding_box, &transition->guard_text_pos,
                        temp_text);
  g_clear_pointer (&temp_text, g_free);
}
