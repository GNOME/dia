/* Dia -- an diagram creation/manipulation program
 * Support for computing bounding boxes 
 * Copyright (C) 2001 Cyrille Chepelov
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

#include <config.h>

#define _BSD_SOURCE 1
#include <math.h>
#include <string.h> /* memcmp() */

#include <glib.h>

#include "geometry.h"
#include "boundingbox.h"

/** Translates x- or y- part of bezier points to Bernstein polynom coefficients
 * @param p x- or y-part of the four points
 * @param A 
 * @param B 
 * @param C 
 * @param D 
 * See: Foley et al., Computer Graphics, Bezier Curves or
 * http://en.wikipedia.org/wiki/B%C3%A9zier_curve
 */
void
bernstein_develop(const real p[4], real *A, real *B, real *C, real *D)
{
  *A =   -p[0]+3*p[1]-3*p[2]+p[3];
  *B =  3*p[0]-6*p[1]+3*p[2];
  *C = -3*p[0]+3*p[1];
  *D =    p[0];
  /* if Q(u)=Sum(i=0..3)piBi(u) (Bi(u) being the Bernstein stuff),
     then Q(u)=Au^3+Bu^2+Cu+p[0]. */
}

/** Evaluates the Bernstein polynoms for a given position
 * @param p x- or y-values of four points describing the bezier
 * @param u position on the curve [0 .. 1]
 * @returns the evaluate x- or y-part of the point
 */
real
bezier_eval(const real p[4], real u)
{
  real A,B,C,D;
  bernstein_develop(p,&A,&B,&C,&D);
  return A*u*u*u+B*u*u+C*u+D;
}

/** Calculates the tangent for a given point on a bezier curve
 * @param p x- or y-values of four points describing the bezier
 * @param u position on the curve between[0 .. 1]
 * @return the x- or y-part of the tangent vector
 */
real
bezier_eval_tangent(const real p[4], real u)
{
  real A,B,C,D;
  bernstein_develop(p,&A,&B,&C,&D);
  return 3*A*u*u+2*B*u+C;
}  

/**
 * Calculates the extrma of the given curve in x- or y-direction.
 * @param p x- or y-values of four points describing the bezier
 * @param u The position of the extrema [0 .. 1]
 * @return The number of extrema found.
 */
static int
bicubicbezier_extrema(const real p[4],real u[2])
{
  real A,B,C,D,delta;

  bernstein_develop(p,&A,&B,&C,&D);
  delta = 4*B*B - 12*A*C;

  u[0] = u[1] = 0.0;
  if (delta<0) return 0;

  /* just a quadratic contribution? */
  if (fabs(A) < 1e-6) {
    u[0] = -C/(2*B);
    return 1;
  }

  u[0] = (-2*B + sqrt(delta)) / (6*A);
  if (delta==0) return 1;
  u[1] = (-2*B - sqrt(delta)) / (6*A);
  return 2;
}

/** Add to a bounding box the area covered by a standard arrow.
 * @param rect The bounding box to adjust
 * @param vertex The end point of the arrow.
 * @param normed_dir The normalized direction of the arrow (i.e. 1 cm in the
 * direction the arrow points from.
 * @param extra_long ???
 * @param extra_trans ???
 */
static void
add_arrow_rectangle(Rectangle *rect,
                    const Point *vertex,
                    const Point *normed_dir,
                    real extra_long,real extra_trans)
{
  Point vl,vt,pt;
  vl = *normed_dir;

  point_get_perp(&vt,&vl);
  point_copy_add_scaled(&pt,vertex,&vl,extra_long);
  point_add_scaled(&pt,&vt,extra_trans);
  rectangle_add_point(rect,&pt);
  point_add_scaled(&pt,&vt,-2.0 * extra_trans);
  rectangle_add_point(rect,&pt);
  point_add_scaled(&pt,&vl,-2.0 * extra_long);
  rectangle_add_point(rect,&pt);
  point_add_scaled(&pt,&vt,2.0 * extra_trans);
  rectangle_add_point(rect,&pt);  
}

