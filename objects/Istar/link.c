/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * Objects for drawing i* diagrams
 * This class supports all i* links as decoraterd bezier curves
 * Copyright (C) 2002-2003 Christophe Ponsard
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

#include "pixmaps/link.xpm"  /* generic "unspecified" link */

typedef struct _Link Link;

typedef enum {
   UNSPECIFIED,
   POS_CONTRIB,
   NEG_CONTRIB,
   DEPENDENCY,
   DECOMPOSITION,
   MEANS_ENDS
} LinkType;

struct _Link {
  Connection connection;
  ConnectionPoint connector;

  LinkType type;

  Point pm;
  BezPoint line[3];
  Handle pm_handle;
};

#define HANDLE_MOVE_MID_POINT (HANDLE_CUSTOM1)

#define LINK_WIDTH 0.12
#define LINK_DASHLEN 0.5
#define LINK_FONTHEIGHT 0.7
#define LINK_ARROWLEN 0.8
#define LINK_ARROWWIDTH 0.5

#define LINK_FG_COLOR color_black
#define LINK_BG_COLOR color_white
#define LINK_DEP_WIDTH 0.8
#define LINK_DEP_HEIGHT 0.6
#define LINK_REF_WIDTH 1.0
#define LINK_REF_HEIGHT 1.0

static DiaFont *link_font = NULL;

static DiaObjectChange* link_move_handle(Link *link, Handle *handle,
				   Point *to, ConnectionPoint *cp,
				   HandleMoveReason reason, ModifierKeys modifiers);
static DiaObjectChange* link_move(Link *link, Point *to);
static void link_select(Link *link, Point *clicked_point,
			      DiaRenderer *interactive_renderer);
static void link_draw(Link *link, DiaRenderer *renderer);
static DiaObject *link_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real link_distance_from(Link *link, Point *point);
static void link_update_data(Link *link);
static void link_destroy(Link *link);
static DiaObject *link_load(ObjectNode obj_node, int version,DiaContext *ctx);

static PropDescription *link_describe_props(Link *mes);
static void link_get_props(Link * link, GPtrArray *props);
static void link_set_props(Link *link, GPtrArray *props);


static Point compute_annot(Point* p1, Point* p2, Point* pm, double f, double d);
static void compute_dependency(BezPoint *line, BezPoint *bpl);
static void compute_line(Point* p1, Point* p2, Point *pm, BezPoint* line);


