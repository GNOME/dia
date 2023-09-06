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

typedef struct _Maor Maor;

typedef enum {
    MAOR_AND_REF,
    MAOR_AND_COMP_REF,
    MAOR_OR_REF,
    MAOR_OR_COMP_REF,
    MAOR_OPER_REF
} MaorType;

struct _Maor {
  Connection connection;
  ConnectionPoint connector;

  Handle text_handle;

  char *text;
  Point text_pos;
  real text_width;

  MaorType type;
};

#define MAOR_WIDTH 0.1
#define MAOR_DASHLEN 0.5
#define MAOR_FONTHEIGHT 0.7
#define MAOR_ARROWLEN 0.8
#define MAOR_ARROWWIDTH 0.5
#define HANDLE_MOVE_TEXT (HANDLE_CUSTOM1)
#define MAOR_FG_COLOR color_black
#define MAOR_BG_COLOR color_white
#define MAOR_ICON_WIDTH 1.0
#define MAOR_ICON_HEIGHT 1.0
#define MAOR_REF_WIDTH 1.0
#define MAOR_REF_HEIGHT 1.0

static DiaFont *maor_font = NULL;

static DiaObjectChange* maor_move_handle(Maor *maor, Handle *handle,
				   Point *to, ConnectionPoint *cp,
				   HandleMoveReason reason, ModifierKeys modifiers);
static DiaObjectChange* maor_move(Maor *maor, Point *to);
static void maor_select(Maor *maor, Point *clicked_point,
			      DiaRenderer *interactive_renderer);
static void maor_draw(Maor *maor, DiaRenderer *renderer);
static DiaObject *maor_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real maor_distance_from(Maor *maor, Point *point);
static void maor_update_data(Maor *maor);
static void maor_destroy(Maor *maor);
static DiaObject *maor_load(ObjectNode obj_node, int version,DiaContext *ctx);

static PropDescription *maor_describe_props(Maor *mes);
static void maor_get_props(Maor * maor, GPtrArray *props);
static void maor_set_props(Maor *maor, GPtrArray *props);

static void compute_and(Point *p, double w, double h, BezPoint *bpl);
static void compute_or(Point *p, double w, double h, BezPoint *bpl);
static void compute_oper(Point *p, double w, double h, Point *pl);
static void draw_agent_icon(Maor *maor, double w, double h, DiaRenderer *renderer);


