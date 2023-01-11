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

 /*! \file arc.c -- Implementation of "Standard - Arc" */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>

#include "object.h"
#include "connection.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "diainteractiverenderer.h"
#include "attributes.h"
#include "arrows.h"
#include "properties.h"

#define DEFAULT_WIDTH 0.25

#define HANDLE_MIDDLE (HANDLE_CUSTOM1)
#define HANDLE_CENTER (HANDLE_CUSTOM2)

/* If you wan debug spew */
#define TRACE(fun) /* fun */

typedef struct _Arc Arc;

/*!
 * \brief Standard - Arc : a portion of the circumference of a circle
 * \extends _Connection
 * \ingroup StandardObjects
 */
struct _Arc {
  Connection connection; /*!< inheritance */

  Handle middle_handle; /*!< _Handle on the middle of the cicumference portion */
  Handle center_handle; /*!< Handle on he center of the full circle */

  Color arc_color; /*!< Color of the Arc */
  double curve_distance; /*!< distance between middle_handle and chord */
  double line_width; /*!< line width for the Arc */
  DiaLineStyle line_style; /*!< line style for the Arc */
  DiaLineCaps line_caps; /*!< line ends of the Arc */
  double dashlength; /*!< part of the linestyle if not DIA_LINE_STYLE_SOLID */
  Arrow start_arrow, end_arrow; /*!< arrows */

  /* Calculated parameters: */
  double radius;
  Point center;
  double angle1, angle2;
};

/* updates both endpoints and arc->curve_distance */
static DiaObjectChange* arc_move_handle(Arc *arc, Handle *handle,
				     Point *to, ConnectionPoint *cp,
				     HandleMoveReason reason, ModifierKeys modifiers);
static DiaObjectChange* arc_move(Arc *arc, Point *to);
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

static void arc_set_props(Arc *arc, GPtrArray *props);

static void arc_save(Arc *arc, ObjectNode obj_node, DiaContext *ctx);
static DiaObject *arc_load(ObjectNode obj_node, int version, DiaContext *ctx);
static int arc_compute_midpoint(Arc *arc, const Point * ep0, const Point * ep1 , Point * midpoint);
static void calculate_arc_object_edge(Arc *arc, real ang_start, real ang_end, DiaObject *obj, Point *target, gboolean clockwiseness);
static void arc_get_point_at_angle(Arc *arc, Point* point, real angle);
real round_angle(real angle);
real get_middle_arc_angle(real angle1, real angle2, gboolean clock);

static ObjectTypeOps arc_type_ops =
{
  (CreateFunc) arc_create,
  (LoadFunc)   arc_load,
  (SaveFunc)   arc_save,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

static PropDescription arc_props[] = {
  CONNECTION_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH_OPTIONAL,
  PROP_STD_LINE_COLOUR_OPTIONAL,
  PROP_STD_LINE_STYLE_OPTIONAL,
  PROP_STD_LINE_CAPS_OPTIONAL,
  PROP_STD_START_ARROW,
  PROP_STD_END_ARROW,
  { "curve_distance", PROP_TYPE_REAL, 0,
    N_("Curve distance"), NULL },
  PROP_DESC_END
};

static PropOffset arc_offsets[] = {
  CONNECTION_COMMON_PROPERTIES_OFFSETS,
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Arc, line_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Arc, arc_color) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Arc, line_style), offsetof(Arc, dashlength) },
  { "line_caps", PROP_TYPE_ENUM, offsetof(Arc, line_caps) },
  { "start_arrow", PROP_TYPE_ARROW, offsetof(Arc, start_arrow) },
  { "end_arrow", PROP_TYPE_ARROW, offsetof(Arc, end_arrow) },
  { "curve_distance", PROP_TYPE_REAL, offsetof(Arc, curve_distance) },
  { "start_point", PROP_TYPE_POINT, offsetof(Connection, endpoints[0]) },
  { "end_point", PROP_TYPE_POINT, offsetof(Connection, endpoints[1]) },
  { NULL, 0, 0 }
};

