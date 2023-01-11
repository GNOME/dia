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

/* DO NOT USE THIS OBJECT AS A BASIS FOR A NEW OBJECT. */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>
#include <string.h>

#include "object.h"
#include "orth_conn.h"
#include "diarenderer.h"
#include "attributes.h"
#include "arrows.h"
#include "properties.h"

#include "pixmaps/participation.xpm"

typedef struct _Participation Participation;

struct _Participation {
  OrthConn orth;

  gboolean total;
};


#define PARTICIPATION_WIDTH 0.1
#define PARTICIPATION_DASHLEN 0.4
#define PARTICIPATION_FONTHEIGHT 0.8
#define TOTAL_SEPARATION 0.25

static real participation_distance_from(Participation *dep, Point *point);
static void participation_select(Participation *dep, Point *clicked_point,
			      DiaRenderer *interactive_renderer);
static DiaObjectChange *participation_move_handle    (Participation    *dep,
                                                      Handle           *handle,
                                                      Point            *to,
                                                      ConnectionPoint  *cp,
                                                      HandleMoveReason  reason,
                                                      ModifierKeys      modifiers);
static DiaObjectChange *participation_move           (Participation    *dep,
                                                      Point            *to);
static void participation_draw(Participation *dep, DiaRenderer *renderer);
static DiaObject *participation_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static DiaObject *participation_copy(Participation *dep);
static void participation_save(Participation *dep, ObjectNode obj_node,
			       DiaContext *ctx);
static DiaObject *participation_load(ObjectNode obj_node, int version,DiaContext *ctx);
static void participation_update_data(Participation *dep);
static PropDescription *
participation_describe_props(Participation *participation);
static void
participation_get_props(Participation *participation, GPtrArray *props);
static void
participation_set_props(Participation *participation, GPtrArray *props);
static DiaMenu *participation_get_object_menu(Participation *participation,
					      Point *clickedpoint);

static ObjectTypeOps participation_type_ops =
{
  (CreateFunc) participation_create,
  (LoadFunc)   participation_load,
  (SaveFunc)   participation_save
};

DiaObjectType participation_type =
{
  "ER - Participation",  /* name */
  /* Version 0 had no autorouting and so shouldn't have it set by default. */
  1,                  /* version */
  participation_xpm,   /* pixmap */
  &participation_type_ops /* ops */
};

