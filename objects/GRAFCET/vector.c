/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * GRAFCET charts support for Dia
 * Copyright (C) 2000 Cyrille Chepelov
 *
 * This file is derived from objects/standard/zigzag.c
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

#include <math.h>

#include "object.h"
#include "orth_conn.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "properties.h"

#include "grafcet.h"
#include "pixmaps/vector.xpm"

#define ARC_LINE_WIDTH (GRAFCET_GENERAL_LINE_WIDTH)
#define ARC_ARROW_LENGTH .8
#define ARC_ARROW_WIDTH .6
#define ARC_ARROW_TYPE ARROW_FILLED_TRIANGLE

#define HANDLE_MIDDLE HANDLE_CUSTOM1

typedef struct _Arc {
  OrthConn orth;

  gboolean uparrow;
} Arc;


static DiaObjectChange *arc_move_handle         (Arc              *arc,
                                                 Handle           *handle,
                                                 Point            *to,
                                                 ConnectionPoint  *cp,
                                                 HandleMoveReason  reason,
                                                 ModifierKeys      modifiers);
static DiaObjectChange *arc_move                (Arc              *arc,
                                                 Point            *to);
static void arc_select(Arc *arc, Point *clicked_point,
			      DiaRenderer *interactive_renderer);
static void arc_draw(Arc *arc, DiaRenderer *renderer);
static DiaObject *arc_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real arc_distance_from(Arc *arc, Point *point);
static void arc_update_data(Arc *arc);
static void arc_destroy(Arc *arc);
static DiaMenu *arc_get_object_menu(Arc *arc,
					   Point *clickedpoint);

static DiaObject *arc_load(ObjectNode obj_node, int version,DiaContext *ctx);
static PropDescription *arc_describe_props(Arc *arc);
static void arc_get_props(Arc *arc,
                                 GPtrArray *props);
static void arc_set_props(Arc *arc,
                                 GPtrArray *props);

static ObjectTypeOps arc_type_ops =
{
  (CreateFunc)arc_create,   /* create */
  (LoadFunc)  arc_load,/* using_properties */     /* load */
  (SaveFunc)  object_save_using_properties,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL,
};

DiaObjectType old_arc_type =
{
  "GRAFCET - Vector", /* name */
  /* Version 0 had no autorouting and so shouldn't have it set by default. */
  1,               /* version */
  vector_xpm,       /* pixmap */
  &arc_type_ops        /* ops */
};

DiaObjectType grafcet_arc_type =
{
  "GRAFCET - Arc",   /* name */
  0,              /* version */
  vector_xpm,      /* pixmap */
  &arc_type_ops       /* ops */
};

