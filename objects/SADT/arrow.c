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
#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>

#include "config.h"
#include "intl.h"
#include "object.h"
#include "neworth_conn.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "widgets.h"
#include "message.h"
#include "color.h"
#include "lazyprops.h"

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

typedef struct _SadtarrowPropertiesDialog SadtarrowPropertiesDialog;
typedef struct _SadtarrowDefaultsDialog SadtarrowDefaultsDialog;
typedef struct _SadtarrowState SadtarrowState;

struct _SadtarrowState {
  ObjectState obj_state;
  
  Sadtarrow_style style;
  gboolean autogray;
};

typedef struct _Sadtarrow {
  NewOrthConn orth;

  Sadtarrow_style style;
  gboolean autogray;
} Sadtarrow;

struct _SadtarrowPropertiesDialog {
  AttributeDialog dialog;
  Sadtarrow *parent;

  EnumAttribute style;
  BoolAttribute autogray;
};

typedef struct _SadtarrowDefaults {
  Sadtarrow_style style;
  gboolean autogray;
} SadtarrowDefaults;

struct _SadtarrowDefaultsDialog {
  AttributeDialog dialog;
  SadtarrowDefaults *parent;

  EnumAttribute style;
  BoolAttribute autogray;
};

static SadtarrowPropertiesDialog *sadtarrow_properties_dialog;
static SadtarrowDefaultsDialog *sadtarrow_defaults_dialog;
static SadtarrowDefaults sadtarrow_defaults;