static ObjectOps participation_ops = {
  (DestroyFunc)         orthconn_destroy,
  (DrawFunc)            participation_draw,
  (DistanceFunc)        participation_distance_from,
  (SelectFunc)          participation_select,
  (CopyFunc)            participation_copy,
  (MoveFunc)            participation_move,
  (MoveHandleFunc)      participation_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      participation_get_object_menu,
  (DescribePropsFunc)   participation_describe_props,
  (GetPropsFunc)        participation_get_props,
  (SetPropsFunc)        participation_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription participation_props[] = {
  ORTHCONN_COMMON_PROPERTIES,
  { "total", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Total:"), NULL, NULL },
  PROP_DESC_END
};

static PropDescription *
participation_describe_props(Participation *participation)
{
  if (participation_props[0].quark == 0)
    prop_desc_list_calculate_quarks(participation_props);
  return participation_props;
}

static PropOffset participation_offsets[] = {
  ORTHCONN_COMMON_PROPERTIES_OFFSETS,
  { "total", PROP_TYPE_BOOL, offsetof(Participation, total) },
  { NULL, 0, 0}
};


static void
participation_get_props(Participation *participation, GPtrArray *props)
{
  object_get_props_from_offsets(&participation->orth.object,
                                participation_offsets, props);
}

static void
participation_set_props(Participation *participation, GPtrArray *props)
{
  object_set_props_from_offsets(&participation->orth.object,
                                participation_offsets, props);
  participation_update_data(participation);
}


static real
participation_distance_from(Participation *participation, Point *point)
{
  OrthConn *orth = &participation->orth;
  return orthconn_distance_from(orth, point, PARTICIPATION_WIDTH);
}

static void
participation_select(Participation *participation, Point *clicked_point,
		  DiaRenderer *interactive_renderer)
{
  orthconn_update_data(&participation->orth);
}


static DiaObjectChange *
participation_move_handle (Participation    *participation,
                           Handle           *handle,
                           Point            *to,
                           ConnectionPoint  *cp,
                           HandleMoveReason  reason,
                           ModifierKeys      modifiers)
{
  DiaObjectChange *change;

  g_return_val_if_fail (participation != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  change = orthconn_move_handle (&participation->orth,
                                 handle,
                                 to,
                                 cp,
                                 reason,
                                 modifiers);
  participation_update_data (participation);

  return change;
}


static DiaObjectChange *
participation_move(Participation *participation, Point *to)
{
  DiaObjectChange *change;

  change = orthconn_move (&participation->orth, to);
  participation_update_data (participation);

  return change;
}


static void
participation_draw (Participation *participation, DiaRenderer *renderer)
{
  OrthConn *orth = &participation->orth;
  Point *points;
  Point *left_points;
  Point *right_points;
  int i, n;
  real last_left, last_right;

  points = &orth->points[0];
  n = orth->numpoints;

  last_left = 0.0;
  last_right = 0.0;

  dia_renderer_set_linewidth (renderer, PARTICIPATION_WIDTH);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  if (participation->total) {
    left_points = g_new0 (Point, n);
    right_points = g_new0 (Point, n);
    for(i = 0; i < n - 1; i++) {
      if(orth->orientation[i] == HORIZONTAL) { /* HORIZONTAL */
        if (points[i].x < points[i+1].x) { /* RIGHT */
          left_points[i].x = points[i].x + last_left;
          left_points[i].y = points[i].y - TOTAL_SEPARATION / 2.0;
          last_left = - TOTAL_SEPARATION/2.0;
          right_points[i].x = points[i].x + last_right;
          right_points[i].y = points[i].y + TOTAL_SEPARATION / 2.0;
          last_right = TOTAL_SEPARATION/2.0;
        } else { /* LEFT */
          left_points[i].x = points[i].x + last_left;
          left_points[i].y = points[i].y + TOTAL_SEPARATION / 2.0;
          last_left = TOTAL_SEPARATION/2.0;
          right_points[i].x = points[i].x + last_right;
          right_points[i].y = points[i].y - TOTAL_SEPARATION / 2.0;
          last_right = - TOTAL_SEPARATION/2.0;
        }
      } else { /* VERTICAL */
        if (points[i].y < points[i+1].y) { /* DOWN */
          left_points[i].x = points[i].x + TOTAL_SEPARATION / 2.0;
          left_points[i].y = points[i].y + last_left;
          last_left = TOTAL_SEPARATION/2.0;
          right_points[i].x = points[i].x - TOTAL_SEPARATION / 2.0;
          right_points[i].y = points[i].y + last_right;
          last_right = - TOTAL_SEPARATION/2.0;
        } else { /* UP */
          left_points[i].x = points[i].x - TOTAL_SEPARATION / 2.0;
          left_points[i].y = points[i].y + last_left;
          last_left = - TOTAL_SEPARATION/2.0;
          right_points[i].x = points[i].x + TOTAL_SEPARATION / 2.0;
          right_points[i].y = points[i].y + last_right;
          last_right = TOTAL_SEPARATION/2.0;
        }
      }
    }
    if(orth->orientation[i-1] == HORIZONTAL) { /* HORIZONTAL */
      left_points[i].x = points[i].x;
      left_points[i].y = points[i].y + last_left;
      right_points[i].x = points[i].x;
      right_points[i].y = points[i].y + last_right;
    } else { /* VERTICAL */
      left_points[i].x = points[i].x + last_left;
      left_points[i].y = points[i].y;
      right_points[i].x = points[i].x + last_right;
      right_points[i].y = points[i].y;
    }

    dia_renderer_draw_polyline (renderer, left_points, n, &color_black);
    dia_renderer_draw_polyline (renderer, right_points, n, &color_black);
    g_clear_pointer (&left_points, g_free);
    g_clear_pointer (&right_points, g_free);
  } else {
    dia_renderer_draw_polyline (renderer, points, n, &color_black);
  }
}

static void
participation_update_data(Participation *participation)
{
  OrthConn *orth = &participation->orth;
  PolyBBExtras *extra = &orth->extra_spacing;
  real extra_width;

  orthconn_update_data(orth);

  if (participation->total) {
    extra_width = TOTAL_SEPARATION/2.0;
  } else {
    extra_width = 0.0;
  }
  extra->middle_trans =
    extra->start_trans =
    extra->end_trans =
    extra->start_long =
    extra->end_long = PARTICIPATION_WIDTH/2.0 + extra_width;

  orthconn_update_boundingbox(orth);
}

static DiaObject *
participation_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Participation *participation;
  OrthConn *orth;
  DiaObject *obj;

  participation = g_new0 (Participation, 1);
  orth = &participation->orth;
  obj = &orth->object;

  obj->type = &participation_type;

  obj->ops = &participation_ops;

  orthconn_init(orth, startpoint);

  participation_update_data(participation);

  participation->total = FALSE;

  *handle1 = orth->handles[0];
  *handle2 = orth->handles[orth->numpoints-2];

  return &participation->orth.object;
}

static DiaObject *
participation_copy(Participation *participation)
{
  Participation *newparticipation;
  OrthConn *orth, *neworth;

  orth = &participation->orth;

  newparticipation = g_new0 (Participation, 1);
  neworth = &newparticipation->orth;

  orthconn_copy(orth, neworth);

  newparticipation->total = participation->total;

  participation_update_data(newparticipation);

  return &newparticipation->orth.object;
}


static void
participation_save(Participation *participation, ObjectNode obj_node,
		   DiaContext *ctx)
{
  orthconn_save(&participation->orth, obj_node, ctx);

  data_add_boolean(new_attribute(obj_node, "total"),
		   participation->total, ctx);
}

static DiaObject *
participation_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  AttributeNode attr;
  Participation *participation;
  OrthConn *orth;
  DiaObject *obj;

  participation = g_new0(Participation, 1);

  orth = &participation->orth;
  obj = &orth->object;

  obj->type = &participation_type;
  obj->ops = &participation_ops;

  orthconn_load(orth, obj_node, ctx);

  attr = object_find_attribute(obj_node, "total");
  if (attr != NULL)
    participation->total = data_boolean(attribute_first_data(attr), ctx);

  participation_update_data(participation);

  return &participation->orth.object;
}


static DiaObjectChange *
participation_add_segment_callback (DiaObject *obj,
                                    Point     *clicked,
                                    gpointer   data)
{
  DiaObjectChange *change;

  change = orthconn_add_segment ((OrthConn *) obj, clicked);
  participation_update_data ((Participation *) obj);

  return change;
}


static DiaObjectChange *
participation_delete_segment_callback (DiaObject *obj,
                                       Point     *clicked,
                                       gpointer   data)
{
  DiaObjectChange *change;

  change = orthconn_delete_segment ((OrthConn *) obj, clicked);
  participation_update_data ((Participation *) obj);

  return change;
}


static DiaMenuItem object_menu_items[] = {
  { N_("Add segment"), participation_add_segment_callback, NULL, 1 },
  { N_("Delete segment"), participation_delete_segment_callback, NULL, 1 },
  ORTHCONN_COMMON_MENUS,
};

static DiaMenu object_menu = {
  "Participation",
  sizeof(object_menu_items)/sizeof(DiaMenuItem),
  object_menu_items,
  NULL
};

static DiaMenu *
participation_get_object_menu(Participation *participation, Point *clickedpoint)
{
  OrthConn *orth;

  orth = &participation->orth;
  /* Set entries sensitive/selected etc here */
  object_menu_items[0].active = orthconn_can_add_segment(orth, clickedpoint);
  object_menu_items[1].active = orthconn_can_delete_segment(orth, clickedpoint);
  orthconn_update_object_menu(orth, clickedpoint, &object_menu_items[2]);
  return &object_menu;
}
