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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <math.h>

#include "intl.h"
#include "object.h"
#include "connection.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "widgets.h"
#include "arrows.h"
#include "properties.h"

#include "pixmaps/arc.xpm"

#define DEFAULT_WIDTH 0.25

#define HANDLE_MIDDLE HANDLE_CUSTOM1

/* If you wan debug spew */
#define TRACE(fun) /*fun*/

typedef struct _Arc Arc;

struct _Arc {
  Connection connection;

  Handle middle_handle;

  Color arc_color;
  real curve_distance;
  real line_width;
  LineStyle line_style;
  real dashlength;
  Arrow start_arrow, end_arrow;

  /* Calculated parameters: */
  real radius;
  Point center;
  real angle1, angle2;

};

/* updates both endpoints and arc->curve_distance */
static ObjectChange* arc_move_handle(Arc *arc, Handle *handle,
				     Point *to, ConnectionPoint *cp,
				     HandleMoveReason reason, ModifierKeys modifiers);
static ObjectChange* arc_move(Arc *arc, Point *to);
static void arc_select(Arc *arc, Point *clicked_point,
		       DiaRenderer *interactive_renderer);
static void arc_draw(Arc *arc, DiaRenderer *renderer);
static DiaObject *arc_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static real arc_distance_from(Arc *arc, Point *point);
static void arc_update_data(Arc *arc);
static void arc_update_handles(Arc *arc);
static void arc_destroy(Arc *arc);
static DiaObject *arc_copy(Arc *arc);

static PropDescription *arc_describe_props(Arc *arc);
static void arc_get_props(Arc *arc, GPtrArray *props);
static void arc_set_props(Arc *arc, GPtrArray *props);

static void arc_save(Arc *arc, ObjectNode obj_node, const char *filename);
static DiaObject *arc_load(ObjectNode obj_node, int version, const char *filename);
static int arc_compute_midpoint(Arc *arc, const Point * ep0, const Point * ep1 , Point * midpoint);
static void calculate_arc_object_edge(Arc *arc, real ang_start, real ang_end, DiaObject *obj, Point *target);
static void arc_get_point_at_angle(Arc *arc, Point* point, real angle);

static ObjectTypeOps arc_type_ops =
{
  (CreateFunc) arc_create,
  (LoadFunc)   arc_load,
  (SaveFunc)   arc_save,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType arc_type =
{
  "Standard - Arc",  /* name */
  0,                 /* version */
  (char **) arc_xpm, /* pixmap */
  
  &arc_type_ops      /* ops */
};

DiaObjectType *_arc_type = (DiaObjectType *) &arc_type;

static ObjectOps arc_ops = {
  (DestroyFunc)         arc_destroy,
  (DrawFunc)            arc_draw,
  (DistanceFunc)        arc_distance_from,
  (SelectFunc)          arc_select,
  (CopyFunc)            arc_copy,
  (MoveFunc)            arc_move,
  (MoveHandleFunc)      arc_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   arc_describe_props,
  (GetPropsFunc)        arc_get_props,
  (SetPropsFunc)        arc_set_props,
};

static PropDescription arc_props[] = {
  OBJECT_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_LINE_STYLE,
  PROP_STD_START_ARROW,
  PROP_STD_END_ARROW,
  { "curve_distance", PROP_TYPE_REAL, 0,
    N_("Curve distance"), NULL },
  PROP_DESC_END
};

static PropDescription *
arc_describe_props(Arc *arc)
{
  if (arc_props[0].quark == 0)
    prop_desc_list_calculate_quarks(arc_props);
  return arc_props;
}

static PropOffset arc_offsets[] = {
  OBJECT_COMMON_PROPERTIES_OFFSETS,
  { "line_width", PROP_TYPE_REAL, offsetof(Arc, line_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Arc, arc_color) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Arc, line_style), offsetof(Arc, dashlength) },
  { "start_arrow", PROP_TYPE_ARROW, offsetof(Arc, start_arrow) },
  { "end_arrow", PROP_TYPE_ARROW, offsetof(Arc, end_arrow) },
  { "curve_distance", PROP_TYPE_REAL, offsetof(Arc, curve_distance) },
  { "start_point", PROP_TYPE_POINT, offsetof(Connection, endpoints[0]) },
  { "end_point", PROP_TYPE_POINT, offsetof(Connection, endpoints[1]) },
  { NULL, 0, 0 }
};

