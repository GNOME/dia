/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * SADT diagram support Copyright(C) 2000 Cyrille Chepelov
 * This file has been forked (sigh!) from Alex's zigzagline.c.
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
#include "color.h"
#include "properties.h"

#include "sadt.h"

#include "pixmaps/arrow.xpm"


#define ARROW_CORNER_RADIUS .75
#define ARROW_LINE_WIDTH 0.10
#define ARROW_HEAD_LENGTH .8
#define ARROW_HEAD_WIDTH .8
#define ARROW_HEAD_TYPE ARROW_FILLED_TRIANGLE
#define ARROW_DOT_LOFFSET .4
#define ARROW_DOT_WOFFSET .5
#define ARROW_DOT_RADIUS .25
#define ARROW_PARENS_WOFFSET .5
#define ARROW_PARENS_LOFFSET .55
#define ARROW_PARENS_LENGTH 1.0
#define HANDLE_MIDDLE HANDLE_CUSTOM1

typedef enum { SADT_ARROW_NORMAL,
	       SADT_ARROW_IMPORTED,
	       SADT_ARROW_IMPLIED,
	       SADT_ARROW_DOTTED,
	       SADT_ARROW_DISABLED } Sadtarrow_style;

typedef struct _Sadtarrow {
  OrthConn orth;

  Sadtarrow_style style;
  gboolean autogray;

  Color line_color;
} Sadtarrow;


static DiaObjectChange *sadtarrow_move_handle        (Sadtarrow        *sadtarrow,
                                                      Handle           *handle,
                                                      Point            *to,
                                                      ConnectionPoint  *cp,
                                                      HandleMoveReason  reason,
                                                      ModifierKeys      modifiers);
static DiaObjectChange *sadtarrow_move               (Sadtarrow        *sadtarrow,
                                                      Point            *to);
static void sadtarrow_select(Sadtarrow *sadtarrow, Point *clicked_point,
			      DiaRenderer *interactive_renderer);
static void sadtarrow_draw(Sadtarrow *sadtarrow, DiaRenderer *renderer);
static DiaObject *sadtarrow_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real sadtarrow_distance_from(Sadtarrow *sadtarrow, Point *point);
static void sadtarrow_update_data(Sadtarrow *sadtarrow);
static void sadtarrow_destroy(Sadtarrow *sadtarrow);
static DiaMenu *sadtarrow_get_object_menu(Sadtarrow *sadtarrow,
					   Point *clickedpoint);

static DiaObject *sadtarrow_load(ObjectNode obj_node, int version,
				 DiaContext *ctx);
static PropDescription *sadtarrow_describe_props(Sadtarrow *sadtarrow);
static void sadtarrow_get_props(Sadtarrow *sadtarrow,
                                 GPtrArray *props);
static void sadtarrow_set_props(Sadtarrow *sadtarrow,
                                 GPtrArray *props);


static ObjectTypeOps sadtarrow_type_ops =
{
  (CreateFunc)sadtarrow_create,   /* create */
  (LoadFunc)  sadtarrow_load,     /* load */
  (SaveFunc)  object_save_using_properties, /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType sadtarrow_type =
{
  "SADT - arrow",    /* name */
  /* Version 0 had no autorouting and so shouldn't have it set by default. */
  1,              /* version */
  arrow_xpm,       /* pixmap */
  &sadtarrow_type_ops /* ops */
};


static ObjectOps sadtarrow_ops = {
  (DestroyFunc)         sadtarrow_destroy,
  (DrawFunc)            sadtarrow_draw,
  (DistanceFunc)        sadtarrow_distance_from,
  (SelectFunc)          sadtarrow_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            sadtarrow_move,
  (MoveHandleFunc)      sadtarrow_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      sadtarrow_get_object_menu,
  (DescribePropsFunc)   sadtarrow_describe_props,
  (GetPropsFunc)        sadtarrow_get_props,
  (SetPropsFunc)        sadtarrow_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

PropEnumData flow_style[] = {
  { N_("Normal"),SADT_ARROW_NORMAL },
  { N_("Import resource (not shown upstairs)"),SADT_ARROW_IMPORTED },
  { N_("Imply resource (not shown downstairs)"),SADT_ARROW_IMPLIED },
  { N_("Dotted arrow"),SADT_ARROW_DOTTED },
  { N_("disable arrow heads"),SADT_ARROW_DISABLED },
  { NULL }};

static PropDescription sadtarrow_props[] = {
  ORTHCONN_COMMON_PROPERTIES,

  { "arrow_style", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE,
    N_("Flow style:"), NULL, flow_style },
  { "autogray",PROP_TYPE_BOOL,PROP_FLAG_VISIBLE,
    N_("Automatically gray vertical flows:"),
    N_("To improve the ease of reading, flows which begin and end vertically "
       "can be rendered gray")},
  PROP_STD_LINE_COLOUR_OPTIONAL,
  PROP_DESC_END
};

static PropDescription *
sadtarrow_describe_props(Sadtarrow *sadtarrow)
{
  if (sadtarrow_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(sadtarrow_props);
  }
  return sadtarrow_props;
}

static PropOffset sadtarrow_offsets[] = {
  ORTHCONN_COMMON_PROPERTIES_OFFSETS,
  { "arrow_style", PROP_TYPE_ENUM, offsetof(Sadtarrow,style)},
  { "autogray",PROP_TYPE_BOOL, offsetof(Sadtarrow,autogray)},
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Sadtarrow, line_color) },
  { NULL, 0, 0 }
};

