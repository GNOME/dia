/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Objects for drawing KAOS goal diagrams.
 * This class supports the whole goal specialization hierarchy
 * Copyright (C) 2002 Christophe Ponsard
 *
 * Based on SADT box object
 * Copyright (C) 2000, 2001 Cyrille Chepelov
 *
 * Forked from Flowchart toolbox -- objects for drawing flowcharts.
 * Copyright (C) 1999 James Henstridge.
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
#include <string.h>
#include <stdio.h>

#include <glib.h>

#include "object.h"
#include "connection.h"
#include "diarenderer.h"
#include "handle.h"
#include "arrows.h"
#include "properties.h"
#include "dia_image.h"

#include "pixmaps/contributes.xpm"

typedef struct _Mbr Mbr;

typedef enum {
    MBR_CONTRIBUTES,
    MBR_OBSTRUCTS,
    MBR_CONFLICTS,
    MBR_RESP,
    MBR_MONITORS,
    MBR_CONTROLS,
    MBR_CAPABLEOF,
    MBR_PERFORMS,
    MBR_INPUT,
    MBR_OUTPUT
} MbrType;

struct _Mbr {
  Connection connection;

  MbrType type;

  Point pm;
  BezPoint line[3];
  Handle pm_handle;

  double text_width,text_ascent;
};

#define HANDLE_MOVE_MID_POINT (HANDLE_CUSTOM1)

#define MBR_WIDTH 0.1
#define MBR_DASHLEN 0.5
#define MBR_DECFONTHEIGHT 0.7
#define MBR_ARROWLEN 0.8
#define MBR_ARROWWIDTH 0.5

#define MBR_FG_COLOR color_black
#define MBR_BG_COLOR color_white
#define MBR_RED_COLOR color_red

#define MBR_DEC_SIZE 1.0

static Color color_red = { 1.0f, 0.0f, 0.0f, 1.0f };

static DiaFont *mbr_font = NULL;

static DiaObjectChange* mbr_move_handle(Mbr *mbr, Handle *handle,
				   Point *to, ConnectionPoint *cp,
				   HandleMoveReason reason, ModifierKeys modifiers);
static DiaObjectChange* mbr_move(Mbr *mbr, Point *to);
static void mbr_select(Mbr *mbr, Point *clicked_point,
			      DiaRenderer *interactive_renderer);
static void mbr_draw(Mbr *mbr, DiaRenderer *renderer);
static DiaObject *mbr_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real mbr_distance_from(Mbr *mbr, Point *point);
static void mbr_update_data(Mbr *mbr);
static void mbr_destroy(Mbr *mbr);
static DiaObject *mbr_load(ObjectNode obj_node, int version,DiaContext *ctx);

static PropDescription *mbr_describe_props(Mbr *mes);
static void mbr_get_props(Mbr * mbr, GPtrArray *props);
static void mbr_set_props(Mbr *mbr, GPtrArray *props);

static void compute_line(Point* p1, Point* p2, Point *pm, BezPoint* line);