static void
arc_get_props(Arc *arc, GPtrArray *props)
{
  object_get_props_from_offsets(&arc->connection.object, 
                                arc_offsets, props);
}

static void
arc_set_props(Arc *arc, GPtrArray *props)
{
  object_set_props_from_offsets(&arc->connection.object, 
                                arc_offsets, props);
  arc_update_data(arc);
}

static int
in_angle(real angle, real startangle, real endangle)
{
  if (startangle > endangle) {  /* passes 360 degrees */
    endangle += 360.0;
    if (angle<startangle)
      angle += 360;
  }
  return (angle>=startangle) && (angle<=endangle);
}

static real
arc_distance_from(Arc *arc, Point *point)
{
  Point *endpoints;
  Point from_center;
  real angle;
  real d, d2;
  
  endpoints = &arc->connection.endpoints[0];

  from_center = *point;
  point_sub(&from_center, &arc->center);

  angle = -atan2(from_center.y, from_center.x)*180.0/M_PI;
  if (angle<0)
    angle+=360.0;

  if (in_angle(angle, arc->angle1, arc->angle2)) {
    d = fabs(sqrt(point_dot(&from_center, &from_center)) - arc->radius);
    d -= arc->line_width/2.0;
    if (d<0)
      d = 0.0;
    return d;
  } else {
    d = distance_point_point(&endpoints[0], point);
    d2 = distance_point_point(&endpoints[1], point);

    return MIN(d,d2);
  }
}

static void
arc_select(Arc *arc, Point *clicked_point,
	   DiaRenderer *interactive_renderer)
{
  arc_update_handles(arc);
}

static void
arc_update_handles(Arc *arc)
{
  Point *middle_pos;
  real dist,dx,dy;

  Connection *conn = &arc->connection;

  connection_update_handles(conn);
  
  middle_pos = &arc->middle_handle.pos;

  dx = conn->endpoints[1].x - conn->endpoints[0].x;
  dy = conn->endpoints[1].y - conn->endpoints[0].y;
  
  dist = sqrt(dx*dx + dy*dy);
  if (dist > 0.000001) {
    middle_pos->x =
      (conn->endpoints[0].x + conn->endpoints[1].x) / 2.0 -
      arc->curve_distance*dy/dist;
    middle_pos->y =
      (conn->endpoints[0].y + conn->endpoints[1].y) / 2.0 +
      arc->curve_distance*dx/dist;
  }
}
/** returns the number of intersection the circle has with the horizontal line at y=horiz
 * if 1 point intersects then *int1 is that point
 * if 2 points intersect then *int1 and *int2 are these points
 */
static int
arc_circle_intersects_horiz(const Arc *arc, real horiz, Point *int1, Point *int2){
  /* inject y=horiz into r^2 = (x-x_c)^2 + (y-y_c)^2 
   * this is r^2 = (x-x_c)^2 + (horiz-y_c)^2 
   * translate to x^2 + b*x + c = 0 
   * b = -2 x_c 
   * c = x_c^2 - r^2 + (horiz-y_c)^2 
   * and solve classically */
  real b, c, delta;
  b = -2.0 * arc->center.x;
  c =  arc->center.x * arc->center.x + (horiz - arc->center.y) * (horiz - arc->center.y) - arc->radius * arc->radius;
  delta = b*b - 4 * c;
  if (delta < 0)
          return 0;
  else if (delta == 0){
         int1->x = -b/2; 
         int1->y = horiz;
         return 1; 
  }
  else {
         int1->x = (sqrt(delta)-b)/2; 
         int1->y = horiz;
         int2->x = (-sqrt(delta)-b)/2; 
         int2->y = horiz;
          return 2;
  }
}
/** returns the number of intersection the circle has with the vertical line at x=vert
 * if 1 point intersects then *int1 is that point
 * if 2 points intersect then *int1 and *int2 are these points
 */