static ObjectTypeOps link_type_ops =
{
  (CreateFunc) link_create,
  (LoadFunc)   link_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType istar_link_type =
{
  "Istar - link",    /* name */
  0,              /* version */
  istar_link_xpm,  /* pixmap */
  &link_type_ops,     /* ops */
  NULL, /* pixmap_file */
  NULL, /* default_user_data */
  NULL, /* prop_descs */
  NULL, /* prop_offsets */
  DIA_OBJECT_HAS_VARIANTS /* flags */
};


static ObjectOps link_ops = {
  (DestroyFunc)         link_destroy,
  (DrawFunc)            link_draw,
  (DistanceFunc)        link_distance_from,
  (SelectFunc)          link_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            link_move,
  (MoveHandleFunc)      link_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   link_describe_props,
  (GetPropsFunc)        link_get_props,
  (SetPropsFunc)        link_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropEnumData prop_link_type_data[] = {
  { N_("Unspecified"),      UNSPECIFIED },
  { N_("Positive contrib"), POS_CONTRIB },
  { N_("Negative contrib"), NEG_CONTRIB },
  { N_("Dependency"),       DEPENDENCY },
  { N_("Decomposition"),    DECOMPOSITION },
  { N_("Means-Ends"),       MEANS_ENDS },
  { NULL, 0}
};

static PropDescription link_props[] = {
  CONNECTION_COMMON_PROPERTIES,
  { "type", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE|PROP_FLAG_NO_DEFAULTS,
    N_("Type:"), NULL, prop_link_type_data },
  { "pm", PROP_TYPE_POINT, 0, "pm", NULL, NULL},
  PROP_DESC_END
};

static PropDescription *
link_describe_props(Link *link)
{
  if (link_props[0].quark == 0)
    prop_desc_list_calculate_quarks(link_props);
  return link_props;

}

static PropOffset link_offsets[] = {
  CONNECTION_COMMON_PROPERTIES_OFFSETS,
  { "type", PROP_TYPE_ENUM, offsetof(Link,type)},
  { "pm", PROP_TYPE_POINT, offsetof(Link,pm) },
  { NULL, 0, 0 }
};

static void
link_get_props(Link * link, GPtrArray *props)
{
  object_get_props_from_offsets(&link->connection.object,
                                link_offsets, props);
}

static void
link_set_props(Link *link, GPtrArray *props)
{
  object_set_props_from_offsets(&link->connection.object,
                                link_offsets, props);
  link_update_data(link);
}


static real
link_distance_from(Link *link, Point *point)
{
  return distance_bez_line_point(link->line, 3, LINK_WIDTH, point);
}

static void
link_select(Link *link, Point *clicked_point,
	    DiaRenderer *interactive_renderer)
{
  connection_update_handles(&link->connection);
}


static DiaObjectChange *
link_move_handle (Link             *link,
                  Handle           *handle,
                  Point            *to,
                  ConnectionPoint  *cp,
                  HandleMoveReason  reason,
                  ModifierKeys      modifiers)
{
  Point p1, p2;
  Point *endpoints;

  g_return_val_if_fail (link != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  if (handle->id == HANDLE_MOVE_MID_POINT) {
    link->pm = *to;
  } else  {
    endpoints = &link->connection.endpoints[0];
    p1.x = 0.5*(endpoints[0].x + endpoints[1].x);
    p1.y = 0.5*(endpoints[0].y + endpoints[1].y);
    connection_move_handle(&link->connection, handle->id, to, cp, reason, modifiers);
    p2.x = 0.5*(endpoints[0].x + endpoints[1].x);
    p2.y = 0.5*(endpoints[0].y + endpoints[1].y);
    point_sub(&p2, &p1);
    point_add(&link->pm, &p2);
  }

  link_update_data(link);
  return NULL;
}

static DiaObjectChange*
link_move(Link *link, Point *to)
{
  Point start_to_end;
  Point *endpoints = &link->connection.endpoints[0];
  Point delta;

  delta = *to;
  point_sub(&delta, &endpoints[0]);

  start_to_end = endpoints[1];
  point_sub(&start_to_end, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add(&endpoints[1], &start_to_end);

  point_add(&link->pm, &delta);

  link_update_data(link);
  return NULL;
}

static Point
bezier_line_eval(BezPoint *line,int p,real u)
{
  real bx[4],by[4];
  Point res;

  bx[0]=line[p-1].p3.x;
  bx[1]=line[p].p1.x;
  bx[2]=line[p].p2.x;
  bx[3]=line[p].p3.x;

  by[0]=line[p-1].p3.y;
  by[1]=line[p].p1.y;
  by[2]=line[p].p2.y;
  by[3]=line[p].p3.y;

  res.x=bezier_eval(bx,u);
  res.y=bezier_eval(by,u);

  return res;
}

/*
  p1, p2, pm are start, end medium point
  f is fraction <0.5 for between p1 and pm, >05. for between pm and p2
  d is lateral offset
  cx,cy are text width/height
*/
static Point
compute_annot(Point* p1, Point* p2, Point* pm, double f, double d)
{
  Point res;
  double dx,dy,k;

  /* computing position */
  if (f<0.5) {
    f=f*2;
    dx=pm->x - p1->x;
    dy=pm->y - p1->y;
    res.x=p1->x+dx*f;
    res.y=p1->y+dy*f;
  } else {
    f=(f-0.5)*2;
    dx=p2->x - pm->x;
    dy=p2->y - pm->y;
    res.x=pm->x+dx*f;
    res.y=pm->y+dy*f;
  }

  /* some orthogonal shift */
  k=sqrt(dx*dx+dy*dy);
  if (k!=0) {
    res.x+=dy/k*d;
    res.y-=dx/k*d;
  }

  /* centering text */
  res.y+=0.25;

  return res;
}

/* compute bezier for Dependency */
static void compute_dependency(BezPoint *line, BezPoint *bpl) {
  Point ref;
  double dx,dy,dxp,dyp,k;
  real bx[4],by[4];

  /* computing anchor point and tangent */
  bx[0]=line[1].p3.x;
  bx[1]=line[2].p1.x;
  bx[2]=line[2].p2.x;
  bx[3]=line[2].p3.x;

  by[0]=line[1].p3.y;
  by[1]=line[2].p1.y;
  by[2]=line[2].p2.y;
  by[3]=line[2].p3.y;

  ref.x=bezier_eval(bx,0.25);
  ref.y=bezier_eval(by,0.25);
  dx=bezier_eval_tangent(bx,0.25);
  dy=bezier_eval_tangent(by,0.25);
  k=sqrt(dx*dx+dy*dy);

  /* normalizing */
  if (k!=0) {
    dx=dx/k;
    dy=dy/k;
    dxp=dy;
    dyp=-dx;
  } else {
    dx=0;
    dy=1;
    dxp=1;
    dyp=0;
  }

  /*  more offset for origin */
  ref.x -= LINK_DEP_HEIGHT * dx;
  ref.y -= LINK_DEP_HEIGHT * dy;

  /* bezier */
  bpl[0].type=BEZ_MOVE_TO;
  bpl[0].p1.x = ref.x + LINK_DEP_WIDTH/2.0 * dxp;
  bpl[0].p1.y = ref.y + LINK_DEP_WIDTH/2.0 * dyp;

  bpl[1].type=BEZ_CURVE_TO;
  bpl[1].p3.x = ref.x + LINK_DEP_HEIGHT * dx;
  bpl[1].p3.y = ref.y + LINK_DEP_HEIGHT * dy;
  bpl[1].p1.x = ref.x + LINK_DEP_WIDTH/2.0 * dxp + LINK_DEP_HEIGHT * dx;
  bpl[1].p1.y = ref.y + LINK_DEP_WIDTH/2.0 * dyp + LINK_DEP_HEIGHT * dy;
  bpl[1].p2.x = bpl[1].p1.x;
  bpl[1].p2.y = bpl[1].p1.y;

  bpl[2].type=BEZ_CURVE_TO;
  bpl[2].p3.x = ref.x - LINK_DEP_WIDTH/2.0 * dxp;
  bpl[2].p3.y = ref.y - LINK_DEP_WIDTH/2.0 * dyp;
  bpl[2].p1.x = ref.x - LINK_DEP_WIDTH/2.0 * dxp + LINK_DEP_HEIGHT * dx;
  bpl[2].p1.y = ref.y - LINK_DEP_WIDTH/2.0 * dyp + LINK_DEP_HEIGHT * dy;
  bpl[2].p2.x = bpl[2].p1.x;
  bpl[2].p2.y = bpl[2].p1.y;

  bpl[3].type=BEZ_LINE_TO;
  bpl[3].p1.x=bpl[0].p1.x;
  bpl[3].p1.y=bpl[0].p1.y;
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


/* drawing here -- TBD inverse flow ??  */
static void
link_draw (Link *link, DiaRenderer *renderer)
{
  Point *endpoints, p1, p2, pa;
  Arrow arrow;
  char *annot;
  double w;
  BezPoint bpl[4];

  /* some asserts */
  g_return_if_fail (link != NULL);
  g_return_if_fail (renderer != NULL);

  /* some point computations */
  endpoints = &link->connection.endpoints[0];
  p1 = endpoints[0];     /* could reverse direction here */
  p2 = endpoints[1];
  pa = compute_annot(&p1,&p2,&link->pm,0.75,0.75);

  /** computing properties **/
  w=LINK_WIDTH;
  annot=NULL;
  arrow.type = ARROW_FILLED_TRIANGLE;
  arrow.length = LINK_ARROWLEN;
  arrow.width = LINK_ARROWWIDTH;

  switch (link->type) {
    case POS_CONTRIB:
      w=1.5*LINK_WIDTH;
      annot = g_strdup("+");
      break;
    case NEG_CONTRIB:
      w=1.5*LINK_WIDTH;
      annot = g_strdup("-");
      break;
    case DEPENDENCY:
      annot = g_strdup("");
      break;
    case DECOMPOSITION:
      arrow.type = ARROW_CROSS;
      annot = g_strdup("");
      break;
    case MEANS_ENDS:
      arrow.type = ARROW_LINES;
      annot = g_strdup("");
      break;
    case UNSPECIFIED: /* use above defaults */
      annot = g_strdup("");
      break;
    default:
      g_return_if_reached ();
  }

  /** drawing line **/
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linewidth (renderer, w);
  dia_renderer_draw_bezier_with_arrows (renderer,
                                        link->line,
                                        3,
                                        w,
                                        &LINK_FG_COLOR,
                                        NULL,
                                        &arrow);

  /** drawing decoration **/
  dia_renderer_set_font (renderer, link_font, LINK_FONTHEIGHT);
  if ((annot != NULL) && strlen (annot) != 0) {
    dia_renderer_draw_string (renderer,
                              annot,
                              &pa,
                              DIA_ALIGN_CENTRE,
                              &color_black);
  }
  g_clear_pointer (&annot, g_free);

  /** special stuff for dependency **/
  if (link->type == DEPENDENCY) {
    compute_dependency (link->line,bpl);
    dia_renderer_draw_bezier (renderer, bpl, 4, &LINK_FG_COLOR);
  }
}

/* creation here */
static DiaObject *
link_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Link *link;
  Connection *conn;
  LineBBExtras *extra;
  DiaObject *obj;

  if (link_font == NULL) {
    link_font = dia_font_new_from_style(DIA_FONT_SANS, LINK_FONTHEIGHT);
  }

  link = g_new0 (Link, 1);

  conn = &link->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  conn->endpoints[1].y -= 2;

  obj = &conn->object;
  extra = &conn->extra_spacing;

  switch (GPOINTER_TO_INT(user_data)) {
    case 1: link->type=UNSPECIFIED; break;
    case 2: link->type=POS_CONTRIB; break;
    case 3: link->type=NEG_CONTRIB; break;
    case 4: link->type=DEPENDENCY;  break;
    case 5: link->type=DECOMPOSITION; break;
    case 6: link->type=MEANS_ENDS;  break;
    default: link->type=UNSPECIFIED; break;
  }

  obj->type = &istar_link_type;
  obj->ops = &link_ops;

  /* connectionpoint init */
  connection_init(conn, 3, 0);   /* with mid-control */

  link->pm.x=0.5*(conn->endpoints[0].x + conn->endpoints[1].x);
  link->pm.y=0.5*(conn->endpoints[0].y + conn->endpoints[1].y);
  link->pm_handle.id = HANDLE_MOVE_MID_POINT;
  link->pm_handle.type = HANDLE_MINOR_CONTROL;
  link->pm_handle.connect_type = HANDLE_NONCONNECTABLE;
  link->pm_handle.connected_to = NULL;
  obj->handles[2] = &link->pm_handle;

  compute_line(&conn->endpoints[0],&conn->endpoints[1],&link->pm,link->line);

  extra->start_long =
  extra->start_trans =
  extra->end_long = LINK_WIDTH/2.0;
  extra->end_trans = MAX(LINK_WIDTH,LINK_ARROWLEN)/2.0;

  link_update_data(link);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];

  return &link->connection.object;
}

static void
link_destroy(Link *link)
{
  connection_destroy(&link->connection);
}

static void
link_update_data(Link *link)
{
  Connection *conn = &link->connection;
  DiaObject *obj = &conn->object;
  DiaRectangle rect;
  Point p1,p2,p3,p4,pa;

/* Too complex to easily decide */
/*
  if (connpoint_is_autogap(conn->endpoint_handles[0].connected_to) ||
      connpoint_is_autogap(conn->endpoint_handles[1].connected_to)) {
    connection_adjust_for_autogap(conn);
  }
*/
  obj->position = conn->endpoints[0];

  link->pm_handle.pos = link->pm;

  connection_update_handles(conn);
  connection_update_boundingbox(conn);

  /* endpoint */
  p1 = conn->endpoints[0];
  p2 = conn->endpoints[1];

  /* bezier */
  compute_line(&p1,&p2,&link->pm,link->line);

  /* connection point */
  link->connector.pos.x=p1.x;
  link->connector.pos.y=p1.y;

  /* Update boundingbox for mid-point (TBD is this necessary ?) */
  rectangle_add_point(&obj->bounding_box, &link->pm);

  /* Add boundingbox for annotation text (over-estimated) : */
  pa=compute_annot(&p1,&p2,&link->pm,0.75,0.75);
  rect.left = pa.x-0.3;
  rect.right = rect.left+0.6;
  rect.top = pa.y - LINK_FONTHEIGHT;
  rect.bottom = rect.top + 2*LINK_FONTHEIGHT;
  rectangle_union(&obj->bounding_box, &rect);

  /* Add boundingbox for dependency decoration (with some overestimation toi be safe) */
  pa=bezier_line_eval(link->line,2,0.25);
  p3.x=pa.x-LINK_DEP_WIDTH*1.5;
  p3.y=pa.y-LINK_DEP_HEIGHT*1.5;
  p4.x=p3.x+LINK_DEP_WIDTH*3;
  p4.y=p3.y+LINK_DEP_HEIGHT*3;
  rect.left=p3.x;
  rect.right=p4.x;
  rect.top=p3.y;
  rect.bottom=p4.y;
  rectangle_union(&obj->bounding_box, &rect);
}

static DiaObject *
link_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  return object_load_using_properties(&istar_link_type,obj_node,version,ctx);
}