static void
sadtarrow_get_props(Sadtarrow *sadtarrow, GPtrArray *props)
{
  object_get_props_from_offsets(&sadtarrow->orth.object,
                                sadtarrow_offsets,props);
}

static void
sadtarrow_set_props(Sadtarrow *sadtarrow, GPtrArray *props)
{
  object_set_props_from_offsets(&sadtarrow->orth.object,
                                sadtarrow_offsets,props);
  sadtarrow_update_data(sadtarrow);
}


static real
sadtarrow_distance_from(Sadtarrow *sadtarrow, Point *point)
{
  OrthConn *orth = &sadtarrow->orth;
  return orthconn_distance_from(orth, point, ARROW_LINE_WIDTH);
}

static void
sadtarrow_select(Sadtarrow *sadtarrow, Point *clicked_point,
		  DiaRenderer *interactive_renderer)
{
  orthconn_update_data(&sadtarrow->orth);
}


static DiaObjectChange *
sadtarrow_move_handle (Sadtarrow        *sadtarrow,
                       Handle           *handle,
                       Point            *to,
                       ConnectionPoint  *cp,
                       HandleMoveReason  reason,
                       ModifierKeys      modifiers)
{
  DiaObjectChange *change;

  g_return_val_if_fail (sadtarrow != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  change = orthconn_move_handle (&sadtarrow->orth,
                                 handle,
                                 to,
                                 cp,
                                 reason,
                                 modifiers);
  sadtarrow_update_data (sadtarrow);

  return change;
}


static DiaObjectChange *
sadtarrow_move (Sadtarrow *sadtarrow, Point *to)
{
  DiaObjectChange *change;

  change = orthconn_move (&sadtarrow->orth, to);
  sadtarrow_update_data (sadtarrow);

  return change;
}


static void draw_dot(DiaRenderer *renderer,
		     Point *end, Point *vect, Color *col);
static void draw_tunnel(DiaRenderer *renderer,
			     Point *end, Point *vect, Color *col);

#define GBASE .45
#define GMULT .55


static void
sadtarrow_draw (Sadtarrow *sadtarrow, DiaRenderer *renderer)
{
  OrthConn *orth = &sadtarrow->orth;
  Point *points;
  int n;
  Color col;
  Arrow arrow;

  points = &orth->points[0];
  n = orth->numpoints;

  dia_renderer_set_linewidth (renderer, ARROW_LINE_WIDTH);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  col = sadtarrow->line_color;
  if (sadtarrow->autogray &&
      (orth->orientation[0] == VERTICAL) &&
      (orth->orientation[orth->numpoints-2] == VERTICAL)) {
    col.red = GBASE + (GMULT*col.red);
    col.green = GBASE + (GMULT*col.green);
    col.blue = GBASE + (GMULT*col.blue);
  }

  arrow.type = ARROW_HEAD_TYPE;
  arrow.length = ARROW_HEAD_LENGTH;
  arrow.width = ARROW_HEAD_WIDTH;

  dia_renderer_draw_rounded_polyline_with_arrows (renderer,
                                                  points,
                                                  n,
                                                  ARROW_LINE_WIDTH,
                                                  &col,
                                                  sadtarrow->style == SADT_ARROW_DOTTED?&arrow:NULL,
                                                  sadtarrow->style != SADT_ARROW_DISABLED?&arrow:NULL,
                                                  ARROW_CORNER_RADIUS);

  /* Draw the extra stuff. */
  switch (sadtarrow->style) {
    case SADT_ARROW_IMPORTED:
      draw_tunnel (renderer,&points[0],&points[1],&col);
      break;
    case SADT_ARROW_IMPLIED:
      draw_tunnel (renderer,&points[n-1],&points[n-2],&col);
      break;
    case SADT_ARROW_DOTTED:
      draw_dot (renderer,&points[n-1], &points[n-2],&col);
      draw_dot (renderer,&points[0], &points[1],&col);
      break;
    case SADT_ARROW_NORMAL:
    case SADT_ARROW_DISABLED:
    default:
      break;
  }
}


static void
draw_dot (DiaRenderer *renderer,
          Point       *end,
          Point       *vect,
          Color       *col)
{
  Point vv,vp,vt,pt;
  real vlen;
  vv = *end;
  point_sub (&vv,vect);
  vlen = distance_point_point (vect,end);
  if (vlen < 1E-7) return;
  point_scale (&vv,1/vlen);

  vp.y = vv.x;
  vp.x = -vv.y;

  pt = *end;
    vt = vp;
  point_scale (&vt,ARROW_DOT_WOFFSET);
  point_add (&pt,&vt);
  vt = vv;
  point_scale (&vt,-ARROW_DOT_LOFFSET);
  point_add (&pt,&vt);

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_draw_ellipse (renderer,
                             &pt,
                             ARROW_DOT_RADIUS,
                             ARROW_DOT_RADIUS,
                             col,
                             NULL);
}

static void
draw_tunnel (DiaRenderer *renderer,
             Point       *end,
             Point       *vect,
             Color       *col)
{
  Point vv,vp,vt1,vt2;
  BezPoint curve1[2];
  BezPoint curve2[2];

  real vlen;
  vv = *end;
  point_sub (&vv,vect);
  vlen = distance_point_point (vect,end);
  if (vlen < 1E-7) return;
  point_scale (&vv,1/vlen);
  vp.y = vv.x;
  vp.x = -vv.y;

  curve1[0].type = curve2[0].type = BEZ_MOVE_TO;
  curve1[0].p1   = curve2[0].p1   = *end;
  vt1 = vv;
  point_scale (&vt1,-ARROW_PARENS_LOFFSET - (.5*ARROW_PARENS_LENGTH));
  point_add (&curve1[0].p1,&vt1); point_add (&curve2[0].p1,&vt1);
                                           /* gcc, work for me, please. */
  vt2 = vp;
  point_scale (&vt2,ARROW_PARENS_WOFFSET);
  point_add (&curve1[0].p1,&vt2);  point_sub (&curve2[0].p1,&vt2);

  vt2 = vp;
  vt1 = vv;
  point_scale (&vt1,2.0*ARROW_PARENS_LENGTH / 6.0);
  point_scale (&vt2,ARROW_PARENS_LENGTH / 6.0);
  curve1[1].type = curve2[1].type = BEZ_CURVE_TO;
  curve1[1].p1 = curve1[0].p1;  curve2[1].p1 = curve2[0].p1;
  point_add (&curve1[1].p1,&vt1);  point_add (&curve2[1].p1,&vt1);
  point_add (&curve1[1].p1,&vt2);  point_sub (&curve2[1].p1,&vt2);
  curve1[1].p2 = curve1[1].p1;  curve2[1].p2 = curve2[1].p1;
  point_add (&curve1[1].p2,&vt1);  point_add (&curve2[1].p2,&vt1);
  curve1[1].p3 = curve1[1].p2;  curve2[1].p3 = curve2[1].p2;
  point_add (&curve1[1].p3,&vt1);  point_add (&curve2[1].p3,&vt1);
  point_sub (&curve1[1].p3,&vt2);  point_add (&curve2[1].p3,&vt2);

  dia_renderer_draw_bezier (renderer,curve1,2,col);
  dia_renderer_draw_bezier (renderer,curve2,2,col);
}


static DiaObject *
sadtarrow_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Sadtarrow *sadtarrow;
  OrthConn *orth;
  DiaObject *obj;

  sadtarrow = g_new0 (Sadtarrow, 1);
  orth = &sadtarrow->orth;
  obj = &orth->object;

  obj->type = &sadtarrow_type;
  obj->ops = &sadtarrow_ops;

  orthconn_init(orth, startpoint);

  sadtarrow_update_data(sadtarrow);

  sadtarrow->style = SADT_ARROW_NORMAL; /* sadtarrow_defaults.style; */
  sadtarrow->autogray = TRUE; /* sadtarrow_defaults.autogray; */
  sadtarrow->line_color = color_black;

  *handle1 = orth->handles[0];
  *handle2 = orth->handles[orth->numpoints-2];
  return &sadtarrow->orth.object;
}