static int
arc_circle_intersects_vert(const Arc *arc, real vert, Point *int1, Point *int2){
  /* inject x=vert into r^2 = (x-x_c)^2 + (y-y_c)^2 
   * this is r^2 = (vert-x_c)^2 + (y-y_c)^2 
   * translate to y^2 + b*y + c = 0 and solve classically */
  real b, c, delta;
  b = -2*arc->center.y;
  c =  arc->center.y * arc->center.y + (vert - arc->center.x) * (vert - arc->center.x) - arc->radius * arc->radius;
  delta = b*b - 4 * c;
  if (delta < 0)
          return 0;
  else if (delta == 0){
         int1->y = -b/2; 
         int1->x = vert;
         return 1; 
  }
  else {
         int1->y = (sqrt(delta)-b)/2; 
         int1->x = vert;
         int2->y = (-sqrt(delta)-b)/2; 
         int2->x = vert;
         return 2;
  }
}
        
        
            

static real
arc_compute_curve_distance(const Arc *arc, const Point *start, const Point *end, const Point *mid)
{
    Point a,b;
    real tmp,cd;

    b = *mid;
    point_sub(&b, start);
   
    a = *end;
    point_sub(&a, start);

    tmp = point_dot(&a,&b);
    cd =
      sqrt(fabs(point_dot(&b,&b) - tmp*tmp/point_dot(&a,&a)));
    
    if (a.x*b.y - a.y*b.x < 0) 
      cd = - cd;
    return cd;
}

/** rotates p around the center by an angle given in radians 
 * a positive angle is ccw on the screen*/
static void
rotate_point_around_point(Point *p, const Point *center, real angle)
{
        real radius;
        real a;
        point_sub(p,center);
        radius = point_len(p);
        a = -atan2(p->y,p->x); /* y axis points down*/
        a += angle;
        p->x = cos(a); p->y = -sin(a);/* y axis points down*/
        point_scale(p,radius);
        point_add(p,center);
}


/* finds the point intersecting the full circle 
 * on the vector defined by the center and Point *to
 * that point is returned in Point *best if 1 is returned */
static int
arc_find_radial(const Arc *arc, const Point *to, Point *best)
{
        Point tmp;
        tmp = *to;
        point_sub(&tmp, &arc->center);
        point_normalize(&tmp);
        point_scale(&tmp,arc->radius);
        point_add(&tmp, &arc->center);
        *best = tmp;
        return 1;
        
}

/* finds the closest point intersecting the full circle 
 * at any position on the vertical and horizontal lines going through Point to
 * that point is returned in Point *best if 1 is returned */
static int
arc_find_closest_vert_horiz(const Arc *arc, const Point *to, Point *best){
     Point i1,i2;
     int nh,nv;
        nh = arc_circle_intersects_horiz(arc, to->y, &i1, &i2);
        if (nh==2){
           *best = *closest_to(to,&i1,&i2);
        }
        else if (nh==1) {
           *best = i1;
        }
        nv = arc_circle_intersects_vert(arc, to->x, &i1, &i2);
        if (nv==2){
           Point tmp;
           tmp = *closest_to(to,&i1,&i2);
           *best = *closest_to(to,&tmp,best);
        }
        else if (nv==1) {
           *best = *closest_to(to,&i1,best);
        }
        if (nv|nh)
                return 1;
        return 0;
}
static ObjectChange*
arc_move_handle(Arc *arc, Handle *handle,
		Point *to, ConnectionPoint *cp,
		HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(arc!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);
  if (handle->id == HANDLE_MIDDLE) {
          TRACE(printf("curve_dist: %.2f \n",arc->curve_distance));
          arc->curve_distance = arc_compute_curve_distance(arc, &arc->connection.endpoints[0], &arc->connection.endpoints[1], to);
          TRACE(printf("curve_dist: %.2f \n",arc->curve_distance));

  } else {
        Point best;
        TRACE(printf("Modifiers: %d \n",modifiers));
        if (modifiers & MODIFIER_SHIFT)
        /* if(arc->end_arrow.type == ARROW_NONE)*/
        {
          TRACE(printf("SHIFT USED, to at %.2f %.2f  ",to->x,to->y));
          if (arc_find_radial(arc, to, &best)){
            /* needs to move two handles at the same time 
             * compute pos of middle handle */
            Point midpoint;
            int ok;
            if (handle == (&arc->connection.endpoint_handles[0]))
              ok = arc_compute_midpoint(arc, &best , &arc->connection.endpoints[1], &midpoint);
            else
              ok = arc_compute_midpoint(arc,  &arc->connection.endpoints[0], &best , &midpoint);
            if (!ok)
              return NULL;
            connection_move_handle(&arc->connection, handle->id, &best, cp, reason, modifiers);
            /* recompute curve distance equiv. move middle handle */
            arc->curve_distance = arc_compute_curve_distance(arc, &arc->connection.endpoints[0], &arc->connection.endpoints[1], &midpoint);
            TRACE(printf("curve_dist: %.2f \n",arc->curve_distance));
          }
          else {
            TRACE(printf("NO best\n"));
          }
       } else {
          connection_move_handle(&arc->connection, handle->id, to, cp, reason, modifiers);
       }
  }

  arc_update_data(arc);

  return NULL;
}