DiaObjectType arc_type =
{
  "Standard - Arc",   /* name */
  0,                  /* version */
  (const char **) "res:/org/gnome/Dia/objects/standard/arc.png",
  &arc_type_ops,      /* ops */
  NULL,               /* pixmap_file */
  NULL,               /* default_user_data */
  arc_props,          /* prop_descs */
  arc_offsets         /* prop_offsets */
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
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   object_describe_props,
  (GetPropsFunc)        object_get_props,
  (SetPropsFunc)        arc_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

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

/* Degenerated arc is a line */
static gboolean
arc_is_line (Arc *arc)
{
  if (fabs(arc->curve_distance) <= 0.01)
    return TRUE;
  return FALSE;
}

static real
arc_distance_from(Arc *arc, Point *point)
{
  Point *endpoints;
  Point from_center;
  real angle;
  real d, d2;

  endpoints = &arc->connection.endpoints[0];

  if (arc_is_line (arc))
    return distance_line_point (&endpoints[0], &endpoints[1],
                                arc->line_width, point);

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
  middle_pos->x = (conn->endpoints[0].x + conn->endpoints[1].x) / 2.0;
  middle_pos->y = (conn->endpoints[0].y + conn->endpoints[1].y) / 2.0;

  dx = conn->endpoints[1].x - conn->endpoints[0].x;
  dy = conn->endpoints[1].y - conn->endpoints[0].y;

  dist = sqrt(dx*dx + dy*dy);
  if (dist > 0.000001) {
    middle_pos->x -= arc->curve_distance*dy/dist;
    middle_pos->y += arc->curve_distance*dx/dist;
  }
  /* just update it from the calculated position */
  arc->center_handle.pos = arc->center;

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

static gboolean
point_projection_is_between (const Point *c,
			     const Point *a,
			     const Point *b)
{
  real len = distance_point_point (a, b);

  if (len > 0) {
    real r = ((a->y - c->y) * (a->y - b->y) - (a->x - c->x) * (b->x - a->x)) / (len * len);
    return (r >= 0 && r <= 1.0);
  }
  /* identity of three points ? */
  return (c->x == a->x && c->y == a->y);
}


static DiaObjectChange*
arc_move_handle (Arc              *arc,
                 Handle           *handle,
                 Point            *to,
                 ConnectionPoint  *cp,
                 HandleMoveReason  reason,
                 ModifierKeys      modifiers)
{
  g_return_val_if_fail (arc != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  {
    const Point *p1, *p2;

    /* A minimum distance between our three points needs to be maintained
     * Otherwise our math will get unstable with unpredictable results. */
    if (handle->id == HANDLE_MIDDLE) {
      p1 = &arc->connection.endpoints[0];
      p2 = &arc->connection.endpoints[1];
    } else if (handle->id == HANDLE_CENTER) {
      p1 = &arc->connection.endpoints[0];
      p2 = &arc->connection.endpoints[1];
      /* special movement, let the center point snap to grid */
    } else {
      p1 = &arc->middle_handle.pos;
      p2 = &arc->connection.endpoints[(handle == (&arc->connection.endpoint_handles[0])) ? 1 : 0];
    }


    if (   (distance_point_point (to, p1) < 0.01)
        || (distance_point_point (to, p2) < 0.01)) {
      return NULL;
    }
  }

  if (handle->id == HANDLE_MIDDLE) {
    TRACE (g_printerr ("curve_dist: %.2f \n", arc->curve_distance));
    arc->curve_distance = arc_compute_curve_distance (arc,
                                                      &arc->connection.endpoints[0],
                                                      &arc->connection.endpoints[1],
                                                      to);
    TRACE (g_printerr ("curve_dist: %.2f \n", arc->curve_distance));
  } else if (handle->id == HANDLE_CENTER) {
    /* We can move the handle only on the line through center and middle
     * Intersecting chord theorem says a*a=b*c
     *   with a = dist(p1,p2)/2
     *        b = curve_distance
     *        c = 2*radius-b or c = r + d
     *   with Pythagoras r^2 = d^2 + a^2
     *        d = sqrt(r^2-a^2)
     */
    Point p0 = arc->connection.endpoints[0];
    Point p1 = arc->connection.endpoints[1];
    Point p2 = { (p0.x + p1.x) / 2.0, (p0.y + p1.y) / 2.0 };
    double a = distance_point_point (&p0, &p1) / 2.0;
    double r = (distance_point_point (&p0, to) + distance_point_point (&p1, to)) / 2.0;
    double d = sqrt (r * r - a * a);
    double cd;
    /* If the new point lies between midpoint and the chords center the angles is >180,
    * so c is smaller than r */
    if (point_projection_is_between (to, &arc->middle_handle.pos, &p2)) {
      d = -d;
    }
    cd  = (a * a) / (r + d);
    /* the sign of curve_distance, i.e. if the arc angle is clockwise, does not change */
    arc->curve_distance = (arc->curve_distance > 0) ? cd : -cd;
  } else {
    Point best;
    TRACE (g_printerr ("Modifiers: %d \n", modifiers));
    if (modifiers & MODIFIER_SHIFT)
    /* if(arc->end_arrow.type == ARROW_NONE)*/
    {
      TRACE(g_printerr ("SHIFT USED, to at %.2f %.2f  ",to->x,to->y));
      if (arc_find_radial (arc, to, &best)){
        /* needs to move two handles at the same time
          * compute pos of middle handle */
        Point midpoint;
        int ok;

        if (handle == (&arc->connection.endpoint_handles[0])) {
          ok = arc_compute_midpoint (arc,
                                     &best,
                                     &arc->connection.endpoints[1],
                                     &midpoint);
        } else {
          ok = arc_compute_midpoint (arc,
                                     &arc->connection.endpoints[0],
                                     &best,
                                     &midpoint);
        }

        if (!ok) {
          return NULL;
        }

        connection_move_handle (&arc->connection,
                                handle->id,
                                &best,
                                cp,
                                reason,
                                modifiers);
        connection_adjust_for_autogap (&arc->connection);
        /* recompute curve distance equiv. move middle handle */
        arc->curve_distance = arc_compute_curve_distance (arc,
                                                          &arc->connection.endpoints[0],
                                                          &arc->connection.endpoints[1],
                                                          &midpoint);
        TRACE (g_printerr ("curve_dist: %.2f \n", arc->curve_distance));
      }
      else {
        TRACE (g_printerr ("NO best\n"));
      }
    } else {
      connection_move_handle (&arc->connection,
                              handle->id,
                              to,
                              cp,
                              reason,
                              modifiers);
      connection_adjust_for_autogap (&arc->connection);
    }
  }

  arc_update_data (arc);

  return NULL;
}


static DiaObjectChange*
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
            if (!isfinite(angle)){
                    return 0;
            }
            if (angle < -1 * M_PI){
                    TRACE(g_printerr ("angle: %.2f ",angle));
                    angle += 2*M_PI;
                    TRACE(g_printerr ("angle: %.2f ",angle));
            }
            if (angle > 1 * M_PI){
                    TRACE(g_printerr ("angle: %.2f ",angle));
                    angle -= 2*M_PI;
                    TRACE(g_printerr ("angle: %.2f ",angle));
            }

            midpos = arc->middle_handle.pos;
            /*rotate middle handle by half the angle */
            TRACE(g_printerr ("\nmidpos before: %.2f %.2f \n",midpos.x, midpos.y));
            rotate_point_around_point(&midpos, &arc->center, angle/2);
            TRACE(g_printerr ("\nmidpos after : %.2f %.2f \n",midpos.x, midpos.y));
            *midpoint = midpos;
            return 1;
}
/** updates point to the point on the arc at angle angle degrees */
void arc_get_point_at_angle(Arc *arc, Point* point, real angle)
{
        Point vec;
        vec.x = cos(angle/180.0*M_PI);
        vec.y = -sin(angle/180.0*M_PI);
        point_copy(point,&arc->center);
        point_add_scaled(point,&vec,arc->radius);
}
/** returns the angle in [0,360[ corresponding to this angle*/
real round_angle(real angle){
        real a = angle;
        while (a<0) a+=360;
        while (a>=360) a-=360;
        return a;
}
/** returns the angle in the middle from angle1 to angle2*/
real get_middle_arc_angle(real angle1, real angle2, gboolean clock)
{
        real delta;
        angle1 = round_angle(angle1);
        angle2 = round_angle(angle2);
        delta = (angle2-angle1);
        if (delta<0) delta+=360;
        if (clock)
                return round_angle(angle1-(360-delta)/2);
        else
                return round_angle(angle1+delta/2);
}

#undef TRACE_DIST
/* PRE: ang_start should be outside object.
 *      ang_end should be inside
 *      if both are inside or if ang_start is very close , then the point at ang_start is returned
 */
static void
calculate_arc_object_edge(Arc *arc, real ang_start, real ang_end, DiaObject *obj, Point *target, gboolean clockwiseness)
{
#define MAXITER 25
#ifdef TRACE_DIST
  real trace[MAXITER];
  real disttrace[MAXITER];
  int j = 0;
#endif
  real mid1, mid2, mid3;
  real dist;
  int i = 0;

  mid1 = ang_start;
  mid2 = get_middle_arc_angle(ang_start, ang_end, clockwiseness);
  mid3 = ang_end;

  TRACE(g_printerr ("Find middle angle between %f° and  %f°\n",ang_start,ang_end));
  /* If the other end is inside the object */
  arc_get_point_at_angle(arc,target,mid1);
  dist = obj->ops->distance_from(obj, target );
  if (dist < 0.001){
          TRACE(g_printerr ("Point at %f°: %f,%f is very close to object: %f, returning it\n",mid1, target->x, target->y, dist));
          return ;
  }
  do {
    arc_get_point_at_angle(arc, target, mid2);
    dist = obj->ops->distance_from(obj, target);
#ifdef TRACE_DIST
    trace[i] = mid2;
    disttrace[i] = dist;
#endif
    i++;

    if (dist < 0.0000001) {
      mid3 = mid2;
    } else {
      mid1 = mid2;
    }
    mid2 = get_middle_arc_angle(mid1,mid3,clockwiseness);

  } while (i < MAXITER && (dist < 0.0000001 || dist > 0.001));

#ifdef TRACE_DIST
    for (j = 0; j < i; j++) {
      arc_get_point_at_angle(arc,target,trace[j]);
      g_printerr ("%d: %f° : %f,%f :%f\n", j, trace[j],target->x,target->y, disttrace[j]);
    }
#endif
  arc_get_point_at_angle(arc,target,mid2);
  return ;
}

static void
arc_draw (Arc *arc, DiaRenderer *renderer)
{
  Point *endpoints;
  Point gaptmp[3];
  ConnectionPoint *start_cp, *end_cp;

  g_return_if_fail (arc != NULL);
  g_return_if_fail (renderer != NULL);

  endpoints = &arc->connection.endpoints[0];

  gaptmp[0] = endpoints[0];
  gaptmp[1] = endpoints[1];
  start_cp = arc->connection.endpoint_handles[0].connected_to;
  end_cp = arc->connection.endpoint_handles[1].connected_to;

  TRACE(g_printerr ("drawing arc:\n start:%f°:%f,%f \tend:%f°:%f,%f\n",arc->angle1,endpoints[0].x,endpoints[0].y, arc->angle2,endpoints[1].x,endpoints[1].y));

  if (connpoint_is_autogap (start_cp)) {
     TRACE (printf ("computing start intersection\ncurve_distance: %f\n", arc->curve_distance));
     if (arc->curve_distance < 0)
        calculate_arc_object_edge (arc, arc->angle1, arc->angle2, start_cp->object, &gaptmp[0], FALSE);
     else
        calculate_arc_object_edge (arc, arc->angle2, arc->angle1, start_cp->object, &gaptmp[0], TRUE);
  }
  if (connpoint_is_autogap (end_cp)) {
    TRACE(g_printerr ("computing end intersection\ncurve_distance: %f\n",arc->curve_distance));
    if (arc->curve_distance < 0)
      calculate_arc_object_edge (arc, arc->angle2, arc->angle1, end_cp->object, &gaptmp[1], TRUE);
    else
      calculate_arc_object_edge (arc, arc->angle1, arc->angle2, end_cp->object, &gaptmp[1], FALSE);
  }

  /* compute new middle_point */
  arc_compute_midpoint (arc, &gaptmp[0], &gaptmp[1], &gaptmp[2]);

  dia_renderer_set_linewidth (renderer, arc->line_width);
  dia_renderer_set_linestyle (renderer, arc->line_style, arc->dashlength);
  dia_renderer_set_linecaps (renderer, arc->line_caps);

  /* Special case when almost line: */
  if (arc_is_line (arc)) {
    TRACE (printf ("drawing like a line\n"));
    dia_renderer_draw_line_with_arrows (renderer,
                                        &gaptmp[0],
                                        &gaptmp[1],
                                        arc->line_width,
                                        &arc->arc_color,
                                        &arc->start_arrow,
                                        &arc->end_arrow);
    return;
  }

  if (   arc->start_arrow.type ==  ARROW_NONE
      && arc->end_arrow.type ==  ARROW_NONE
      && !start_cp && !end_cp) {
    /* avoid all the calculation errors and start with original arcs */
    real angle1 = arc->curve_distance > 0.0 ? arc->angle1 : arc->angle2;
    real angle2 = arc->curve_distance > 0.0 ? arc->angle2 : arc->angle1;
    /* make it direction aware */
    if (arc->curve_distance > 0.0 && angle2 < angle1) {
      angle1 -= 360.0;
    } else if (arc->curve_distance < 0.0 && angle2 > angle1) {
      angle2 -= 360.0;
    }
    dia_renderer_draw_arc (renderer,
                           &arc->center_handle.pos,
                           arc->radius*2.0,
                           arc->radius*2.0,
                           angle1,
                           angle2,
                           &arc->arc_color);
  } else {
    dia_renderer_draw_arc_with_arrows (renderer,
                                       &gaptmp[0],
                                       &gaptmp[1],
                                       &gaptmp[2],
                                       arc->line_width,
                                       &arc->arc_color,
                                       &arc->start_arrow,
                                       &arc->end_arrow);
  }

  if (DIA_IS_INTERACTIVE_RENDERER (renderer) &&
      dia_object_is_selected (&arc->connection.object)) {
    /* draw the central angle */
    Color line_color = { 0.0, 0.0, 0.6, 1.0 };

    dia_renderer_set_linewidth (renderer, 0);
    dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_DOTTED, 1);
    dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
    dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

    dia_renderer_draw_line (renderer, &endpoints[0], &arc->center, &line_color);
    dia_renderer_draw_line (renderer, &endpoints[1], &arc->center, &line_color);
  }
}

/* helper function to initialize the extra handles */
static void
_arc_setup_handles(Arc *arc)
{
  DiaObject *obj = &arc->connection.object;

  obj->handles[2] = &arc->middle_handle;
  arc->middle_handle.id = HANDLE_MIDDLE;
  arc->middle_handle.type = HANDLE_MINOR_CONTROL;
  arc->middle_handle.connect_type = HANDLE_NONCONNECTABLE;
  arc->middle_handle.connected_to = NULL;

  obj->handles[3] = &arc->center_handle;
  arc->center_handle.id = HANDLE_CENTER;
  arc->center_handle.type = HANDLE_MINOR_CONTROL;
  arc->center_handle.connect_type = HANDLE_NONCONNECTABLE;
  arc->center_handle.connected_to = NULL;
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

  arc = g_new0 (Arc, 1);
  arc->connection.object.enclosing_box = g_new0 (DiaRectangle, 1);

  arc->line_width =  attributes_get_default_linewidth();
  arc->curve_distance = 1.0;
  arc->arc_color = attributes_get_foreground();
  attributes_get_default_line_style(&arc->line_style, &arc->dashlength);
  arc->line_caps = DIA_LINE_CAPS_BUTT;
  arc->start_arrow = attributes_get_default_start_arrow();
  arc->end_arrow = attributes_get_default_end_arrow();

  conn = &arc->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  point_add(&conn->endpoints[1], &defaultlen);

  obj = &conn->object;

  obj->type = &arc_type;;
  obj->ops = &arc_ops;

  connection_init(conn, 4, 0);

  _arc_setup_handles (arc);

  arc_update_data(arc);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return &arc->connection.object;
}

static void
arc_destroy(Arc *arc)
{
  g_clear_pointer (&arc->connection.object.enclosing_box, g_free);
  connection_destroy(&arc->connection);
}

static DiaObject *
arc_copy(Arc *arc)
{
  Arc *newarc;
  Connection *conn, *newconn;
  DiaObject *newobj;

  conn = &arc->connection;

  newarc = g_new0 (Arc, 1);
  newarc->connection.object.enclosing_box = g_new0 (DiaRectangle, 1);
  newconn = &newarc->connection;
  newobj = &newconn->object;

  connection_copy(conn, newconn);

  newarc->arc_color = arc->arc_color;
  newarc->curve_distance = arc->curve_distance;
  newarc->line_width = arc->line_width;
  newarc->line_style = arc->line_style;
  newarc->line_caps = arc->line_caps;
  newarc->dashlength = arc->dashlength;
  newarc->start_arrow = arc->start_arrow;
  newarc->end_arrow = arc->end_arrow;
  newarc->radius = arc->radius;
  newarc->center = arc->center;
  newarc->angle1 = arc->angle1;
  newarc->angle2 = arc->angle2;

  newobj->handles[2] = &newarc->middle_handle;
  newarc->middle_handle = arc->middle_handle;

  newobj->handles[3] = &newarc->center_handle;
  newarc->center_handle = arc->center_handle;

  arc_update_data(arc);

  return &newarc->connection.object;
}

/* copied from lib/diarenderer.c, the one there should be removed */
static gboolean
is_right_hand (const Point *a, const Point *b, const Point *c)
{
  Point dot1, dot2;

  dot1 = *a;
  point_sub(&dot1, c);
  point_normalize(&dot1);
  dot2 = *b;
  point_sub(&dot2, c);
  point_normalize(&dot2);
  return point_cross(&dot1, &dot2) > 0;
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
  gboolean righthand;

  endpoints = &arc->connection.endpoints[0];
  x1 = endpoints[0].x;
  y1 = endpoints[0].y;
  x2 = endpoints[1].x;
  y2 = endpoints[1].y;

  lensq = (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1);
  if (fabs(arc->curve_distance) > 0.01)
    radius = lensq/(8*arc->curve_distance) + arc->curve_distance/2.0;
  else
    radius = 0.0; /* not really but used for bbox calculation below */

  if (lensq == 0.0 || arc_is_line (arc))
    alpha = 1.0; /* arbitrary, but /not/ 1/0  */
  else
    alpha = (radius - arc->curve_distance) / sqrt(lensq);

  xc = (x1 + x2) / 2.0 + (y2 - y1)*alpha;
  yc = (y1 + y2) / 2.0 + (x1 - x2)*alpha;

  angle1 = -atan2(y1-yc, x1-xc)*180.0/M_PI;
  if (angle1<0)
    angle1+=360.0;
  angle2 = -atan2(y2-yc, x2-xc)*180.0/M_PI;
  if (angle2<0)
    angle2+=360.0;

  /* swap: draw_arc is not always counter-clockwise, but our member variables
   * stay counter-clockwise to keep all the internal calculations simple
   */
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

  /* LineBBExtras not applicable to calculate the arrows bounding box */
  extra->start_trans =
  extra->end_trans   =
  extra->start_long  =
  extra->end_long    = (arc->line_width / 2.0);

  /* updates midpoint */
  arc_update_handles(arc);
  /* startpoint, midpoint, endpoint */
  righthand = is_right_hand (&endpoints[0], &arc->middle_handle.pos, &endpoints[1]);
  /* there should be no need to calculate the direction once more */
  if (!(   (righthand && arc->curve_distance <= 0.0)
        || (!righthand && arc->curve_distance >= 0.0)))
    g_warning ("Standard - Arc: check invariant!");
  connection_update_boundingbox(conn);

  /* fix boundingbox for arc's special shape XXX find a more elegant way: */
  if (in_angle(0, arc->angle1, arc->angle2)) {
    /* rigth side, y does not matter if included */
    Point pt = { arc->center.x + arc->radius + (arc->line_width / 2.0), y1 };
    rectangle_add_point (&obj->bounding_box, &pt);
  }
  if (in_angle(90, arc->angle1, arc->angle2)) {
    /* top side, x does not matter if included */
    Point pt = {x1, arc->center.y - arc->radius - (arc->line_width / 2.0) };
    rectangle_add_point (&obj->bounding_box, &pt);
  }
  if (in_angle(180, arc->angle1, arc->angle2)) {
    /* left side, y does not matter if included */
    Point pt = { arc->center.x - arc->radius - (arc->line_width / 2.0), y1 };
    rectangle_add_point (&obj->bounding_box, &pt);
  }
  if (in_angle(270, arc->angle1, arc->angle2)) {
    /* bootom side, x does not matter if included */
    Point pt = { x1, arc->center.y + arc->radius + (arc->line_width / 2.0) };
    rectangle_add_point (&obj->bounding_box, &pt);
  }
  if (arc->start_arrow.type != ARROW_NONE) {
    /* a good from-point would be the chord of arrow length, but draw_arc_with_arrows
     * currently uses the tangent For big arcs the difference is not huge and the
     * minimum size of small arcs should be limited by the arror length.
     */
    DiaRectangle bbox = {0,};
    real tmp;
    Point move_arrow, move_line;
    Point to = arc->connection.endpoints[0];
    Point from = to;
    point_sub (&from, &arc->center);
    tmp = from.x;
    if (righthand)
      from.x = -from.y, from.y = tmp;
    else
      from.x = from.y, from.y = -tmp;
    point_add (&from, &to);

    calculate_arrow_point(&arc->start_arrow, &to, &from,
                          &move_arrow, &move_line, arc->line_width);
    /* move them */
    point_sub(&to, &move_arrow);
    point_sub(&from, &move_line);
    arrow_bbox(&arc->start_arrow, arc->line_width, &to, &from, &bbox);
    rectangle_union(&obj->bounding_box, &bbox);
  }
  if (arc->end_arrow.type != ARROW_NONE) {
    DiaRectangle bbox = {0,};
    real tmp;
    Point move_arrow, move_line;
    Point to = arc->connection.endpoints[1];
    Point from = to;
    point_sub (&from, &arc->center);
    tmp = from.x;
    if (righthand)
      from.x = from.y, from.y = -tmp;
    else
      from.x = -from.y, from.y = tmp;
    point_add (&from, &to);
    calculate_arrow_point(&arc->end_arrow, &to, &from,
                          &move_arrow, &move_line, arc->line_width);
    /* move them */
    point_sub(&to, &move_arrow);
    point_sub(&from, &move_line);
    arrow_bbox(&arc->end_arrow, arc->line_width, &to, &from, &bbox);
    rectangle_union(&obj->bounding_box, &bbox);
  }
  /* if selected put the centerpoint in the box, too. */
  g_assert (obj->enclosing_box != NULL);
  *obj->enclosing_box = obj->bounding_box;
  rectangle_add_point(obj->enclosing_box, &arc->center);

  obj->position = conn->endpoints[0];
}

static void
arc_save(Arc *arc, ObjectNode obj_node, DiaContext *ctx)
{
  connection_save(&arc->connection, obj_node, ctx);

  if (!color_equals(&arc->arc_color, &color_black))
    data_add_color(new_attribute(obj_node, "arc_color"),
		   &arc->arc_color, ctx);

  if (arc->curve_distance != 0.1)
    data_add_real(new_attribute(obj_node, "curve_distance"),
		  arc->curve_distance, ctx);

  if (arc->line_width != 0.1)
    data_add_real(new_attribute(obj_node, PROP_STDNAME_LINE_WIDTH),
		  arc->line_width, ctx);

  if (arc->line_style != DIA_LINE_STYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  arc->line_style, ctx);

  if (arc->line_style != DIA_LINE_STYLE_SOLID &&
      arc->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
		  arc->dashlength, ctx);

  if (arc->line_caps != DIA_LINE_CAPS_BUTT) {
    data_add_enum (new_attribute (obj_node, "line_caps"),
                   arc->line_caps,
                   ctx);
  }

  if (arc->start_arrow.type != ARROW_NONE) {
    dia_arrow_save (&arc->start_arrow,
                    obj_node,
                    "start_arrow",
                    "start_arrow_length",
                    "start_arrow_width",
                    ctx);
  }

  if (arc->end_arrow.type != ARROW_NONE) {
    dia_arrow_save (&arc->end_arrow,
                    obj_node,
                    "end_arrow",
                    "end_arrow_length",
                    "end_arrow_width",
                    ctx);
  }
}


static DiaObject *
arc_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  Arc *arc;
  Connection *conn;
  DiaObject *obj;
  AttributeNode attr;

  arc = g_new0 (Arc, 1);
  arc->connection.object.enclosing_box = g_new0 (DiaRectangle, 1);

  conn = &arc->connection;
  obj = &conn->object;

  obj->type = &arc_type;
  obj->ops = &arc_ops;

  connection_load(conn, obj_node, ctx);

  arc->arc_color = color_black;
  attr = object_find_attribute(obj_node, "arc_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &arc->arc_color, ctx);

  arc->curve_distance = 0.1;
  attr = object_find_attribute(obj_node, "curve_distance");
  if (attr != NULL)
    arc->curve_distance = data_real(attribute_first_data(attr), ctx);

  arc->line_width = 0.1;
  attr = object_find_attribute(obj_node, PROP_STDNAME_LINE_WIDTH);
  if (attr != NULL)
    arc->line_width = data_real(attribute_first_data(attr), ctx);

  arc->line_style = DIA_LINE_STYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    arc->line_style = data_enum(attribute_first_data(attr), ctx);

  arc->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    arc->dashlength = data_real(attribute_first_data(attr), ctx);

  arc->line_caps = DIA_LINE_CAPS_BUTT;
  attr = object_find_attribute (obj_node, "line_caps");
  if (attr != NULL) {
    arc->line_caps = data_enum (attribute_first_data (attr), ctx);
  }

  dia_arrow_load (&arc->start_arrow,
                  obj_node,
                  "start_arrow",
                  "start_arrow_length",
                  "start_arrow_width",
                  ctx);

  dia_arrow_load (&arc->end_arrow,
                  obj_node,
                  "end_arrow",
                  "end_arrow_length",
                  "end_arrow_width",
                  ctx);

  connection_init(conn, 4, 0);

  _arc_setup_handles (arc);

  /* older versions did not prohibit everything reduced to a single point
   * and afterwards failed on all the calculations producing nan.
   */
  if (distance_point_point (&arc->connection.endpoints[0],
                            &arc->connection.endpoints[1]) < 0.02) {
    arc->curve_distance = 0.0;
    arc->connection.endpoints[0].x -= 0.01;
    arc->connection.endpoints[1].x += 0.01;
    arc_update_handles (arc);
  }

  arc_update_data(arc);

  return &arc->connection.object;
}