/** Calculate the boundingbox for a 2D bezier curve segment.
 * @param p0 start point
 * @param p1 1st control point
 * @param p2 2nd control point
 * @param p3 end point
 * @param extra information about extra space from linewidth and arrow to add to the bounding box
 * @param rect The rectangle that the segment fits inside.
 */
void
bicubicbezier2D_bbox(const Point *p0,const Point *p1,
                     const Point *p2,const Point *p3,
                     const PolyBBExtras *extra,
                     Rectangle *rect)
{
  real x[4],y[4];
  Point vl,vt,p,tt;
  real *xy;
  int i,extr;
  real u[2];

  rect->left = rect->right = p0->x;
  rect->top = rect->bottom = p0->y;

  rectangle_add_point(rect,p3);
  /* start point */  
  point_copy_add_scaled(&vl,p0,p1,-1);
  if (point_len(&vl) == 0)
    point_copy_add_scaled(&vl,p0,p2,-1);
  point_normalize(&vl); 
  add_arrow_rectangle(rect,p0,&vl,extra->start_long,MAX(extra->start_trans,
                                                         extra->middle_trans));

  /* end point */
  point_copy_add_scaled(&vl,p3,p2,-1);
  if (point_len(&vl) == 0)
    point_copy_add_scaled(&vl,p3,p1,-1);
  point_normalize(&vl); 
  add_arrow_rectangle(rect,p3,&vl,extra->end_long,MAX(extra->end_trans,
                                                      extra->middle_trans));

  /* middle part */  
  x[0] = p0->x; x[1] = p1->x; x[2] = p2->x; x[3] = p3->x;
  y[0] = p0->y; y[1] = p1->y; y[2] = p2->y; y[3] = p3->y;

  for (xy = x; xy ; xy=(xy==x?y:NULL) ) { /* sorry */
    extr = bicubicbezier_extrema(xy,u);
    for (i=0;i<extr;i++) {
      if ((u[i]<0) || (u[i]>1)) continue;
      p.x = bezier_eval(x,u[i]);
      vl.x = bezier_eval_tangent(x,u[i]);
      p.y = bezier_eval(y,u[i]);
      vl.y = bezier_eval_tangent(y,u[i]);
      point_normalize(&vl);
      point_get_perp(&vt,&vl);

      point_copy_add_scaled(&tt,&p,&vt,extra->middle_trans);
      rectangle_add_point(rect,&tt);
      point_copy_add_scaled(&tt,&p,&vt,-extra->middle_trans);
      rectangle_add_point(rect,&tt);
    }
  }
}

/** Calculate the bounding box for a simple line.
 * @param p1 One end of the line.
 * @param p2 The other end of the line.
 * @param extra Extra information 
 * @param rect The box that the line and extra stuff fits inside.
 */
void
line_bbox(const Point *p1, const Point *p2,
          const LineBBExtras *extra,
          Rectangle *rect)
{
  Point vl;
  
  rect->left = rect->right = p1->x;
  rect->top = rect->bottom = p1->y;

  rectangle_add_point(rect,p2); /* as a safety, so we don't need to care if it above or below p1 */
  
  point_copy_add_scaled(&vl,p1,p2,-1);
  point_normalize(&vl);
  add_arrow_rectangle(rect,p1,&vl,extra->start_long,extra->start_trans);
  point_scale(&vl,-1);
  add_arrow_rectangle(rect,p2,&vl,extra->end_long,extra->end_trans);
}

/** Calculate the bounding box of an ellipse.
 * @param centre The center point of the ellipse.
 * @param width The width of the ellipse.
 * @param height The height of the ellipse.
 * @param extra Extra information required.
 * @param rect The bounding box that the ellipse fits inside.
 */
void
ellipse_bbox(const Point *centre, real width, real height,
             const ElementBBExtras *extra,
             Rectangle *rect)
{
  Rectangle rin;
  rin.left = centre->x - width/2;
  rin.right = centre->x + width/2;
  rin.top = centre->y - height/2;
  rin.bottom = centre->y + height/2;

  rectangle_bbox(&rin,extra,rect);
}