static ObjectChange*
arc_move(Arc *arc, Point *to)
{
  Point start_to_end;
  Point *endpoints = &arc->connection.endpoints[0]; 

  start_to_end = endpoints[1];
  point_sub(&start_to_end, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add(&endpoints[1], &start_to_end);

  arc_update_data(arc);

  return NULL;
}

static int 
arc_compute_midpoint(Arc *arc, const Point * ep0, const Point * ep1 , Point * midpoint)
{
            real angle;
            Point midpos;
            Point *oep0, *oep1;
            
            oep0 = &arc->connection.endpoints[0];
            oep1 = &arc->connection.endpoints[1];

            /* angle is total delta of angle of both endpoints */
            angle = -atan2(ep0->y - arc->center.y, ep0->x - arc->center.x); /* angle of new */
            angle -= -atan2(oep0->y - arc->center.y, oep0->x - arc->center.x); /* minus angle of old */
            angle += -atan2(ep1->y - arc->center.y, ep1->x - arc->center.x); /* plus angle of new */
            angle -= -atan2(oep1->y - arc->center.y, oep1->x - arc->center.x); /* minus angle of old */
            if (!finite(angle)){
                    return 0;
            }
            if (angle < -1 * M_PI){
                    TRACE(printf("angle: %.2f ",angle));
                    angle += 2*M_PI;
                    TRACE(printf("angle: %.2f ",angle));
            }
            if (angle > 1 * M_PI){
                    TRACE(printf("angle: %.2f ",angle));
                    angle -= 2*M_PI;
                    TRACE(printf("angle: %.2f ",angle));
            }

            midpos = arc->middle_handle.pos;
            /*rotate middle handle by half the angle */
            TRACE(printf("\nmidpos before: %.2f %.2f \n",midpos.x, midpos.y));
            rotate_point_around_point(&midpos, &arc->center, angle/2); 
            TRACE(printf("\nmidpos after : %.2f %.2f \n",midpos.x, midpos.y));
            *midpoint = midpos;
            return 1;
}
/** updates point to the point on the arc at angle angle */
void arc_get_point_at_angle(Arc *arc, Point* point, real angle)
{
        Point vec;
        vec.x = cos(angle/180.0*M_PI);
        vec.y = sin(angle/180.0*M_PI);
        point_copy(point,&arc->center);
        point_add_scaled(point,&vec,arc->radius);
}
static void
calculate_arc_object_edge(Arc *arc, real ang_start, real ang_end, DiaObject *obj, Point *target) 
{
#define MAXITER 25
#ifdef TRACE_DIST
  real trace[MAXITER];
  real disttrace[MAXITER];
#endif
  real mid1, mid2, mid3;
  real dist;
  int i = 0;

  mid1 = ang_start;
  mid2 = (ang_start + ang_end)/2;
  mid3 = ang_end;

  /* If the other end is inside the object */
  arc_get_point_at_angle(arc,target,mid3);
  dist = obj->ops->distance_from(obj, target );
  if (dist < 0.001){
          arc_get_point_at_angle(arc,target,mid1);
          return ;
  }
  do {
    arc_get_point_at_angle(arc, target, mid2);
    dist = obj->ops->distance_from(obj, target);
    if (dist < 0.0000001) {
      mid1 = mid2;
    } else {
      mid3 = mid2;
    }
    mid2 = (mid1 + mid3) / 2;
    
#ifdef TRACE_DIST
    trace[i] = mid2;
    disttrace[i] = dist;
#endif
    i++;
  } while (i < MAXITER && (dist < 0.0000001 || dist > 0.001));
  
#ifdef TRACE_DIST
  if (i == MAXITER) {
    for (i = 0; i < MAXITER; i++) {
      printf("%d: %f, %f: %f\n", i, trace[i].x, trace[i].y, disttrace[i]);
    }
    printf("i = %d, dist = %f\n", i, dist);
  }
#endif
  arc_get_point_at_angle(arc,target,mid2);
  return ;
}
static void
arc_draw(Arc *arc, DiaRenderer *renderer)
{
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  Point *endpoints;
  Point gaptmp[3];
  ConnectionPoint *start_cp, *end_cp;  
    
  assert(arc != NULL);
  assert(renderer != NULL);

  endpoints = &arc->connection.endpoints[0];

  gaptmp[0] = endpoints[0];
  gaptmp[1] = endpoints[1];
  start_cp = arc->connection.endpoint_handles[0].connected_to;
  end_cp = arc->connection.endpoint_handles[1].connected_to;

  if (connpoint_is_autogap(start_cp)) 
     calculate_arc_object_edge(arc, arc->angle1, arc->angle2, start_cp->object, &gaptmp[0]);
  if (connpoint_is_autogap(end_cp)) 
     calculate_arc_object_edge(arc, arc->angle2, arc->angle1, end_cp->object, &gaptmp[1]);

  /* compute new middle_point */
  arc_compute_midpoint(arc, &gaptmp[0], &gaptmp[1], &gaptmp[2]); 

  renderer_ops->set_linewidth(renderer, arc->line_width);
  renderer_ops->set_linestyle(renderer, arc->line_style);
  renderer_ops->set_dashlength(renderer, arc->dashlength);
  renderer_ops->set_linecaps(renderer, LINECAPS_BUTT);
  
  /* Special case when almost line: */
  if (fabs(arc->curve_distance) <= 0.01) {
          TRACE(printf("drawing like a line\n")); 
    renderer_ops->draw_line_with_arrows(renderer,
					 &gaptmp[0], &gaptmp[1],
					 arc->line_width,
					 &arc->arc_color,
					 &arc->start_arrow,
					 &arc->end_arrow);
    return;
  }

  renderer_ops->draw_arc_with_arrows(renderer,
				      &gaptmp[0],
				      &gaptmp[1],
				      &gaptmp[2],
				      arc->line_width,
				      &arc->arc_color,
				      &arc->start_arrow,
				      &arc->end_arrow);
}

static DiaObject *
arc_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Arc *arc;
  Connection *conn;
  DiaObject *obj;
  Point defaultlen = { 1.0, 1.0 };

  arc = g_malloc0(sizeof(Arc));

  arc->line_width =  attributes_get_default_linewidth();
  arc->curve_distance = 1.0;
  arc->arc_color = attributes_get_foreground(); 
  attributes_get_default_line_style(&arc->line_style, &arc->dashlength);
  arc->start_arrow = attributes_get_default_start_arrow();
  arc->end_arrow = attributes_get_default_end_arrow();

  conn = &arc->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  point_add(&conn->endpoints[1], &defaultlen);
 
  obj = &conn->object;
  
  obj->type = &arc_type;;
  obj->ops = &arc_ops;
  
  connection_init(conn, 3, 0);

  obj->handles[2] = &arc->middle_handle;
  arc->middle_handle.id = HANDLE_MIDDLE;
  arc->middle_handle.type = HANDLE_MINOR_CONTROL;
  arc->middle_handle.connect_type = HANDLE_NONCONNECTABLE;
  arc->middle_handle.connected_to = NULL;

  arc_update_data(arc);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return &arc->connection.object;
}