static ObjectTypeOps maor_type_ops =
{
  (CreateFunc) maor_create,
  (LoadFunc)   maor_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType kaos_maor_type =
{
  "KAOS - maor",     /* name */
  0,              /* version */
  contributes_xpm, /* pixmap */
  &maor_type_ops,     /* ops */
  NULL, /* pixmap_file */
  NULL, /* default_user_data */
  NULL, /* prop_descs */
  NULL, /* prop_offsets */
  DIA_OBJECT_HAS_VARIANTS /* flags */
};

static ObjectOps maor_ops = {
  (DestroyFunc)         maor_destroy,
  (DrawFunc)            maor_draw,
  (DistanceFunc)        maor_distance_from,
  (SelectFunc)          maor_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            maor_move,
  (MoveHandleFunc)      maor_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   maor_describe_props,
  (GetPropsFunc)        maor_get_props,
  (SetPropsFunc)        maor_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropEnumData prop_maor_type_data[] = {
  { N_("AND Refinement"), MAOR_AND_REF },
  { N_("Complete AND Refinement"), MAOR_AND_REF },
  { N_("OR Refinement"), MAOR_OR_REF },
  { N_("Operationalization"), MAOR_OPER_REF },
  { NULL, 0}
};

static PropDescription maor_props[] = {
  CONNECTION_COMMON_PROPERTIES,
  { "text", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Text:"), NULL, NULL },

  { "type", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE|PROP_FLAG_NO_DEFAULTS,
    N_("Type:"), NULL, prop_maor_type_data },

  { "text_pos", PROP_TYPE_POINT, 0,
    "text_pos:", NULL,NULL },
  PROP_DESC_END
};

static PropDescription *
maor_describe_props(Maor *maor)
{
  if (maor_props[0].quark == 0)
    prop_desc_list_calculate_quarks(maor_props);
  return maor_props;

}

static PropOffset maor_offsets[] = {
  CONNECTION_COMMON_PROPERTIES_OFFSETS,
  { "text", PROP_TYPE_STRING, offsetof(Maor, text) },
  { "type", PROP_TYPE_ENUM, offsetof(Maor,type)},
  { "text_pos", PROP_TYPE_POINT, offsetof(Maor,text_pos) },
  { NULL, 0, 0 }
};

static void
maor_get_props(Maor * maor, GPtrArray *props)
{
  object_get_props_from_offsets(&maor->connection.object,maor_offsets, props);
}

static void
maor_set_props(Maor *maor, GPtrArray *props)
{
  object_set_props_from_offsets(&maor->connection.object,maor_offsets, props);
  maor_update_data(maor);
}


static real
maor_distance_from(Maor *maor, Point *point)
{
  Point *endpoints;
  real d1,d2;

  endpoints = &maor->connection.endpoints[0];
  d1 = distance_line_point(&endpoints[0], &endpoints[1], MAOR_WIDTH, point);
  d2 = distance_point_point(&endpoints[0], point) - MAOR_REF_WIDTH/2.0;
  if (d2<0) d2=0;

  if (d1<d2) return d1; else return d2;
}

static void
maor_select(Maor *maor, Point *clicked_point,
	    DiaRenderer *interactive_renderer)
{
  connection_update_handles(&maor->connection);
}


static DiaObjectChange *
maor_move_handle (Maor             *maor,
                  Handle           *handle,
                  Point            *to,
                  ConnectionPoint  *cp,
                  HandleMoveReason  reason,
                  ModifierKeys      modifiers)
{
  Point p1, p2;
  Point *endpoints;

  g_return_val_if_fail (maor != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  if (handle->id == HANDLE_MOVE_TEXT) {
    maor->text_pos = *to;
  } else  {
    endpoints = &maor->connection.endpoints[0];
    p1.x = 0.5*(endpoints[0].x + endpoints[1].x);
    p1.y = 0.5*(endpoints[0].y + endpoints[1].y);
    connection_move_handle(&maor->connection, handle->id, to, cp, reason, modifiers);
    connection_adjust_for_autogap(&maor->connection);
    p2.x = 0.5*(endpoints[0].x + endpoints[1].x);
    p2.y = 0.5*(endpoints[0].y + endpoints[1].y);
    point_sub(&p2, &p1);
    point_add(&maor->text_pos, &p2);
  }

  maor_update_data(maor);
  return NULL;
}

static DiaObjectChange*
maor_move(Maor *maor, Point *to)
{
  Point start_to_end;
  Point *endpoints = &maor->connection.endpoints[0];
  Point delta;

  delta = *to;
  point_sub(&delta, &endpoints[0]);

  start_to_end = endpoints[1];
  point_sub(&start_to_end, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add(&endpoints[1], &start_to_end);

  point_add(&maor->text_pos, &delta);

  maor_update_data(maor);
  return NULL;
}

static void compute_and(Point *p, double w, double h, BezPoint *bpl) {
     Point ref;
     ref.x=p->x-w/2;
     ref.y=p->y-h/2;

     bpl[0].type=BEZ_MOVE_TO;
     bpl[0].p1.x=ref.x;
     bpl[0].p1.y=ref.y+h;

     bpl[1].type=BEZ_LINE_TO;
     bpl[1].p1.x=ref.x+w/20;
     bpl[1].p1.y=ref.y+h/2;

     bpl[2].type=BEZ_CURVE_TO;
     bpl[2].p3.x=ref.x+w/2;
     bpl[2].p3.y=ref.y;
     bpl[2].p1.x=bpl[1].p1.x+w*0.15;
     bpl[2].p1.y=bpl[1].p1.y-h/2;
     bpl[2].p2.x=bpl[2].p3.x-w/4;
     bpl[2].p2.y=bpl[2].p3.y;

     bpl[3].type=BEZ_CURVE_TO;
     bpl[3].p3.x=ref.x+w*19/20;
     bpl[3].p3.y=ref.y+h/2;
     bpl[3].p1.x=bpl[2].p3.x+w/4;
     bpl[3].p1.y=bpl[2].p3.y;
     bpl[3].p2.x=bpl[3].p3.x-w*0.15;
     bpl[3].p2.y=bpl[3].p3.y-h/2;

     bpl[4].type=BEZ_LINE_TO;
     bpl[4].p1.x=ref.x+w;
     bpl[4].p1.y=ref.y+h;

     bpl[5].type=BEZ_LINE_TO;
     bpl[5].p1.x=ref.x;
     bpl[5].p1.y=ref.y+h;
}

static void compute_or(Point *p, double w, double h, BezPoint *bpl) {
     Point ref;
     ref.x=p->x-w/2;
     ref.y=p->y-h/2;

     bpl[0].type=BEZ_MOVE_TO;
     bpl[0].p1.x=ref.x;
     bpl[0].p1.y=ref.y+h;

     bpl[1].type=BEZ_CURVE_TO;
     bpl[1].p3.x=ref.x+w/2;
     bpl[1].p3.y=ref.y;
     bpl[1].p1.x=bpl[0].p1.x;
     bpl[1].p1.y=bpl[0].p1.y-h/2;
     bpl[1].p2.x=bpl[1].p3.x-w/2;
     bpl[1].p2.y=bpl[1].p3.y+h/4;

     bpl[2].type=BEZ_CURVE_TO;
     bpl[2].p3.x=ref.x+w;
     bpl[2].p3.y=ref.y+h;
     bpl[2].p1.x=bpl[1].p3.x+w/2;
     bpl[2].p1.y=bpl[1].p3.y+h/4;
     bpl[2].p2.x=bpl[2].p3.x;
     bpl[2].p2.y=bpl[2].p3.y-w/2;

     bpl[3].type=BEZ_CURVE_TO;
     bpl[3].p3.x=ref.x;
     bpl[3].p3.y=ref.y+h;
     bpl[3].p1.x=bpl[2].p3.x-w/2;
     bpl[3].p1.y=bpl[2].p3.y-h/4;
     bpl[3].p2.x=bpl[3].p3.x+w/2;
     bpl[3].p2.y=bpl[3].p3.y-h/4;
}

static void compute_oper(Point *p, double w, double h, Point *pl) {
     Point ref;
     double wd=w/2;
     double hd=h/2;
     double s30=sin(M_PI/6)*wd;
     double c30=cos(M_PI/6)*hd;

     ref.x=p->x;
     ref.y=p->y;

     pl[0].x=ref.x;
     pl[0].y=ref.y-hd;
     pl[1].x=ref.x+c30;
     pl[1].y=ref.y-s30;
     pl[2].x=ref.x+c30;
     pl[2].y=ref.y+s30;
     pl[3].x=ref.x;
     pl[3].y=ref.y+hd;
     pl[4].x=ref.x-c30;
     pl[4].y=ref.y+s30;
     pl[5].x=ref.x-c30;
     pl[5].y=ref.y-s30;
     pl[6].x=pl[0].x;
     pl[6].y=pl[0].y;
}

static void
draw_agent_icon (Maor        *maor,
                 double       w,
                 double       h,
                 DiaRenderer *renderer)
{
  double rx,ry;
  Point ref,c,p1,p2;

  ref=maor->connection.endpoints[0];
  rx=ref.x;
  ry=ref.y-h*0.2;

  /* head */
  c.x=rx;
  c.y=ry;
  dia_renderer_draw_ellipse (renderer,&c,h/5,h/5,&MAOR_FG_COLOR, NULL);

  /* body */
  p1.x=rx;
  p1.y=ry;
  p2.x=p1.x;
  p2.y=p1.y+3.5*h/10;
  dia_renderer_draw_line (renderer,&p1,&p2,&MAOR_FG_COLOR);

  /* arms */
  p1.x=rx-1.5*h/10;
  p1.y=ry+2.2*h/10;
  p2.x=rx+1.5*h/10;
  p2.y=p1.y;
  dia_renderer_draw_line (renderer,&p1,&p2,&MAOR_FG_COLOR);

  /* left leg */
  p1.x=rx;
  p1.y=ry+3.5*h/10;
  p2.x=p1.x-h/10;
  p2.y=p1.y+2*h/10;
  dia_renderer_draw_line (renderer,&p1,&p2,&MAOR_FG_COLOR);

  /* right leg */
  p1.x=rx;
  p1.y=ry+3.5*h/10;
  p2.x=p1.x+h/10;
  p2.y=p1.y+2*h/10;
  dia_renderer_draw_line (renderer,&p1,&p2,&MAOR_FG_COLOR);
}


/* drawing here -- TBD inverse flow ??  */
static void
maor_draw (Maor *maor, DiaRenderer *renderer)
{
  Point *endpoints, p1, p2;
  Arrow arrow;
  BezPoint bpl[6];
  Point pl[7];
  char *mname = g_strdup (maor->text);

  /* some asserts */
  g_return_if_fail (maor != NULL);
  g_return_if_fail (renderer != NULL);

  /* arrow type */
  arrow.type = ARROW_FILLED_TRIANGLE;
  arrow.length = MAOR_ARROWLEN;
  arrow.width = MAOR_ARROWWIDTH;

  endpoints = &maor->connection.endpoints[0];

  /* some computations */
  p1 = endpoints[0];     /* could reverse direction here */
  p2 = endpoints[1];

  /** drawing directed line **/
  dia_renderer_set_linewidth (renderer, MAOR_WIDTH);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0);
  dia_renderer_draw_line_with_arrows (renderer,
                                      &p1,
                                      &p2,
                                      MAOR_WIDTH,
                                      &MAOR_FG_COLOR,
                                      NULL,
                                      &arrow);

  /** drawing vector decoration  **/
  /* and ref */
  switch (maor->type) {
    case MAOR_AND_REF:
      compute_and (&p1,MAOR_REF_WIDTH,MAOR_REF_HEIGHT,bpl);
      dia_renderer_draw_beziergon (renderer,bpl,6,&MAOR_BG_COLOR,&MAOR_FG_COLOR);
      break;

    case MAOR_AND_COMP_REF:
      compute_and (&p1,MAOR_REF_WIDTH,MAOR_REF_HEIGHT,bpl);
      dia_renderer_draw_beziergon (renderer,bpl,6,&MAOR_FG_COLOR,NULL);
      break;

    case MAOR_OR_REF:
      compute_or (&p1,MAOR_REF_WIDTH,MAOR_REF_HEIGHT,bpl);
      dia_renderer_draw_beziergon (renderer,bpl,4,&MAOR_BG_COLOR,&MAOR_FG_COLOR);
      break;

    case MAOR_OR_COMP_REF:
      compute_or (&p1,MAOR_REF_WIDTH,MAOR_REF_HEIGHT,bpl);
      dia_renderer_draw_beziergon (renderer,bpl,4,&MAOR_FG_COLOR,NULL);
      break;

    case MAOR_OPER_REF:
      compute_oper (&p1,MAOR_REF_WIDTH,MAOR_REF_HEIGHT,pl);
      dia_renderer_draw_polygon (renderer,pl,7,&MAOR_BG_COLOR,&MAOR_FG_COLOR);
      draw_agent_icon (maor,MAOR_REF_WIDTH,MAOR_REF_HEIGHT,renderer);
      break;

    default:
      g_return_if_reached ();
  }

  /** writing text on arrow (maybe not a good idea) **/
  dia_renderer_set_font (renderer, maor_font, MAOR_FONTHEIGHT);

  if (mname && strlen (mname) != 0) {
    dia_renderer_draw_string (renderer,
                              mname,
                              &maor->text_pos,
                              DIA_ALIGN_CENTRE,
                              &color_black);
  }

  g_clear_pointer (&mname, g_free);
}


/* creation here */
static DiaObject *
maor_create (Point   *startpoint,
             void    *user_data,
             Handle **handle1,
             Handle **handle2)
{
  Maor *maor;
  Connection *conn;
  LineBBExtras *extra;
  DiaObject *obj;

  if (maor_font == NULL) {
    maor_font =
      dia_font_new_from_style (DIA_FONT_SANS, MAOR_FONTHEIGHT);
  }

  maor = g_new0 (Maor, 1);

  conn = &maor->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  conn->endpoints[1].y -= 2;

  obj = &conn->object;
  extra = &conn->extra_spacing;

  switch (GPOINTER_TO_INT(user_data)) {
    case 1: maor->type=MAOR_AND_REF; break;
    case 2: maor->type=MAOR_AND_COMP_REF; break;
    case 3: maor->type=MAOR_OR_REF; break;
    case 4: maor->type=MAOR_OR_COMP_REF; break;
    case 5: maor->type=MAOR_OPER_REF; break;
    default: maor->type=MAOR_AND_REF; break;
  }

  obj->type = &kaos_maor_type;
  obj->ops = &maor_ops;

  /* connectionpoint init */
  connection_init(conn, 3, 1);      /* added one connection point (3rd handle for text move) */
  obj->connections[0] = &maor->connector;
  maor->connector.object = obj;
  maor->connector.connected = NULL;

  maor->text = g_strdup("");
  maor->text_width = 0.0;
  maor->text_pos.x = 0.5*(conn->endpoints[0].x + conn->endpoints[1].x);
  maor->text_pos.y = 0.5*(conn->endpoints[0].y + conn->endpoints[1].y);

  maor->text_handle.id = HANDLE_MOVE_TEXT;
  maor->text_handle.type = HANDLE_MINOR_CONTROL;
  maor->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  maor->text_handle.connected_to = NULL;
  obj->handles[2] = &maor->text_handle;

  extra->start_long =
  extra->start_trans =
  extra->end_long = MAOR_WIDTH/2.0;
  extra->end_trans = MAX(MAOR_WIDTH,MAOR_ARROWLEN)/2.0;

  maor_update_data(maor);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];

  return &maor->connection.object;
}

static void
maor_destroy(Maor *maor)
{
  connection_destroy(&maor->connection);

  g_clear_pointer (&maor->text, g_free);
}

static void
maor_update_data(Maor *maor)
{
  Connection *conn = &maor->connection;
  DiaObject *obj = &conn->object;
  DiaRectangle rect;
  Point p1,p2,p3,p4;

  if (connpoint_is_autogap(conn->endpoint_handles[0].connected_to) ||
      connpoint_is_autogap(conn->endpoint_handles[1].connected_to)) {
    connection_adjust_for_autogap(conn);
  }
  obj->position = conn->endpoints[0];

  maor->text_handle.pos = maor->text_pos;

  connection_update_handles(conn);
  connection_update_boundingbox(conn);

  maor->text_width =
    dia_font_string_width(maor->text, maor_font, MAOR_FONTHEIGHT);

  /* endpoint */
  p1 = conn->endpoints[0];
  p2 = conn->endpoints[1];

  /* connection point */
  maor->connector.pos.x=p1.x;
  maor->connector.pos.y=p1.y+MAOR_ICON_HEIGHT/2;

  /* Add boundingbox for mid image: */
  p3.x=(p1.x+p2.x)/2.0-MAOR_ICON_WIDTH/2;
  p3.y=(p1.y+p2.y)/2.0-MAOR_ICON_HEIGHT/2;
  p4.x=p3.x+MAOR_ICON_WIDTH;
  p4.y=p3.y+MAOR_ICON_HEIGHT;
  rect.left=p3.x;
  rect.right=p4.x;
  rect.top=p3.y;
  rect.bottom=p4.y;
  rectangle_union(&obj->bounding_box, &rect);

  /* Add boundingbox for end image: */
  p3.x=p1.x-MAOR_REF_WIDTH*1.1/2;    /* 1.1 factor to be safe (fix for or) */
  p3.y=p1.y-MAOR_REF_HEIGHT*1.1/2;
  p4.x=p3.x+MAOR_REF_WIDTH*1.1;
  p4.y=p3.y+MAOR_REF_HEIGHT*1.1;
  rect.left=p3.x;
  rect.right=p4.x;
  rect.top=p3.y;
  rect.bottom=p4.y;
  rectangle_union(&obj->bounding_box, &rect);

  /* Add boundingbox for text: */
  rect.left = maor->text_pos.x-maor->text_width/2;
  rect.right = rect.left + maor->text_width;
  rect.top = maor->text_pos.y - dia_font_ascent(maor->text, maor_font, MAOR_FONTHEIGHT);
  rect.bottom = rect.top + MAOR_FONTHEIGHT;
  rectangle_union(&obj->bounding_box, &rect);
}

static DiaObject *
maor_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  return object_load_using_properties(&kaos_maor_type,
                                      obj_node,version,ctx);
}