static void sadtarrow_move_handle(Sadtarrow *sadtarrow, Handle *handle,
				   Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void sadtarrow_move(Sadtarrow *sadtarrow, Point *to);
static void sadtarrow_select(Sadtarrow *sadtarrow, Point *clicked_point,
			      Renderer *interactive_renderer);
static void sadtarrow_draw(Sadtarrow *sadtarrow, Renderer *renderer);
static Object *sadtarrow_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real sadtarrow_distance_from(Sadtarrow *sadtarrow, Point *point);
static void sadtarrow_update_data(Sadtarrow *sadtarrow);
static void sadtarrow_destroy(Sadtarrow *sadtarrow);
static Object *sadtarrow_copy(Sadtarrow *sadtarrow);
static PROPDLG_TYPE sadtarrow_get_properties(Sadtarrow *sadtarrow);
static ObjectChange *sadtarrow_apply_properties(Sadtarrow *sadtarrow);
static DiaMenu *sadtarrow_get_object_menu(Sadtarrow *sadtarrow,
					   Point *clickedpoint);

static SadtarrowState *sadtarrow_get_state(Sadtarrow *sadtarrow);
static void sadtarrow_set_state(Sadtarrow *sadtarrow, SadtarrowState *state);

static void sadtarrow_save(Sadtarrow *sadtarrow, ObjectNode obj_node,
			    const char *filename);
static Object *sadtarrow_load(ObjectNode obj_node, int version,
			       const char *filename);

static PROPDLG_TYPE sadtarrow_get_defaults(void);
static void sadtarrow_apply_defaults(void);

static ObjectTypeOps sadtarrow_type_ops =
{
  (CreateFunc)sadtarrow_create,   /* create */
  (LoadFunc)  sadtarrow_load,     /* load */
  (SaveFunc)  sadtarrow_save,      /* save */
  (GetDefaultsFunc)   sadtarrow_get_defaults, 
  (ApplyDefaultsFunc) sadtarrow_apply_defaults
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
  (CopyFunc)            sadtarrow_copy,
  (MoveFunc)            sadtarrow_move,
  (MoveHandleFunc)      sadtarrow_move_handle,
  (GetPropertiesFunc)   sadtarrow_get_properties,
  (ApplyPropertiesFunc) sadtarrow_apply_properties,
  (ObjectMenuFunc)      sadtarrow_get_object_menu
};

static ObjectChange *
sadtarrow_apply_properties(Sadtarrow *sadtarrow)
{
  ObjectState *old_state;
  SadtarrowPropertiesDialog *dlg = sadtarrow_properties_dialog;

  PROPDLG_SANITY_CHECK(dlg,sadtarrow);
  
  old_state = (ObjectState *)sadtarrow_get_state(sadtarrow);

  PROPDLG_APPLY_ENUM(dlg,style);
  PROPDLG_APPLY_BOOL(dlg,autogray);

  sadtarrow_update_data(sadtarrow);
  return new_object_state_change(&sadtarrow->orth.object, old_state, 
				 (GetStateFunc)sadtarrow_get_state,
				 (SetStateFunc)sadtarrow_set_state);
}

PropDlgEnumEntry flow_style[] = {
  { N_("Normal"),SADT_ARROW_NORMAL,NULL },
  { N_("Import resource (not shown upstairs)"),SADT_ARROW_IMPORTED,NULL },
  { N_("Imply resource (not shown downstairs)"),SADT_ARROW_IMPLIED, NULL },
  { N_("Dotted arrow"),SADT_ARROW_DOTTED, NULL },
  { N_("disable arrow heads"),SADT_ARROW_DISABLED, NULL },
  { NULL}};

static PROPDLG_TYPE
sadtarrow_get_properties(Sadtarrow *sadtarrow)
{
  SadtarrowPropertiesDialog *dlg = sadtarrow_properties_dialog;
  
  PROPDLG_CREATE(dlg,sadtarrow);
  PROPDLG_SHOW_ENUM(dlg,style,_("Flow style:"),flow_style);
  PROPDLG_SHOW_BOOL(dlg,autogray,_("Automatically gray vertical flows:"));
  PROPDLG_READY(dlg);

  sadtarrow_properties_dialog = dlg;

  PROPDLG_RETURN(dlg);
}

static void
init_default_values(void) {
  static int defaults_initialized = 0;

  if (!defaults_initialized) {
    sadtarrow_defaults.style = SADT_ARROW_NORMAL;
    sadtarrow_defaults.autogray = TRUE;
    defaults_initialized = 1;
  }
}

static void 
sadtarrow_apply_defaults(void)
{
  SadtarrowDefaultsDialog *dlg = sadtarrow_defaults_dialog;

  PROPDLG_APPLY_ENUM(dlg,style);
  PROPDLG_APPLY_BOOL(dlg,autogray);
}


static PROPDLG_TYPE
sadtarrow_get_defaults()
{
  SadtarrowDefaultsDialog *dlg = sadtarrow_defaults_dialog;

  init_default_values();
  PROPDLG_CREATE(dlg, &sadtarrow_defaults);
  PROPDLG_SHOW_ENUM(dlg,style,_("Flow style:"),flow_style);
  PROPDLG_SHOW_BOOL(dlg,autogray,_("Automatically gray vertical flows:"));
  PROPDLG_READY(dlg);
  sadtarrow_defaults_dialog = dlg;

  PROPDLG_RETURN(dlg);
}

static real
sadtarrow_distance_from(Sadtarrow *sadtarrow, Point *point)
{
  NewOrthConn *orth = &sadtarrow->orth;
  return neworthconn_distance_from(orth, point, ARROW_LINE_WIDTH);
}

static void
sadtarrow_select(Sadtarrow *sadtarrow, Point *clicked_point,
		  Renderer *interactive_renderer)
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

static void draw_arrowhead(Renderer *renderer,
			   Point *end, Point *vect, Color *col);
static void draw_dot(Renderer *renderer,
		     Point *end, Point *vect, Color *col);
static void draw_parenthesis(Renderer *renderer,
			     Point *end, Point *vect, Color *col);

#define GBASE .45
#define GMULT .55

static void
sadtarrow_draw(Sadtarrow *sadtarrow, Renderer *renderer)
{
  NewOrthConn *orth = &sadtarrow->orth;
  Point *points;
  int n;
  int i;
  Point *p;
  real zzr;
  Color col;

  points = &orth->points[0];
  n = orth->numpoints;
  
  renderer->ops->set_linewidth(renderer, ARROW_LINE_WIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);
  
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
	renderer->ops->draw_line(renderer,&m1,&M,&col);
	renderer->ops->draw_line(renderer,&M,&m2,&col);
       } else {
         /* full rendering. We know len1 and len2 are nonzero. */
         v1.x = (M.x - X.x) / len1;
         v1.y = (M.y - X.y) / len1;
         v2.x = (Y.x - M.x) / len2;
         v2.y = (Y.y - M.y) / len2;
       
         A.x = M.x - (v1.x * rr);
         A.y = M.y - (v1.y * rr);
         renderer->ops->draw_line(renderer,&m1,&A,&col);
         B.x = M.x + (v2.x * rr);
         B.y = M.y + (v2.y * rr);
         renderer->ops->draw_line(renderer,&B,&m2,&col);
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
	   
	   renderer->ops->draw_arc(renderer,&C,rr*2,rr*2,alpha,beta,
				   &col);
	 }
       }
    }
  
  /* depending on the exact arrow type, we'll draw different gizmos 
     (arrow heads, dots, parenthesis) at different places. */

  /* XXX : chop off a little (.1) the starting and ending segments, so that
     their ends don't show up in front of the arrow heads ? Ugh.
  */

  switch (sadtarrow->style) {
  case SADT_ARROW_NORMAL:
    draw_arrowhead(renderer,&points[n-1], &points[n-2],&col);
    break;
  case SADT_ARROW_IMPORTED:
    draw_arrowhead(renderer,&points[n-1], &points[n-2],&col);
    draw_parenthesis(renderer,&points[0],&points[1],&col);
    break;
  case SADT_ARROW_IMPLIED:
    draw_arrowhead(renderer,&points[n-1], &points[n-2],&col);
    draw_parenthesis(renderer,&points[n-1],&points[n-2],&col);
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
static void draw_arrowhead(Renderer *renderer,
		       Point *end, Point *vect, Color *col)
{
    arrow_draw(renderer, ARROW_HEAD_TYPE,
	       end, vect,
	       ARROW_HEAD_LENGTH, ARROW_HEAD_WIDTH, 
	       ARROW_LINE_WIDTH,
	       col, &color_white);
}
static void draw_dot(Renderer *renderer,
		     Point *end, Point *vect, Color *col)
{
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
  
  renderer->ops->set_fillstyle(renderer,FILLSTYLE_SOLID);
  renderer->ops->fill_ellipse(renderer,&pt,
			 ARROW_DOT_RADIUS,ARROW_DOT_RADIUS,
			 col);
}

static void draw_parenthesis(Renderer *renderer,
			     Point *end, Point *vect, Color *col)
{
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

  renderer->ops->draw_bezier(renderer,curve1,2,col);
  renderer->ops->draw_bezier(renderer,curve2,2,col);
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

  init_default_values();
  sadtarrow = g_malloc0(sizeof(Sadtarrow));
  orth = &sadtarrow->orth;
  obj = &orth->object;

  obj->type = &sadtarrow_type;
  obj->ops = &sadtarrow_ops;
  
  neworthconn_init(orth, startpoint);

  sadtarrow_update_data(sadtarrow);

  sadtarrow->style = sadtarrow_defaults.style;
  sadtarrow->autogray = sadtarrow_defaults.autogray;

  *handle1 = orth->handles[0];
  *handle2 = orth->handles[orth->numpoints-2];
  return &sadtarrow->orth.object;
}

static void
sadtarrow_destroy(Sadtarrow *sadtarrow)
{
  neworthconn_destroy(&sadtarrow->orth);
}

static Object *
sadtarrow_copy(Sadtarrow *sadtarrow)
{
  Sadtarrow *newsadtarrow;
  NewOrthConn *orth, *neworth;
  Object *newobj;
  
  orth = &sadtarrow->orth;
 
  newsadtarrow = g_malloc0(sizeof(Sadtarrow));
  neworth = &newsadtarrow->orth;
  newobj = &neworth->object;

  neworthconn_copy(orth, neworth);

  newsadtarrow->style = sadtarrow->style;
  newsadtarrow->autogray = sadtarrow->autogray;

  return &newsadtarrow->orth.object;
}

static SadtarrowState *
sadtarrow_get_state(Sadtarrow *sadtarrow)
{
  SadtarrowState *state = g_new(SadtarrowState, 1);

  state->obj_state.free = NULL;
  
  state->style = sadtarrow->style;
  state->autogray = sadtarrow->autogray;

  return state;
}

static void
sadtarrow_set_state(Sadtarrow *sadtarrow, SadtarrowState *state)
{
  sadtarrow->style = state->style;
  sadtarrow->autogray = state->autogray;

  g_free(state);
  
  sadtarrow_update_data(sadtarrow);
}

static void
sadtarrow_update_data(Sadtarrow *sadtarrow)
{
  NewOrthConn *orth = &sadtarrow->orth;
  NewOrthConnBBExtras *extra = &orth->extra_spacing;

  neworthconn_update_data(&sadtarrow->orth);

  extra->start_long = 
    extra->middle_trans = ARROW_LINE_WIDTH / 2.0;
  
  extra->end_long = MAX(ARROW_HEAD_LENGTH,ARROW_LINE_WIDTH/2.0);
  
  extra->start_trans = ARROW_LINE_WIDTH / 2.0;
  extra->end_trans = MAX(ARROW_LINE_WIDTH/2.0,ARROW_HEAD_WIDTH/2.0);
  
  switch(sadtarrow->style) {
  case SADT_ARROW_IMPORTED:
    extra->start_trans = MAX(ARROW_LINE_WIDTH/2.0,ARROW_PARENS_WOFFSET);
    break;
  case SADT_ARROW_IMPLIED:
    extra->end_trans = MAX(ARROW_LINE_WIDTH/2.0,
                             MAX(ARROW_PARENS_WOFFSET,
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
  return &object_menu;
}


static void
sadtarrow_save(Sadtarrow *sadtarrow, ObjectNode obj_node,
		const char *filename)
{
  neworthconn_save(&sadtarrow->orth, obj_node);

  save_enum(obj_node,"arrow_style",sadtarrow->style);
  save_boolean(obj_node,"autogray",sadtarrow->autogray);
}

static Object *
sadtarrow_load(ObjectNode obj_node, int version, const char *filename)
{
  Sadtarrow *sadtarrow;
  NewOrthConn *orth;
  Object *obj;

  init_default_values();
  sadtarrow = g_malloc0(sizeof(Sadtarrow));

  orth = &sadtarrow->orth;
  obj = &orth->object;

  obj->type = &sadtarrow_type;
  obj->ops = &sadtarrow_ops;

  neworthconn_load(orth, obj_node);

  sadtarrow->style = load_enum(obj_node,"arrow_style",SADT_ARROW_NORMAL);
  sadtarrow->autogray = load_boolean(obj_node,"autogray",TRUE);

  sadtarrow_update_data(sadtarrow);

  return &sadtarrow->orth.object;
}

