static void
arc_destroy(Arc *arc)
{
  connection_destroy(&arc->connection);
}

static DiaObject *
arc_copy(Arc *arc)
{
  Arc *newarc;
  Connection *conn, *newconn;
  DiaObject *newobj;
  
  conn = &arc->connection;
  
  newarc = g_malloc0(sizeof(Arc));
  newconn = &newarc->connection;
  newobj = &newconn->object;

  connection_copy(conn, newconn);

  newarc->arc_color = arc->arc_color;
  newarc->curve_distance = arc->curve_distance;
  newarc->line_width = arc->line_width;
  newarc->line_style = arc->line_style;
  newarc->dashlength = arc->dashlength;
  newarc->start_arrow = arc->start_arrow;
  newarc->end_arrow = arc->end_arrow;
  newarc->radius = arc->radius;
  newarc->center = arc->center;
  newarc->angle1 = arc->angle1;
  newarc->angle2 = arc->angle2;

  newobj->handles[2] = &newarc->middle_handle;
  
  newarc->middle_handle = arc->middle_handle;

  return &newarc->connection.object;
}

static void
arc_update_data(Arc *arc)
{
  Connection *conn = &arc->connection;
  LineBBExtras *extra =&conn->extra_spacing;
  DiaObject *obj = &conn->object;
  Point *endpoints;
  real x1,y1,x2,y2,xc,yc;
  real lensq, alpha, radius;
  real angle1, angle2;
  
  endpoints = &arc->connection.endpoints[0];
  x1 = endpoints[0].x;
  y1 = endpoints[0].y;
  x2 = endpoints[1].x;
  y2 = endpoints[1].y;
  
  lensq = (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1);
  radius = lensq/(8*arc->curve_distance) + arc->curve_distance/2.0;

  alpha = (radius - arc->curve_distance) / sqrt(lensq);
  
  xc = (x1 + x2) / 2.0 + (y2 - y1)*alpha;
  yc = (y1 + y2) / 2.0 + (x1 - x2)*alpha;

  angle1 = -atan2(y1-yc, x1-xc)*180.0/M_PI;
  if (angle1<0)
    angle1+=360.0;
  angle2 = -atan2(y2-yc, x2-xc)*180.0/M_PI;
  if (angle2<0)
    angle2+=360.0;

  if (radius<0.0) {
    real tmp;
    tmp = angle1;
    angle1 = angle2;
    angle2 = tmp;
    radius = -radius;
  }
  
  arc->radius = radius;
  arc->center.x = xc; arc->center.y = yc;
  arc->angle1 = angle1;
  arc->angle2 = angle2;

  extra->start_trans =  (arc->line_width / 2.0);
  extra->end_trans =     (arc->line_width / 2.0);
  if (arc->start_arrow.type != ARROW_NONE) 
    extra->start_trans = MAX(extra->start_trans,arc->start_arrow.width);
  if (arc->end_arrow.type != ARROW_NONE) 
    extra->end_trans = MAX(extra->end_trans,arc->end_arrow.width);
  extra->start_long  = (arc->line_width / 2.0);
  extra->end_long    = (arc->line_width / 2.0);

  connection_update_boundingbox(conn);
  /* fix boundingbox for arc's special shape XXX find a more elegant way: */
  if (in_angle(0, arc->angle1, arc->angle2)) {
    obj->bounding_box.right = arc->center.x + arc->radius 
      + (arc->line_width / 2.0);
  }
  if (in_angle(90, arc->angle1, arc->angle2)) {
    obj->bounding_box.top = arc->center.y - arc->radius
      - (arc->line_width / 2.0);
  }
  if (in_angle(180, arc->angle1, arc->angle2)) {
    obj->bounding_box.left = arc->center.x - arc->radius
      - (arc->line_width / 2.0);
  }
  if (in_angle(270, arc->angle1, arc->angle2)) {
    obj->bounding_box.bottom = arc->center.y + arc->radius
      + (arc->line_width / 2.0);
  }

  obj->position = conn->endpoints[0];
  
  arc_update_handles(arc);
}