static ObjectTypeOps mbr_type_ops =
{
  (CreateFunc) mbr_create,
  (LoadFunc)   mbr_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType kaos_mbr_type =
{
  "KAOS - mbr",      /* name */
  0,              /* version */
  contributes_xpm, /* pixmap */
  &mbr_type_ops,      /* ops */
  NULL, /* pixmap_file */
  NULL, /* default_user_data */
  NULL, /* prop_descs */
  NULL, /* prop_offsets */
  DIA_OBJECT_HAS_VARIANTS /* flags */
};

static ObjectOps mbr_ops = {
  (DestroyFunc)         mbr_destroy,
  (DrawFunc)            mbr_draw,
  (DistanceFunc)        mbr_distance_from,
  (SelectFunc)          mbr_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            mbr_move,
  (MoveHandleFunc)      mbr_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   mbr_describe_props,
  (GetPropsFunc)        mbr_get_props,
  (SetPropsFunc)        mbr_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropEnumData prop_mbr_type_data[] = {
  { N_("Contributes"), MBR_CONTRIBUTES },
  { N_("Obstructs"),   MBR_OBSTRUCTS },
  { N_("Conflicts"),   MBR_CONFLICTS },
  { N_("Responsibility"),   MBR_RESP },
  { N_("Monitors"),    MBR_MONITORS },
  { N_("Controls"),    MBR_CONTROLS },
  { N_("CapableOf"),   MBR_CAPABLEOF },
  { N_("Performs"),    MBR_PERFORMS },
  { N_("Input"),       MBR_INPUT },
  { N_("Output"),      MBR_OUTPUT },
  { NULL, 0}
};

static PropDescription mbr_props[] = {
  CONNECTION_COMMON_PROPERTIES,
  { "type", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE|PROP_FLAG_NO_DEFAULTS,
    N_("Type:"), NULL, prop_mbr_type_data },
  { "pm", PROP_TYPE_POINT, 0, "pm", NULL, NULL},
  PROP_DESC_END
};

static PropDescription *
mbr_describe_props(Mbr *mbr)
{
  if (mbr_props[0].quark == 0)
    prop_desc_list_calculate_quarks(mbr_props);
  return mbr_props;

}

static PropOffset mbr_offsets[] = {
  CONNECTION_COMMON_PROPERTIES_OFFSETS,
  { "type", PROP_TYPE_ENUM, offsetof(Mbr,type)},
  { "pm", PROP_TYPE_POINT, offsetof(Mbr,pm) },
  { NULL, 0, 0 }
};

static void
mbr_get_props(Mbr * mbr, GPtrArray *props)
{
  object_get_props_from_offsets(&mbr->connection.object,
                                mbr_offsets, props);
}

static void
mbr_set_props(Mbr *mbr, GPtrArray *props)
{
  object_set_props_from_offsets(&mbr->connection.object,
                                mbr_offsets, props);
  mbr_update_data(mbr);
}


static real
mbr_distance_from(Mbr *mbr, Point *point)
{
  return distance_bez_line_point(mbr->line, 3, 2*MBR_WIDTH, point);
}

static void
mbr_select(Mbr *mbr, Point *clicked_point,
	    DiaRenderer *interactive_renderer)
{
  connection_update_handles(&mbr->connection);
}


static DiaObjectChange *
mbr_move_handle (Mbr              *mbr,
                 Handle           *handle,
                 Point            *to,
                 ConnectionPoint  *cp,
                 HandleMoveReason  reason,
                 ModifierKeys      modifiers)
{
  Point p1, p2;
  Point *endpoints;

  g_return_val_if_fail (mbr != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  if (handle->id == HANDLE_MOVE_MID_POINT) {
    mbr->pm = *to;
  } else  {
    endpoints = &mbr->connection.endpoints[0];
    p1.x = 0.5*(endpoints[0].x + endpoints[1].x);
    p1.y = 0.5*(endpoints[0].y + endpoints[1].y);
    connection_move_handle(&mbr->connection, handle->id, to, cp, reason, modifiers);
    p2.x = 0.5*(endpoints[0].x + endpoints[1].x);
    p2.y = 0.5*(endpoints[0].y + endpoints[1].y);
    point_sub(&p2, &p1);
    point_add(&mbr->pm, &p2);
  }

  mbr_update_data(mbr);
  return NULL;
}

static DiaObjectChange*
mbr_move(Mbr *mbr, Point *to)
{
  Point start_to_end;
  Point *endpoints = &mbr->connection.endpoints[0];
  Point delta;

  delta = *to;
  point_sub(&delta, &endpoints[0]);

  start_to_end = endpoints[1];
  point_sub(&start_to_end, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add(&endpoints[1], &start_to_end);

  point_add(&mbr->pm, &delta);

  mbr_update_data(mbr);
  return NULL;
}

static void
compute_line(Point* p1, Point* p2, Point *pm, BezPoint* line) {
  double dx,dy,k;
  double dx1,dy1,k1;
  double dx2,dy2,k2;
  double OFF=1.0;

  dx=p2->x-p1->x;
  dy=p2->y-p1->y;
  k=sqrt(dx*dx+dy*dy);
  if (k!=0) {
    dx=dx/k;
    dy=dy/k;
  } else {
    dx=0;
    dy=1;
  }

  dx1=pm->x-p1->x;
  dy1=pm->y-p1->y;
  k1=sqrt(dx*dx+dy*dy);
  if (k1!=0) {
    dx1=dx1/k;
    dy1=dy1/k;
  } else {
    dx1=0;
    dy1=1;
  }

  dx2=p2->x-pm->x;
  dy2=p2->y-pm->y;
  k2=sqrt(dx*dx+dy*dy);
  if (k2!=0) {
    dx2=dx2/k;
    dy2=dy2/k;
  } else {
    dx2=0;
    dy2=1;
  }

  line[0].type=BEZ_MOVE_TO;
  line[0].p1.x=p1->x;
  line[0].p1.y=p1->y;

  line[1].type=BEZ_CURVE_TO;
  line[1].p3.x=pm->x;
  line[1].p3.y=pm->y;
  line[1].p1.x=p1->x+dx1*OFF;
  line[1].p1.y=p1->y+dy1*OFF;
  line[1].p2.x=pm->x-dx*OFF;
  line[1].p2.y=pm->y-dy*OFF;

  line[2].type=BEZ_CURVE_TO;
  line[2].p3.x=p2->x;
  line[2].p3.y=p2->y;
  line[2].p1.x=pm->x+dx*OFF;
  line[2].p1.y=pm->y+dy*OFF;
  line[2].p2.x=p2->x-dx2*OFF;
  line[2].p2.y=p2->y-dy2*OFF;
}


static char *
compute_text (Mbr *mbr)
{
  char *annot;
  switch (mbr->type) {
    case MBR_RESP:
      annot = g_strdup ("Resp");
      break;
    case MBR_MONITORS:
      annot = g_strdup ("Mon");
      break;
    case MBR_CONTROLS:
      annot = g_strdup ("Ctrl");
      break;
    case MBR_CAPABLEOF:
      annot = g_strdup ("CapOf");
      break;
    case MBR_PERFORMS:
      annot = g_strdup ("Perf");
      break;
    case MBR_INPUT:
      annot = g_strdup ("In");
      break;
    case MBR_OUTPUT:
      annot = g_strdup ("Out");
      break;
    // TODO: Are we sure these are ""
    case MBR_CONTRIBUTES:
    case MBR_OBSTRUCTS:
    case MBR_CONFLICTS:
    default:
      annot = g_strdup ("");
      break;
  }
  return annot;
}


/* drawing here -- TBD inverse flow ??  */
static void
mbr_draw (Mbr *mbr, DiaRenderer *renderer)
{
  Point *endpoints;
  Point p1,p2,pm1,pm2;
  Point pa1,pa2;
  Arrow arrow;
  char *annot;
  double k,dx,dy,dxn,dyn,dxp,dyp;

  /* some asserts */
  g_return_if_fail (mbr != NULL);
  g_return_if_fail (renderer != NULL);

  /* arrow type */
  if (mbr->type!=MBR_CONFLICTS) {
    arrow.type = ARROW_FILLED_TRIANGLE;
  } else {
    arrow.type = ARROW_NONE;
  }
  arrow.length = MBR_ARROWLEN;
  arrow.width = MBR_ARROWWIDTH;

  endpoints = &mbr->connection.endpoints[0];

  /* some computations */
  p1 = endpoints[0];     /* could reverse direction here */
  p2 = endpoints[1];

  /** drawing directed line **/
  dia_renderer_set_linewidth (renderer, MBR_WIDTH);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

  dx=p1.x-p2.x;
  dy=p1.y-p2.y;
  k=sqrt(dx*dx+dy*dy)*2/MBR_DEC_SIZE;

  if (k<0.05) {  /* bug fix for closed bezier */
    dia_renderer_draw_line_with_arrows (renderer,&p1,&p2,MBR_WIDTH,&MBR_FG_COLOR,NULL, &arrow);
  } else {
    dia_renderer_draw_bezier_with_arrows (renderer, mbr->line, 3, MBR_WIDTH, &MBR_FG_COLOR, NULL, &arrow);
  }

  /** drawing vector decoration  **/

  /* red line for obstruction */
  k=k*2/MBR_DEC_SIZE;
  dxn=dx/k;
  dyn=dy/k;
  dxp=-dyn;
  dyp=dxn;

  if (mbr->type==MBR_OBSTRUCTS) {
    pm1.x=mbr->pm.x-dxp;
    pm1.y=mbr->pm.y-dyp;
    pm2.x=mbr->pm.x+dxp;
    pm2.y=mbr->pm.y+dyp;

    dia_renderer_set_linewidth (renderer, MBR_WIDTH*2);
    dia_renderer_draw_line_with_arrows (renderer,&pm1,&pm2,MBR_WIDTH,&MBR_RED_COLOR,NULL,NULL);
  }

  if (mbr->type==MBR_CONFLICTS) {
    pm1.x=mbr->pm.x-dxn-dxp;
    pm1.y=mbr->pm.y-dyn-dyp;
    pm2.x=mbr->pm.x+dxn+dxp;
    pm2.y=mbr->pm.y+dyn+dyp;

    dia_renderer_set_linewidth (renderer, MBR_WIDTH*2);
    dia_renderer_draw_line_with_arrows (renderer,&pm1,&pm2,MBR_WIDTH,&MBR_RED_COLOR,NULL,NULL);

    pm1.x=mbr->pm.x-dxn+dxp;
    pm1.y=mbr->pm.y-dyn+dyp;
    pm2.x=mbr->pm.x+dxn-dxp;
    pm2.y=mbr->pm.y+dyn-dyp;

    dia_renderer_draw_line_with_arrows (renderer,&pm1,&pm2,MBR_WIDTH,&MBR_RED_COLOR,NULL,NULL);
  }


  /** writing decoration text **/
  annot = compute_text (mbr);
  dia_renderer_set_font (renderer, mbr_font, MBR_DECFONTHEIGHT);

  if (annot && strlen (annot) != 0) {
    pa1.x = mbr->pm.x - mbr->text_width / 2;
    pa1.y = mbr->pm.y - mbr->text_ascent + 0.1;  /* with some fix... */
    pa2.x = pa1.x + mbr->text_width;
    pa2.y = pa1.y + MBR_DECFONTHEIGHT + 0.1;  /* with some fix... */
    dia_renderer_draw_rect (renderer, &pa1, &pa2, &color_white, NULL);
    dia_renderer_draw_string (renderer,
                              annot,
                              &mbr->pm,
                              DIA_ALIGN_CENTRE,
                              &MBR_FG_COLOR);
  }

  g_clear_pointer (&annot, g_free);
}

/* creation here */
static DiaObject *
mbr_create(Point *startpoint, void *user_data, Handle **handle1, Handle **handle2)
{
  Mbr *mbr;
  Connection *conn;
  LineBBExtras *extra;
  DiaObject *obj;

  if (mbr_font == NULL) {
    mbr_font = dia_font_new_from_style(DIA_FONT_SANS, MBR_DECFONTHEIGHT);
  }

  mbr = g_new0 (Mbr, 1);

  conn = &mbr->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  conn->endpoints[1].y -= 2;

  obj = &conn->object;
  extra = &conn->extra_spacing;
  switch (GPOINTER_TO_INT(user_data)) {
    case 1: mbr->type=MBR_CONTRIBUTES; break;
    case 2: mbr->type=MBR_OBSTRUCTS; break;
    case 3: mbr->type=MBR_CONFLICTS; break;
    case 4: mbr->type=MBR_RESP; break;
    case 5: mbr->type=MBR_MONITORS; break;
    case 6: mbr->type=MBR_CONTROLS; break;
    case 7: mbr->type=MBR_CAPABLEOF; break;
    case 8: mbr->type=MBR_PERFORMS; break;
    case 9: mbr->type=MBR_INPUT; break;
    case 10: mbr->type=MBR_OUTPUT; break;
    default: mbr->type=MBR_CONTRIBUTES; break;
  }

  obj->type = &kaos_mbr_type;
  obj->ops = &mbr_ops;

  /* connectionpoint init */
  connection_init(conn, 3, 0);

  mbr->text_width = 0.0;
  mbr->text_ascent = 0.0;

  mbr->pm.x=0.5*(conn->endpoints[0].x + conn->endpoints[1].x);
  mbr->pm.y=0.5*(conn->endpoints[0].y + conn->endpoints[1].y);
  mbr->pm_handle.id = HANDLE_MOVE_MID_POINT;
  mbr->pm_handle.type = HANDLE_MINOR_CONTROL;
  mbr->pm_handle.connect_type = HANDLE_NONCONNECTABLE;
  mbr->pm_handle.connected_to = NULL;
  obj->handles[2] = &mbr->pm_handle;

  compute_line(&conn->endpoints[0],&conn->endpoints[1],&mbr->pm,mbr->line);

  extra->start_long =
  extra->start_trans =
  extra->end_long = MBR_WIDTH/2.0;
  extra->end_trans = MAX(MBR_WIDTH,MBR_ARROWLEN)/2.0;

  mbr_update_data(mbr);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];

  return &mbr->connection.object;
}

static void
mbr_destroy(Mbr *mbr)
{
  connection_destroy(&mbr->connection);
}

static void
mbr_update_data(Mbr *mbr)
{
  Connection *conn = &mbr->connection;
  DiaObject *obj = &conn->object;
  DiaRectangle rect;
  Point p1,p2;
  Point p3,p4;
  char *text;

/* Too complex to easily decide -- this is essentially a bezier curve */
/*
  if (connpoint_is_autogap(conn->endpoint_handles[0].connected_to) ||
      connpoint_is_autogap(conn->endpoint_handles[1].connected_to)) {
    connection_adjust_for_autogap(conn);
  }
*/
  obj->position = conn->endpoints[0];

  mbr->pm_handle.pos = mbr->pm;

  connection_update_handles(conn);
  connection_update_boundingbox(conn);

  /* text width */
  text=compute_text(mbr);
  mbr->text_width = dia_font_string_width(text, mbr_font, MBR_DECFONTHEIGHT);
  mbr->text_ascent = dia_font_ascent(text, mbr_font, MBR_DECFONTHEIGHT);

  /* endpoint */
  p1 = conn->endpoints[0];
  p2 = conn->endpoints[1];

 /* bezier */
  compute_line(&p1,&p2,&mbr->pm,mbr->line);

  /* Add boundingbox for mid decoration (slightly overestimated) : */
  p3.x=mbr->pm.x-MBR_DEC_SIZE;
  p3.y=mbr->pm.y-MBR_DEC_SIZE;
  p4.x=p3.x+MBR_DEC_SIZE*2;
  p4.y=p3.y+MBR_DEC_SIZE*2;
  rect.left=p3.x;
  rect.right=p4.x;
  rect.top=p3.y;
  rect.bottom=p4.y;
  rectangle_union(&obj->bounding_box, &rect);

  /* Add boundingbox for text: */
  rect.left = mbr->pm.x-mbr->text_width/2;
  rect.right = rect.left + mbr->text_width;
  rect.top = mbr->pm.y - mbr->text_ascent;
  rect.bottom = rect.top + MBR_DECFONTHEIGHT;
  rectangle_union(&obj->bounding_box, &rect);

  g_clear_pointer (&text, g_free);   /* free auxilliary text */
}

static DiaObject *
mbr_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  return object_load_using_properties(&kaos_mbr_type,
                                      obj_node,version,ctx);
}