static void
sadtarrow_destroy(Sadtarrow *sadtarrow)
{
  orthconn_destroy(&sadtarrow->orth);
}

static void
sadtarrow_update_data(Sadtarrow *sadtarrow)
{
  OrthConn *orth = &sadtarrow->orth;
  PolyBBExtras *extra = &orth->extra_spacing;

  orthconn_update_data(&sadtarrow->orth);

  extra->start_long =
    extra->middle_trans = ARROW_LINE_WIDTH / 2.0;

  extra->end_long = MAX(ARROW_HEAD_LENGTH,ARROW_LINE_WIDTH/2.0);

  extra->start_trans = ARROW_LINE_WIDTH / 2.0;
  extra->end_trans = MAX(ARROW_LINE_WIDTH/2.0,ARROW_HEAD_WIDTH/2.0);

  switch(sadtarrow->style) {
    case SADT_ARROW_IMPORTED:
      extra->start_trans = MAX(ARROW_LINE_WIDTH/2.0,
                              ARROW_PARENS_WOFFSET + ARROW_PARENS_LENGTH/3);
      break;
    case SADT_ARROW_IMPLIED:
      extra->end_trans = MAX(ARROW_LINE_WIDTH/2.0,
                              MAX(ARROW_PARENS_WOFFSET + ARROW_PARENS_LENGTH/3,
                                  ARROW_HEAD_WIDTH/2.0));
      break;
    case SADT_ARROW_DOTTED:
      extra->start_long = extra->end_long;
      extra->end_trans =
        extra->start_trans = MAX(MAX(MAX(ARROW_HEAD_WIDTH,ARROW_HEAD_LENGTH),
                                    ARROW_LINE_WIDTH/2.0),
                                ARROW_DOT_WOFFSET+ARROW_DOT_RADIUS);
      break;
    case SADT_ARROW_NORMAL:
    case SADT_ARROW_DISABLED:
    default:
      break;
  }
  orthconn_update_boundingbox (orth);
}