static void
arc_save(Arc *arc, ObjectNode obj_node, const char *filename)
{
  connection_save(&arc->connection, obj_node);

  if (!color_equals(&arc->arc_color, &color_black))
    data_add_color(new_attribute(obj_node, "arc_color"),
		   &arc->arc_color);
  
  if (arc->curve_distance != 0.1)
    data_add_real(new_attribute(obj_node, "curve_distance"),
		  arc->curve_distance);
  
  if (arc->line_width != 0.1)
    data_add_real(new_attribute(obj_node, "line_width"),
		  arc->line_width);
  
  if (arc->line_style != LINESTYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  arc->line_style);

  if (arc->line_style != LINESTYLE_SOLID &&
      arc->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
		  arc->dashlength);
  
  if (arc->start_arrow.type != ARROW_NONE) {
    data_add_enum(new_attribute(obj_node, "start_arrow"),
		  arc->start_arrow.type);
    data_add_real(new_attribute(obj_node, "start_arrow_length"),
		  arc->start_arrow.length);
    data_add_real(new_attribute(obj_node, "start_arrow_width"),
		  arc->start_arrow.width);
  }
  if (arc->end_arrow.type != ARROW_NONE) {
    data_add_enum(new_attribute(obj_node, "end_arrow"),
		  arc->end_arrow.type);
    data_add_real(new_attribute(obj_node, "end_arrow_length"),
		  arc->end_arrow.length);
    data_add_real(new_attribute(obj_node, "end_arrow_width"),
		  arc->end_arrow.width);
  }
}