/**  Allocate some scratch space to hold a big enough Bezier. 
 * That space is not guaranteed to be preserved upon the next allocation
 * (in fact it's guaranteed it's not).
 * @param numpoints How many points of bezier to allocate space for.
 * @returns Newly allocated array of points.
 */
static BezPoint *
alloc_polybezier_space(int numpoints)
{
  static int alloc_np = 0; 
  static BezPoint *alloced = NULL;
  
  if (alloc_np < numpoints) {
    g_free(alloced);
    alloc_np = numpoints;
    alloced = g_new0(BezPoint,numpoints);
  }
  return alloced;
}

/** Free the scratch space allocated above.
 * @param points Previously allocated list of points.
 * @note Doesn't actually free it, as alloc_polybezier_space does that.
 */
static void
free_polybezier_space(BezPoint *points)
{ /* dummy */ }

/** Calculate the boundingbox for a polyline.
 * @param pts Array of points.
 * @param numpoints Number of elements in `pts'.
 * @param extra Extra space information
 * @param closed Whether the polyline is closed or not.
 * @param rect Return value: The bounding box that includes the points and
 * extra spacing.
 */
void
polyline_bbox(const Point *pts, int numpoints, 
              const PolyBBExtras *extra, gboolean closed,
              Rectangle *rect)
{
  /* It's much easier to re-use the Bezier code... */
  int i;
  BezPoint *bpts = alloc_polybezier_space(numpoints + 1);

  bpts[0].type = BEZ_MOVE_TO;
  bpts[0].p1 = pts[0];

  for (i=1;i<numpoints;i++) {
    bpts[i].type = BEZ_LINE_TO;
    bpts[i].p1 = pts[i];
  }
  /* This one will be used only when closed==TRUE... */
  bpts[numpoints].type = BEZ_LINE_TO;
  bpts[numpoints].p1 = pts[0];
  
  polybezier_bbox(bpts,numpoints + (closed?1:0),extra,closed,rect);
  free_polybezier_space(bpts);
}

/** Calculate a bounding box for a set of bezier points.
 * @param pts The bezier points
 * @param numpoints The number of elements in `pts'
 * @param extra Extra spacing information.
 * @param closed True if the bezier points form a closed line.
 * @param rect Return value: The enclosing rectangle will be stored here.
 */