static ObjectOps arc_ops = {
  (DestroyFunc)         arc_destroy,
  (DrawFunc)            arc_draw,
  (DistanceFunc)        arc_distance_from,
  (SelectFunc)          arc_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            arc_move,
  (MoveHandleFunc)      arc_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      arc_get_object_menu,
  (DescribePropsFunc)   arc_describe_props,
  (GetPropsFunc)        arc_get_props,
  (SetPropsFunc)        arc_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription arc_props[] = {
  ORTHCONN_COMMON_PROPERTIES,
  { "uparrow",PROP_TYPE_BOOL,PROP_FLAG_VISIBLE,
    N_("Draw arrow heads on upward arcs:"),NULL},
  PROP_DESC_END
};

static PropDescription *
arc_describe_props(Arc *arc)
{
  if (arc_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(arc_props);
  }
  return arc_props;
}

static PropOffset arc_offsets[] = {
  ORTHCONN_COMMON_PROPERTIES_OFFSETS,
  { "uparrow",PROP_TYPE_BOOL,offsetof(Arc,uparrow)},
  { NULL,0,0 }
};

static void
arc_get_props(Arc *arc, GPtrArray *props)
{
  object_get_props_from_offsets(&arc->orth.object,
                                arc_offsets,props);
}

static void
arc_set_props(Arc *arc, GPtrArray *props)
{
  object_set_props_from_offsets(&arc->orth.object,
                                arc_offsets,props);
  arc_update_data(arc);
}

static real
arc_distance_from(Arc *arc, Point *point)
{
  OrthConn *orth = &arc->orth;
  return orthconn_distance_from(orth, point, ARC_LINE_WIDTH);
}

static void
arc_select(Arc *arc, Point *clicked_point,
		  DiaRenderer *interactive_renderer)
{
  orthconn_update_data(&arc->orth);
}


static DiaObjectChange *
arc_move_handle (Arc              *arc,
                 Handle           *handle,
                 Point            *to,
                 ConnectionPoint  *cp,
                 HandleMoveReason  reason,
                 ModifierKeys      modifiers)
{
  DiaObjectChange *change;

  change = orthconn_move_handle (&arc->orth, handle, to, cp, reason, modifiers);
  arc_update_data (arc);

  return change;
}


static DiaObjectChange *
arc_move (Arc *arc, Point *to)
{
  DiaObjectChange *change;

  change = orthconn_move (&arc->orth, to);
  arc_update_data (arc);

  return change;
}


static void
arc_draw (Arc *arc, DiaRenderer *renderer)
{
  OrthConn *orth = &arc->orth;
  Point *points;
  int n,i;
  Arrow arrow = { ARC_ARROW_TYPE, ARC_ARROW_LENGTH, ARC_ARROW_WIDTH };

  points = &orth->points[0];
  n = orth->numpoints;

  dia_renderer_set_linewidth (renderer, ARC_LINE_WIDTH);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  dia_renderer_draw_polyline (renderer, points, n, &color_black);

  if (arc->uparrow) {
    for (i=0;i<n-1; i++) {
      if ((points[i].y > points[i+1].y) &&
          (ABS (points[i+1].y-points[i].y) > 5 * ARC_ARROW_LENGTH)) {
        /* Draw an arrow on the middle of the line */
        Point m;
        m.x = points[i].x; /* == points[i+1].x */
        m.y = .5 * (points[i].y + points[i+1].y) - (.5 * ARC_ARROW_LENGTH);
        dia_arrow_draw (&arrow,
                        renderer,
                        &m,
                        &points[i],
                        ARC_LINE_WIDTH,
                        &color_black,
                        &color_white);
      }
    }
  }
}

static DiaObject *
arc_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Arc *arc;
  OrthConn *orth;
  DiaObject *obj;

  arc = g_new0 (Arc, 1);
  orth = &arc->orth;
  obj = &orth->object;

  obj->type = &grafcet_arc_type;
  obj->ops = &arc_ops;

  orthconn_init(orth, startpoint);


  arc->uparrow = TRUE;
  arc_update_data(arc);

  *handle1 = orth->handles[0];
  *handle2 = orth->handles[orth->numhandles-1];
  return &arc->orth.object;
}

static void
arc_destroy(Arc *arc)
{
  orthconn_destroy(&arc->orth);
}

static void
arc_update_data(Arc *arc)
{
  OrthConn *orth = &arc->orth;
  PolyBBExtras *extra = &orth->extra_spacing;

  orthconn_update_data(&arc->orth);

  extra->start_trans =
    extra->start_long =
    extra->end_long  =
    extra->end_trans = ARC_LINE_WIDTH/2.0;
  if (arc->uparrow) {
    extra->middle_trans = (ARC_LINE_WIDTH + ARC_ARROW_WIDTH)/2.0;
  } else {
    extra->middle_trans = ARC_LINE_WIDTH/2.0;
  }

  orthconn_update_boundingbox(orth);
}


static DiaObjectChange *
arc_add_segment_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  DiaObjectChange *change;

  change = orthconn_add_segment ((OrthConn *) obj, clicked);
  arc_update_data ((Arc *) obj);

  return change;
}


static DiaObjectChange *
arc_delete_segment_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  DiaObjectChange *change;

  change = orthconn_delete_segment ((OrthConn *) obj, clicked);
  arc_update_data ((Arc *) obj);

  return change;
}


static DiaMenuItem object_menu_items[] = {
  { N_("Add segment"), arc_add_segment_callback, NULL, 1 },
  { N_("Delete segment"), arc_delete_segment_callback, NULL, 1 },
  ORTHCONN_COMMON_MENUS,
};


static DiaMenu object_menu = {
  "Arc",
  sizeof(object_menu_items)/sizeof(DiaMenuItem),
  object_menu_items,
  NULL
};

static DiaMenu *
arc_get_object_menu(Arc *arc, Point *clickedpoint)
{
  OrthConn *orth;

  orth = &arc->orth;
  /* Set entries sensitive/selected etc here */
  object_menu_items[0].active = orthconn_can_add_segment(orth, clickedpoint);
  object_menu_items[1].active = orthconn_can_delete_segment(orth, clickedpoint);
  orthconn_update_object_menu(orth, clickedpoint, &object_menu_items[2]);
  return &object_menu;
}


static DiaObject *
arc_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  DiaObject *obj = object_load_using_properties(&grafcet_arc_type,
                                                obj_node,version,ctx);
  if (version == 0) {
    AttributeNode attr;
    /* In old objects with no autorouting, set it to false. */
    attr = object_find_attribute(obj_node, "autorouting");
    if (attr == NULL)
      ((OrthConn*)obj)->autorouting = FALSE;
  }
  return obj;
}