static DiaObject *
arc_load(ObjectNode obj_node, int version, const char *filename)
{
  Arc *arc;
  Connection *conn;
  DiaObject *obj;
  AttributeNode attr;

  arc = g_malloc0(sizeof(Arc));

  conn = &arc->connection;
  obj = &conn->object;

  obj->type = &arc_type;
  obj->ops = &arc_ops;

  connection_load(conn, obj_node);

  arc->arc_color = color_black;
  attr = object_find_attribute(obj_node, "arc_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &arc->arc_color);

  arc->curve_distance = 0.1;
  attr = object_find_attribute(obj_node, "curve_distance");
  if (attr != NULL)
    arc->curve_distance = data_real(attribute_first_data(attr));

  arc->line_width = 0.1;
  attr = object_find_attribute(obj_node, "line_width");
  if (attr != NULL)
    arc->line_width = data_real(attribute_first_data(attr));

  arc->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    arc->line_style = data_enum(attribute_first_data(attr));

  arc->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    arc->dashlength = data_real(attribute_first_data(attr));


  arc->start_arrow.type = ARROW_NONE;
  arc->start_arrow.length = DEFAULT_ARROW_LENGTH;
  arc->start_arrow.width = DEFAULT_ARROW_WIDTH;
  attr = object_find_attribute(obj_node, "start_arrow");
  if (attr != NULL)
    arc->start_arrow.type = data_enum(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "start_arrow_length");
  if (attr != NULL)
    arc->start_arrow.length = data_real(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "start_arrow_width");
  if (attr != NULL)
    arc->start_arrow.width = data_real(attribute_first_data(attr));

  arc->end_arrow.type = ARROW_NONE;
  arc->end_arrow.length = DEFAULT_ARROW_LENGTH;
  arc->end_arrow.width = DEFAULT_ARROW_WIDTH;
  attr = object_find_attribute(obj_node, "end_arrow");
  if (attr != NULL)
    arc->end_arrow.type = data_enum(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "end_arrow_length");
  if (attr != NULL)
    arc->end_arrow.length = data_real(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "end_arrow_width");
  if (attr != NULL)
    arc->end_arrow.width = data_real(attribute_first_data(attr));

  connection_init(conn, 3, 0);

  obj->handles[2] = &arc->middle_handle;
  arc->middle_handle.id = HANDLE_MIDDLE;
  arc->middle_handle.type = HANDLE_MINOR_CONTROL;
  arc->middle_handle.connect_type = HANDLE_NONCONNECTABLE;
  arc->middle_handle.connected_to = NULL;

  arc_update_data(arc);

  return &arc->connection.object;
}