void 
polybezier_bbox(const BezPoint *pts, int numpoints,
                const PolyBBExtras *extra, gboolean closed,            
                Rectangle *rect)
{
  Point vx,vn,vsc,vp;
  int i,prev,next;
  Rectangle rt;
  PolyBBExtras bextra,start_bextra,end_bextra,full_bextra;
  LineBBExtras lextra,start_lextra,end_lextra,full_lextra;
  gboolean start,end;

  vp.x=0;
  vp.y=0;

  g_assert(pts[0].type == BEZ_MOVE_TO);

  rect->left = rect->right = pts[0].p1.x;
  rect->top = rect->bottom = pts[0].p1.y;

  /* First, we build derived BBExtras structures, so we have something to feed 
     the primitives. */ 
  if (!closed) {
    start_lextra.start_long = extra->start_long;
    start_lextra.start_trans = MAX(extra->start_trans,extra->middle_trans);
    start_lextra.end_long = 0;
    start_lextra.end_trans = extra->middle_trans;

    end_lextra.start_long = 0;
    end_lextra.start_trans = extra->middle_trans;
    end_lextra.end_long = extra->end_long;
    end_lextra.end_trans = MAX(extra->end_trans,extra->middle_trans);
  }

  full_lextra.start_long = extra->start_long;
  full_lextra.start_trans = MAX(extra->start_trans,extra->middle_trans);
  full_lextra.end_long = extra->end_long;
  full_lextra.end_trans = MAX(extra->end_trans,extra->middle_trans);
  full_bextra.start_long = extra->start_long;
  full_bextra.start_trans = MAX(extra->start_trans,extra->middle_trans);
  full_bextra.middle_trans = extra->middle_trans;
  full_bextra.end_long = extra->end_long;
  full_bextra.end_trans = MAX(extra->end_trans,extra->middle_trans);

  if (!closed) {
    lextra.start_long = 0;
    lextra.start_trans = extra->middle_trans;
    lextra.end_long = 0;
    lextra.end_trans = extra->middle_trans;

    start_bextra.start_long = extra->start_long;
    start_bextra.start_trans = extra->start_trans;
    start_bextra.middle_trans = extra->middle_trans;
    start_bextra.end_long = 0;
    start_bextra.end_trans = extra->middle_trans;

    end_bextra.start_long = 0;
    end_bextra.start_trans = extra->middle_trans;
    end_bextra.middle_trans = extra->middle_trans;
    end_bextra.end_long = extra->end_long;
    end_bextra.end_trans = extra->end_trans;
  }

  bextra.start_long = 0;
  bextra.start_trans = extra->middle_trans;
  bextra.middle_trans = extra->middle_trans;
  bextra.end_long = 0;
  bextra.end_trans = extra->middle_trans;        


  for (i=1;i<numpoints;i++) {
    next = (i+1) % numpoints;
    prev = (i-1) % numpoints;
    if (closed && (next == 0)) next=1;
    if (closed && (prev == 0)) prev=numpoints-1;

    /* We have now: 
       i = index of current vertex. 
       prev,next: index of previous/next vertices (of the control polygon) 

       We want:
        vp, vx, vn: the previous, current and next vertices;
        start, end: TRUE if we're at an end of poly (then, vp and/or vn are not
        valid, respectively).

       Some values *will* be recomputed a few times across iterations (but stored in 
       different boxes). Either gprof says it's a real problem, or gcc finally gets 
       a clue.
    */

    if (pts[i].type == BEZ_MOVE_TO) {
      continue;
    }

    switch(pts[i].type) {
    case BEZ_MOVE_TO:
      g_assert_not_reached();
      break;
    case BEZ_LINE_TO:
      point_copy(&vx,&pts[i].p1);
      switch(pts[prev].type) {
      case BEZ_MOVE_TO:
      case BEZ_LINE_TO:
        point_copy(&vsc,&pts[prev].p1);
        point_copy(&vp,&pts[prev].p1);
        break;
      case BEZ_CURVE_TO:
        point_copy(&vsc,&pts[prev].p3);
        point_copy(&vp,&pts[prev].p3);
        break;
      }
      break;
    case BEZ_CURVE_TO:
      point_copy(&vx,&pts[i].p3);
      point_copy(&vp,&pts[i].p2);
      switch(pts[prev].type) {
      case BEZ_MOVE_TO:
      case BEZ_LINE_TO:
        point_copy(&vsc,&pts[prev].p1);
        break;
      case BEZ_CURVE_TO:
        point_copy(&vsc,&pts[prev].p3);
        break;
      } /* vsc is the start of the curve. */
      
      break;
    }
    start = (pts[prev].type == BEZ_MOVE_TO);
    end = (pts[next].type == BEZ_MOVE_TO);
    point_copy(&vn,&pts[next].p1); /* whichever type pts[next] is. */

    /* Now, we know about a few vertices around the one we're dealing with.
       Depending on the shape of the (previous,current) segment, and whether 
       it's a middle or end segment, we'll be doing different stuff. */ 
    if (closed) {
      if (pts[i].type == BEZ_LINE_TO) {
        line_bbox(&vsc,&vx,&full_lextra,&rt);
      } else {
        bicubicbezier2D_bbox(&vsc,
                             &pts[i].p1,&pts[i].p2,&pts[i].p3,
                             &bextra,
                             &rt);
      }    
    } else if (start) {
      if (pts[i].type == BEZ_LINE_TO) {
        if (end) {
          line_bbox(&vsc,&vx,&full_lextra,&rt);
        } else {
          line_bbox(&vsc,&vx,&start_lextra,&rt);
        }
      } else { /* BEZ_MOVE_TO */ 
        if (end) {
          bicubicbezier2D_bbox(&vsc,
                               &pts[i].p1,&pts[i].p2,&pts[i].p3,
                               &full_bextra,
                               &rt);
        } else {
          bicubicbezier2D_bbox(&vsc,
                               &pts[i].p1,&pts[i].p2,&pts[i].p3,
                               &start_bextra,
                               &rt);
        }
      }
    } else if (end) { /* end but not start. Not closed anyway. */
      if (pts[i].type == BEZ_LINE_TO) {
        line_bbox(&vsc,&vx,&end_lextra,&rt);
      } else {
        bicubicbezier2D_bbox(&vsc,
                             &pts[i].p1,&pts[i].p2,&pts[i].p3,
                             &end_bextra,
                             &rt);
      } 
    } else { /* normal case : middle segment (not closed shape). */
      if (pts[i].type == BEZ_LINE_TO) {
        line_bbox(&vsc,&vx,&lextra,&rt);
      } else {
        bicubicbezier2D_bbox(&vsc,
                             &pts[i].p1,&pts[i].p2,&pts[i].p3,
                             &bextra,
                             &rt);
      } 
    }   
    rectangle_union(rect,&rt);

    /* The following code enlarges a little the bounding box (if necessary) to 
       account with the "pointy corners" X (and PS) add when LINEJOIN_MITER mode is 
       in force. */

    if (!end) { /* only the last segment might not produce overshoot. */
      Point vpx,vxn;
      real co,alpha;

      point_copy_add_scaled(&vpx,&vx,&vp,-1);
      point_normalize(&vpx);
      point_copy_add_scaled(&vxn,&vn,&vx,-1);
      point_normalize(&vxn);

      co = point_dot(&vpx,&vxn);
      alpha = dia_acos(-co);
      if (co > -0.9816) { /* 0.9816 = cos(11deg) */
        /* we have a pointy join. */
        real overshoot;
        Point vovs,pto;

	if (alpha > 0.0 && alpha < M_PI)
	  overshoot = extra->middle_trans / sin(alpha/2.0);
	else /* prependicular? */
	  overshoot = extra->middle_trans;

        point_copy_add_scaled(&vovs,&vpx,&vxn,-1);
        point_normalize(&vovs);
        point_copy_add_scaled(&pto,&vx,&vovs,overshoot);
        
        rectangle_add_point(rect,&pto);
      } else {
        /* we don't have a pointy join. */
#if 0
	/* so nothing to do really - this code would be growing the
	 * bounding box arbitrarily. See e.g with bezier-extreme.dia
	 */
        Point vpxt,vxnt,tmp;

        point_get_perp(&vpxt,&vpx);
        point_get_perp(&vxnt,&vxn);
        
        point_copy_add_scaled(&tmp,&vx,&vpxt,1);
        rectangle_add_point(rect,&tmp);
        point_copy_add_scaled(&tmp,&vx,&vpxt,-1);
        rectangle_add_point(rect,&tmp);
        point_copy_add_scaled(&tmp,&vx,&vxnt,1);
        rectangle_add_point(rect,&tmp);
        point_copy_add_scaled(&tmp,&vx,&vxnt,-1);
        rectangle_add_point(rect,&tmp);
#endif
      }
    }
  }
}

/** Figure out a bounding box for a rectangle (fairly simple:)
 * @param rin A rectangle to find bbox for.
 * @param extra Extra information required to find bbox.
 * @param rout Return value: The enclosing bounding box.
 */
void 
rectangle_bbox(const Rectangle *rin,
	       const ElementBBExtras *extra,
	       Rectangle *rout)
{
  rout->left = rin->left - extra->border_trans;
  rout->top = rin->top - extra->border_trans;
  rout->right = rin->right + extra->border_trans;
  rout->bottom = rin->bottom + extra->border_trans;
}

