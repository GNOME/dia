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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <math.h>

#include "intl.h"
#include "object.h"
#include "neworth_conn.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "widgets.h"
#include "message.h"
#include "color.h"
#include "properties.h"

#include "sadt.h"

#include "pixmaps/arrow.xpm"


#define ARROW_CORNER_RADIUS .75
#define ARROW_LINE_WIDTH 0.10
#define ARROW_HEAD_LENGTH .8
#define ARROW_HEAD_WIDTH .8
#define ARROW_HEAD_TYPE ARROW_FILLED_TRIANGLE
#define ARROW_COLOR color_black
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
  NewOrthConn orth;

  Sadtarrow_style style;
  gboolean autogray;
} Sadtarrow;

static void sadtarrow_move_handle(Sadtarrow *sadtarrow, Handle *handle,
				   Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void sadtarrow_move(Sadtarrow *sadtarrow, Point *to);
static void sadtarrow_select(Sadtarrow *sadtarrow, Point *clicked_point,
			      DiaRenderer *interactive_renderer);
static void sadtarrow_draw(Sadtarrow *sadtarrow, DiaRenderer *renderer);
static Object *sadtarrow_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real sadtarrow_distance_from(Sadtarrow *sadtarrow, Point *point);
static void sadtarrow_update_data(Sadtarrow *sadtarrow);
static void sadtarrow_destroy(Sadtarrow *sadtarrow);
static DiaMenu *sadtarrow_get_object_menu(Sadtarrow *sadtarrow,
					   Point *clickedpoint);

static Object *sadtarrow_load(ObjectNode obj_node, int version,
			       const char *filename);
static PropDescription *sadtarrow_describe_props(Sadtarrow *sadtarrow);
static void sadtarrow_get_props(Sadtarrow *sadtarrow, 
                                 GPtrArray *props);
static void sadtarrow_set_props(Sadtarrow *sadtarrow, 
                                 GPtrArray *props);


static ObjectTypeOps sadtarrow_type_ops =
{
  (CreateFunc)sadtarrow_create,   /* create */
  (LoadFunc)  sadtarrow_load/*using properties*/,     /* load */
  (SaveFunc)  object_save_using_properties, /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

ObjectType sadtarrow_type =
{
  "SADT - arrow",   /* name */
  0,                         /* version */
  (char **) arrow_xpm,      /* pixmap */
  
  &sadtarrow_type_ops       /* ops */
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
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      sadtarrow_get_object_menu,
  (DescribePropsFunc)   sadtarrow_describe_props,
  (GetPropsFunc)        sadtarrow_get_props,
  (SetPropsFunc)        sadtarrow_set_props
};

PropEnumData flow_style[] = {
  { N_("Normal"),SADT_ARROW_NORMAL },
  { N_("Import resource (not shown upstairs)"),SADT_ARROW_IMPORTED },
  { N_("Imply resource (not shown downstairs)"),SADT_ARROW_IMPLIED },
  { N_("Dotted arrow"),SADT_ARROW_DOTTED },
  { N_("disable arrow heads"),SADT_ARROW_DISABLED },
  { NULL }};

static PropDescription sadtarrow_props[] = {
  NEWORTHCONN_COMMON_PROPERTIES,
  { "arrow_style", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE,
    N_("Flow style:"), NULL, flow_style },
  { "autogray",PROP_TYPE_BOOL,PROP_FLAG_VISIBLE,
    N_("Automatically gray vertical flows:"),
    N_("To improve the ease of reading, flows which begin and end vertically "
       "can be rendered gray")},
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
  NEWORTHCONN_COMMON_PROPERTIES_OFFSETS,
  { "arrow_style", PROP_TYPE_ENUM, offsetof(Sadtarrow,style)},
  { "autogray",PROP_TYPE_BOOL, offsetof(Sadtarrow,autogray)},
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
  NewOrthConn *orth = &sadtarrow->orth;
  return neworthconn_distance_from(orth, point, ARROW_LINE_WIDTH);
}

static void
sadtarrow_select(Sadtarrow *sadtarrow, Point *clicked_point,
		  DiaRenderer *interactive_renderer)
{
  neworthconn_update_data(&sadtarrow->orth);
}

static void
sadtarrow_move_handle(Sadtarrow *sadtarrow, Handle *handle,
		       Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(sadtarrow!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  neworthconn_move_handle(&sadtarrow->orth, handle, to, reason);
  sadtarrow_update_data(sadtarrow);
}


static void
sadtarrow_move(Sadtarrow *sadtarrow, Point *to)
{
  neworthconn_move(&sadtarrow->orth, to);
  sadtarrow_update_data(sadtarrow);
}

static void draw_arrowhead(DiaRenderer *renderer,
			   Point *end, Point *vect, Color *col);
static void draw_dot(DiaRenderer *renderer,
		     Point *end, Point *vect, Color *col);
static void draw_tunnel(DiaRenderer *renderer,
			     Point *end, Point *vect, Color *col);

#define GBASE .45
#define GMULT .55

static void
sadtarrow_draw(Sadtarrow *sadtarrow, DiaRenderer *renderer)
{
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  NewOrthConn *orth = &sadtarrow->orth;
  Point *points;
  int n;
  int i;
  Point *p;
  real zzr;
  Color col;

  points = &orth->points[0];
  n = orth->numpoints;
  
  renderer_ops->set_linewidth(renderer, ARROW_LINE_WIDTH);
  renderer_ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer_ops->set_linecaps(renderer, LINECAPS_BUTT);
  
  col = ARROW_COLOR;
  if (sadtarrow->autogray && 
      (orth->orientation[0] == VERTICAL) && 
      (orth->orientation[orth->numpoints-2] == VERTICAL)) {
    col.red = GBASE + (GMULT*col.red);
    col.green = GBASE + (GMULT*col.green);
    col.blue = GBASE + (GMULT*col.blue);
  }

  /* Now we draw what's in essence a rounded-corner zig-zag line */
  p = points;
  zzr = ARROW_CORNER_RADIUS;

  /* This code would welcome some optimisations. In the meantime, it sorta 
     works. */
  for (i=0; i<(n-2); i++,p++) 
    {
      real len1,len2;
      real rr,cosa,sina,cosb,sinb,/*cosg,*/sing,alpha,beta,/*gamma,*/ca,cb;
      Point v1,v2,vca,vcb;
      Point X,Y,m1,m2,M,C,A,B;
      
      rr = zzr;
      
      len1 = distance_point_point(p,p+1);
      if ((len1/2) < rr) rr = len1/2;
      len2 = distance_point_point(p+1,p+2);
      if ((len2/2) < rr) rr = len2/2;

      M.x = (*(p+1)).x; X.x = p->x; Y.x = (*(p+2)).x;
      M.y = (*(p+1)).y; X.y = p->y; Y.y = (*(p+2)).y;
      m1.x = (i==0)?(p->x):((p->x + M.x) / 2);
      m1.y = (i==0)?(p->y):((p->y + M.y) / 2);
      m2.x = (i==(n-3))?(Y.x):((M.x + Y.x) / 2);
      m2.y = (i==(n-3))?(Y.y):((M.y + Y.y) / 2);
      
      if (rr < .01) {
	/* too small corner : we do it a bit faster */
	renderer_ops->draw_line(renderer,&m1,&M,&col);
	renderer_ops->draw_line(renderer,&M,&m2,&col);
       } else {
         /* full rendering. We know len1 and len2 are nonzero. */
         v1.x = (M.x - X.x) / len1;
         v1.y = (M.y - X.y) / len1;
         v2.x = (Y.x - M.x) / len2;
         v2.y = (Y.y - M.y) / len2;
       
         A.x = M.x - (v1.x * rr);
         A.y = M.y - (v1.y * rr);
         renderer_ops->draw_line(renderer,&m1,&A,&col);
         B.x = M.x + (v2.x * rr);
         B.y = M.y + (v2.y * rr);
         renderer_ops->draw_line(renderer,&B,&m2,&col);
         C.x = A.x + (v2.x * rr); /* (or B.x - v1.x*rr) */
         C.y = A.y + (v2.y * rr);
         
         vca.x = A.x - C.x;
         vca.y = -(A.y - C.y); /* @#$! coordinate system */
         vcb.x = B.x - C.x;
         vcb.y = -(B.y - C.y);
         
         ca = distance_point_point(&C,&A);
	 cb = distance_point_point(&C,&B);
	 
	 if ((ca > 1E-7) && (cb > 1E-7)) {
	   cosa = vca.x / ca; sina = vca.y / ca;
	   cosb = vcb.x / cb; sinb = vcb.y / cb;
	   /* Seems that sometimes vca.x > ca etc (though it should be
	    * impossible), so we make border cases here */
	   if (cosa > 1.0) cosa = 1.0;
	   if (cosa < -1.0) cosa = -1.0;
	   if (cosb > 1.0) cosa = 1.0;
	   if (cosb < -1.0) cosb = -1.0;
	   /*cosg = ((M.x-X.x)*(Y.x-M.x) + (M.y-X.y)*(Y.y-M.y))/(len1*len2);*/
	   sing = (-(M.x-X.x)*(Y.y-M.y) + (Y.x-M.x)*(M.y-X.y)) / (len1*len2);

	   alpha = acos(cosa) * 180.0 / M_PI;
	   if (sina < 0.0) alpha = -alpha;
	   beta = acos(cosb) * 180.0 / M_PI ; 
	   if (sinb < 0.0) beta = -beta;
	   /* we'll keep gamma in radians, since we're only interested in its
	      sign */
	   /* after all, we don't even need to compute cos(gamma), and
	      we can use sin(gamma) to extract the sign of gamma itself. */
         
	   if (alpha < 0.0) alpha += 360.0;
	   if (beta < 0.0) beta += 360.0;
	   if (sing /* gamma*/ < 0) {
	     /* if this happens, we swap alpha and beta, in order to draw
		the other portion of circle */
	     real tau;
	     tau = beta;
	     beta=alpha;
	     alpha = tau;
	   }

	   renderer_ops->draw_arc(renderer,&C,rr*2,rr*2,alpha,beta,
				   &col);
	 }
       }
    }
  
  /* depending on the exact arrow type, we'll draw different gizmos 
     (arrow heads, dots, tunnel) at different places. */

  /* XXX : chop off a little (.1) the starting and ending segments, so that
     their ends don't show up in front of the arrow heads ? Ugh.
  */

  switch (sadtarrow->style) {
  case SADT_ARROW_NORMAL:
    draw_arrowhead(renderer,&points[n-1], &points[n-2],&col);
    break;
  case SADT_ARROW_IMPORTED:
    draw_arrowhead(renderer,&points[n-1], &points[n-2],&col);
    draw_tunnel(renderer,&points[0],&points[1],&col);
    break;
  case SADT_ARROW_IMPLIED:
    draw_arrowhead(renderer,&points[n-1], &points[n-2],&col);
    draw_tunnel(renderer,&points[n-1],&points[n-2],&col);
    break;
  case SADT_ARROW_DOTTED:
    draw_arrowhead(renderer,&points[n-1], &points[n-2],&col);
    draw_arrowhead(renderer,&points[0], &points[1],&col);
    draw_dot(renderer,&points[n-1], &points[n-2],&col);
    draw_dot(renderer,&points[0], &points[1],&col);
    break;
  case SADT_ARROW_DISABLED:
    /* No arrow heads */
    break;
  }
}
static void draw_arrowhead(DiaRenderer *renderer,
		       Point *end, Point *vect, Color *col)
{
    arrow_draw(renderer, ARROW_HEAD_TYPE,
	       end, vect,
	       ARROW_HEAD_LENGTH, ARROW_HEAD_WIDTH, 
	       ARROW_LINE_WIDTH,
	       col, &color_white);
}
static void draw_dot(DiaRenderer *renderer,
		     Point *end, Point *vect, Color *col)
{
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  Point vv,vp,vt,pt;
  real vlen;
  vv = *end;
  point_sub(&vv,vect);
  vlen = distance_point_point(vect,end);
  if (vlen < 1E-7) return;
  point_scale(&vv,1/vlen);

  vp.y = vv.x;
  vp.x = -vv.y;

  pt = *end;
    vt = vp;
  point_scale(&vt,ARROW_DOT_WOFFSET);
  point_add(&pt,&vt);
  vt = vv;
  point_scale(&vt,-ARROW_DOT_LOFFSET);
  point_add(&pt,&vt);
  
  renderer_ops->set_fillstyle(renderer,FILLSTYLE_SOLID);
  renderer_ops->fill_ellipse(renderer,&pt,
			 ARROW_DOT_RADIUS,ARROW_DOT_RADIUS,
			 col);
}

static void draw_tunnel(DiaRenderer *renderer,
			     Point *end, Point *vect, Color *col)
{
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  Point vv,vp,vt1,vt2;
  BezPoint curve1[2];
  BezPoint curve2[2];

  real vlen;
  vv = *end;
  point_sub(&vv,vect);
  vlen = distance_point_point(vect,end);
  if (vlen < 1E-7) return;
  point_scale(&vv,1/vlen);
  vp.y = vv.x;
  vp.x = -vv.y;

  curve1[0].type = curve2[0].type = BEZ_MOVE_TO;
  curve1[0].p1   = curve2[0].p1   = *end;
  vt1 = vv;
  point_scale(&vt1,-ARROW_PARENS_LOFFSET - (.5*ARROW_PARENS_LENGTH));
  point_add(&curve1[0].p1,&vt1); point_add(&curve2[0].p1,&vt1); 
                                           /* gcc, work for me, please. */
  vt2 = vp;
  point_scale(&vt2,ARROW_PARENS_WOFFSET);
  point_add(&curve1[0].p1,&vt2);  point_sub(&curve2[0].p1,&vt2);

  vt2 = vp;
  vt1 = vv;
  point_scale(&vt1,2.0*ARROW_PARENS_LENGTH / 6.0);
  point_scale(&vt2,ARROW_PARENS_LENGTH / 6.0);
  curve1[1].type = curve2[1].type = BEZ_CURVE_TO;
  curve1[1].p1 = curve1[0].p1;  curve2[1].p1 = curve2[0].p1;
  point_add(&curve1[1].p1,&vt1);  point_add(&curve2[1].p1,&vt1); 
  point_add(&curve1[1].p1,&vt2);  point_sub(&curve2[1].p1,&vt2); 
  curve1[1].p2 = curve1[1].p1;  curve2[1].p2 = curve2[1].p1;
  point_add(&curve1[1].p2,&vt1);  point_add(&curve2[1].p2,&vt1); 
  curve1[1].p3 = curve1[1].p2;  curve2[1].p3 = curve2[1].p2;
  point_add(&curve1[1].p3,&vt1);  point_add(&curve2[1].p3,&vt1); 
  point_sub(&curve1[1].p3,&vt2);  point_add(&curve2[1].p3,&vt2); 

  renderer_ops->draw_bezier(renderer,curve1,2,col);
  renderer_ops->draw_bezier(renderer,curve2,2,col);
}



static Object *
sadtarrow_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Sadtarrow *sadtarrow;
  NewOrthConn *orth;
  Object *obj;

  sadtarrow = g_malloc0(sizeof(Sadtarrow));
  orth = &sadtarrow->orth;
  obj = &orth->object;

  obj->type = &sadtarrow_type;
  obj->ops = &sadtarrow_ops;
  
  neworthconn_init(orth, startpoint);

  sadtarrow_update_data(sadtarrow);

  sadtarrow->style = SADT_ARROW_NORMAL; /* sadtarrow_defaults.style; */
  sadtarrow->autogray = TRUE; /* sadtarrow_defaults.autogray; */

  *handle1 = orth->handles[0];
  *handle2 = orth->handles[orth->numpoints-2];
  return &sadtarrow->orth.object;
}

static void
sadtarrow_destroy(Sadtarrow *sadtarrow)
{
  neworthconn_destroy(&sadtarrow->orth);
}

static void
sadtarrow_update_data(Sadtarrow *sadtarrow)
{
  NewOrthConn *orth = &sadtarrow->orth;
  PolyBBExtras *extra = &orth->extra_spacing;

  neworthconn_update_data(&sadtarrow->orth);

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
  default: 
    break;
  }
  neworthconn_update_boundingbox(orth);
}

static ObjectChange *
sadtarrow_add_segment_callback(Object *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = neworthconn_add_segment((NewOrthConn *)obj, clicked);
  sadtarrow_update_data((Sadtarrow *)obj);
  return change;
}

static ObjectChange *
sadtarrow_delete_segment_callback(Object *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = neworthconn_delete_segment((NewOrthConn *)obj, clicked);
  sadtarrow_update_data((Sadtarrow *)obj);
  return change;
}

static DiaMenuItem object_menu_items[] = {
  { N_("Add segment"), sadtarrow_add_segment_callback, NULL, 1 },
  { N_("Delete segment"), sadtarrow_delete_segment_callback, NULL, 1 },
  /*  ORTHCONN_COMMON_MENUS,*/
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
  NewOrthConn *orth;

  orth = &sadtarrow->orth;
  /* Set entries sensitive/selected etc here */
  object_menu_items[0].active = neworthconn_can_add_segment(orth, clickedpoint);
  object_menu_items[1].active = neworthconn_can_delete_segment(orth, clickedpoint);
  /*  orthconn_update_object_menu(orth, clickedpoint, &object_menu_items[2]);*/
  return &object_menu;
}

static Object *
sadtarrow_load(ObjectNode obj_node, int version, const char *filename)
{
  return object_load_using_properties(&sadtarrow_type,
                                      obj_node,version,filename);
}

