static DiaObjectChange *
sadtarrow_add_segment_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  DiaObjectChange *change;

  change = orthconn_add_segment ((OrthConn *) obj, clicked);
  sadtarrow_update_data ((Sadtarrow *) obj);

  return change;
}


static DiaObjectChange *
sadtarrow_delete_segment_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  DiaObjectChange *change;

  change = orthconn_delete_segment ((OrthConn *) obj, clicked);
  sadtarrow_update_data ((Sadtarrow *) obj);

  return change;
}


static DiaMenuItem object_menu_items[] = {
  { N_("Add segment"), sadtarrow_add_segment_callback, NULL, 1 },
  { N_("Delete segment"), sadtarrow_delete_segment_callback, NULL, 1 },
  ORTHCONN_COMMON_MENUS,
};

static DiaMenu object_menu = {
  N_("SADT Arrow"),
  sizeof(object_menu_items)/sizeof(DiaMenuItem),
  object_menu_items,
  NULL
};

static DiaMenu *
sadtarrow_get_object_menu(Sadtarrow *sadtarrow, Point *clickedpoint)
{
  OrthConn *orth;

  orth = &sadtarrow->orth;
  /* Set entries sensitive/selected etc here */
  object_menu_items[0].active = orthconn_can_add_segment(orth, clickedpoint);
  object_menu_items[1].active = orthconn_can_delete_segment(orth, clickedpoint);
  orthconn_update_object_menu(orth, clickedpoint, &object_menu_items[2]);
  return &object_menu;
}

static DiaObject *
sadtarrow_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  DiaObject *obj = object_load_using_properties(&sadtarrow_type,
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

