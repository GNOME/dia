/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * path-math.c -- some helper function for binary path operations
 * Copyright (C) 2014, Hans Breuer <Hans@Breuer.Org>
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

#include "geometry.h"
#include "boundingbox.h"
#include "path-math.h"

#include <string.h> /* memcmp() */

/*!
 * \brief Take a path and calculate it to non overlapping pieces
 */

typedef struct _BezierSegment BezierSegment;
struct _BezierSegment {
  Point p0;
  Point p1;
  Point p2;
  Point p3;
};

/*!
 * \brief Split a bezier segment into left and right half
 */
static void
bezier_split (const BezierSegment *a,
	      BezierSegment *a1,
	      BezierSegment *a2)
{
  /* see: Foley et al., Computer Graphics, p508 */
  Point L2, L3, L4, H, R2, R3;
  /* P1 = L1 */
  a1->p0 = a->p0;
  /* L2 = (P1 + P2) / 2 */
  L2.x = (a->p0.x + a->p1.x) / 2;
  L2.y = (a->p0.y + a->p1.y) / 2;
  /* H = (P2 + P3) / 2 */
  H.x = (a->p1.x + a->p2.x) / 2;
  H.y = (a->p1.y + a->p2.y) / 2;
  /* R3 = (P3 + P4) / 2 */
  R3.x = (a->p2.x + a->p3.x) / 2;
  R3.y = (a->p2.y + a->p3.y) / 2;
  /* L3 = (L2 + H) / 2 */
  L3.x = (L2.x + H.x) / 2;
  L3.y = (L2.y + H.y) / 2;
  /* R2 = (H + R3) / 2 */
  R2.x = (H.x + R3.x) / 2;
  R2.y = (H.y + R3.y) / 2;

  a1->p1 = L2;
  a1->p2 = L3;
  /* L4 = R1 = (L3 + R2) / 2 */
  L4.x = (L3.x + R2.x) / 2;
  L4.y = (L3.y + R2.y) / 2;
  a1->p3 = a2->p0 = L4;
  a2->p1 = R2;
  a2->p2 = R3;
  /* P4 = R4 */
  a2->p3 = a->p3;
}

static void
bezier_split_at (const BezierSegment *a,
		 BezierSegment *a1,
		 BezierSegment *a2,
		 real split)
{
  real left = 1.0 - split;
  real right = split;
  /* see: Foley et al., Computer Graphics, p508 */
  Point L2, L3, L4, H, R2, R3;
  /* P1 = L1 */
  a1->p0 = a->p0;
  /* L2 = (P1 + P2) / 2 */
  L2.x = (a->p0.x * left + a->p1.x * right);
  L2.y = (a->p0.y * left + a->p1.y * right);
  /* H = (P2 + P3) / 2 */
  H.x = (a->p1.x * left + a->p2.x * right);
  H.y = (a->p1.y * left + a->p2.y * right);
  /* R3 = (P3 + P4) / 2 */
  R3.x = (a->p2.x * left + a->p3.x * right);
  R3.y = (a->p2.y * left + a->p3.y * right);
  /* L3 = (L2 + H) / 2 */
  L3.x = (L2.x * left + H.x * right);
  L3.y = (L2.y * left + H.y * right);
  /* R2 = (H + R3) / 2 */
  R2.x = (H.x * left + R3.x * right);
  R2.y = (H.y * left + R3.y * right);

  a1->p1 = L2;
  a1->p2 = L3;
  /* L4 = R1 = (L3 + R2) / 2 */
  L4.x = (L3.x * left + R2.x * right);
  L4.y = (L3.y * left + R2.y * right);
  a1->p3 = a2->p0 = L4;
  a2->p1 = R2;
  a2->p2 = R3;
  /* P4 = R4 */
  a2->p3 = a->p3;
}

typedef struct _Intersection Intersection;
struct _Intersection {
  Point pt;        /*!< the crossing point */
  real  split_one; /*!< 0..1: relative placement on segment one */
  real  split_two; /*!< 0..1: relative placement on segment two */
  int   seg_one;   /*!< index of the segment */
  int   seg_two;   /*!< index of the segment */
};

static gboolean
_segment_has_point (const BezierSegment *bs,
		    const Point *pt)
{
  BezPoint bp;
  real dist;
  bp.type = BEZ_CURVE_TO;
  bp.p1 = bs->p1;
  bp.p2 = bs->p2;
  bp.p3 = bs->p3;
  dist = distance_bez_seg_point (&bs->p0, &bp, 0, pt);

  return dist <= 0.0;
}

static gboolean
_segment_is_moveto (const BezierSegment *bs)
{
  if (   memcmp (&bs->p0, &bs->p1, sizeof (Point)) == 0
      && memcmp (&bs->p0, &bs->p2, sizeof (Point)) == 0
      && memcmp (&bs->p0, &bs->p3, sizeof (Point)) == 0)
    return TRUE;
  return FALSE;
}
static gboolean
_segment_is_lineto (const BezierSegment *bs)
{
  if (   memcmp (&bs->p0, &bs->p1, sizeof (Point)) != 0 /* not move-to */
      && memcmp (&bs->p1, &bs->p2, sizeof (Point)) == 0
      && memcmp (&bs->p1, &bs->p3, sizeof (Point)) == 0)
    return TRUE;
  return FALSE;
}

/* search precision */
static const real EPSILON = 0.0001;

/*!
 * \brief Calculate crossing points of two bezier segments
 *
 * Beware two bezier segments can intersect more than once, but this
 * function only returns the first or no intersection. It is the
 * responsibility of the caller to further split segments until there
 * is no intersection left.
 */
static gboolean
bezier_bezier_intersection (GArray              *crossing,
                            const BezierSegment *a,
                            const BezierSegment *b,
                            int                  depth,
                            real                 asplit,
                            real                 bsplit)
{
  DiaRectangle abox, bbox;
  PolyBBExtras extra = { 0, };
  gboolean small_a, small_b;

  /* Avoid intersection overflow: if start and end are on the other segment
   * assume full overlap and no crossing.
   */
  if (   (_segment_has_point (a, &b->p0) && _segment_has_point (a, &b->p3))
      || (_segment_has_point (b, &a->p0) && _segment_has_point (b, &a->p3)))
    return FALSE; /* XXX: more variants pending, partial overlap */

  /* With very similar segments we would create a lot of points with not
   * a very deep recursion (test with ying-yang symbol).
   * Just comparing the segments on depth=1 is not good enough, so for
   * now we are limiting the number of intersections
   */
  if (crossing->len > 127) { /* XXX: arbitrary limit */
    g_warning ("Crossing limit (%d) reached", crossing->len);
    return FALSE;
  }

  bicubicbezier2D_bbox (&a->p0, &a->p1, &a->p2, &a->p3, &extra, &abox);
  bicubicbezier2D_bbox (&b->p0, &b->p1, &b->p2, &b->p3, &extra, &bbox);

  if (!rectangle_intersects (&abox, &bbox))
    return FALSE;
  small_a = (abox.right - abox.left) < EPSILON && (abox.bottom - abox.top) < EPSILON;
  small_b = (bbox.right - bbox.left) < EPSILON && (bbox.bottom - bbox.top) < EPSILON;
  /* if the boxes are small enough we can calculate the point */
  if (small_a && small_b) {
    /* intersecting and both small, should not matter which one is used */
    Point pt = { (abox.right + abox.left + bbox.right + bbox.left) / 4,
		 (abox.bottom + abox.top + bbox.bottom + bbox.top) / 4 };
    Intersection is;
    int i;

    for (i = 0; i < crossing->len; ++i) {
      /* if it's already included we are done */
      if (distance_point_point (&g_array_index (crossing, Intersection, i).pt, &pt) < 1.4142*EPSILON)
        return TRUE; /* although we did not add it */
    }
    is.split_one = asplit;
    is.split_two = bsplit;
    is.pt = pt;
    g_print ("d=%d; as=%g; bs=%g; ", depth, asplit, bsplit);
    g_array_append_val (crossing, is);
    return TRUE;
  } else {
    /* further splitting of a and b; it could be smart to only search in the
     * intersection of a-box and b-box ... */
    BezierSegment a1, a2;
    BezierSegment b1, b2;
    real ofs = 1.0/(1<<(depth+1));
    gboolean ret = FALSE;

    bezier_split (a, &a1, &a2);
    bezier_split (b, &b1, &b2);

    ret |= bezier_bezier_intersection (crossing, &a1, &b1, depth+1, asplit-ofs, bsplit-ofs);
    ret |= bezier_bezier_intersection (crossing, &a2, &b1, depth+1, asplit+ofs, bsplit-ofs);
    ret |= bezier_bezier_intersection (crossing, &a1, &b2, depth+1, asplit-ofs, bsplit+ofs);
    ret |= bezier_bezier_intersection (crossing, &a2, &b2, depth+1, asplit+ofs, bsplit+ofs);
    /* XXX: check !ret case, not sure if it should happen */
    return ret;
  }
}


static gboolean
_segment_from_path (BezierSegment *a, const GArray *p1, int i)
{
  const BezPoint *abp0 = &g_array_index (p1, BezPoint, i-1);
  const BezPoint *abp1 = &g_array_index (p1, BezPoint, i);

  a->p0 = abp0->type == BEZ_CURVE_TO ? abp0->p3 : abp0->p1;

  switch (abp1->type) {
    case BEZ_CURVE_TO :
      a->p1 = abp1->p1; a->p2 = abp1->p2; a->p3 = abp1->p3;
      break;
    case BEZ_LINE_TO :
      if (distance_point_point (&a->p0, &abp1->p1) < EPSILON)
        return FALSE; /* avoid a zero length line-to for confusion with move-to */
      a->p1 = a->p2 = a->p3 = abp1->p1;
      break;
    case BEZ_MOVE_TO :
      a->p0 = a->p1 = a->p2 = a->p3 = abp1->p1;
      break;
    default:
      g_return_val_if_reached (FALSE);
  }

  return TRUE;
}


static void
_curve_from_segment (BezPoint *bp, const BezierSegment *a, gboolean flip)
{
  if (_segment_is_moveto (a))
    bp->type = BEZ_MOVE_TO;
  else if (_segment_is_lineto (a))
    bp->type = BEZ_LINE_TO;
  else
    bp->type = BEZ_CURVE_TO;
  if (!flip) {
    bp->p1 = a->p1;
    bp->p2 = a->p2;
    bp->p3 = a->p3;
  } else {
    if (bp->type != BEZ_CURVE_TO) {
      bp->p1 = bp->p2 = bp->p3 = a->p0;
    } else {
      bp->p1 = a->p2;
      bp->p2 = a->p1;
      bp->p3 = a->p0;
    }
  }
}

typedef struct _Split Split;
struct _Split {
  Point    pt;      /*!< the position of the split */
  int      seg;     /*!< the index of the segment to split */
  real     split;   /*!< 0..1: relative placement on segment */
  gboolean used;    /*!< marked during _make_path() */
  gboolean outside; /*!< not inside the other path */
  GArray  *path;    /*!< subpath copy */
};

/*!
 * \brief Extract splits from crossing
 *
 * Crossing is the array of Intersection which contains split information
 * from crossing between two paths. This function separates the
 * information into splits specific to a single path.
 */
static GArray *
_extract_splits (const GArray *crossing, gboolean one)
{
  GArray *result = g_array_new (FALSE, FALSE, sizeof(Split));
  int i;
  for (i = 0; i < crossing->len; ++i) {
    Split sp = { { 0, 0 }, 0 };
    sp.pt = g_array_index (crossing, Intersection, i).pt;
    if (one) {
      sp.seg = g_array_index (crossing, Intersection, i).seg_one;
      sp.split = g_array_index (crossing, Intersection, i).split_one;
    } else {
      sp.seg = g_array_index (crossing, Intersection, i).seg_two;
      sp.split = g_array_index (crossing, Intersection, i).split_two;
    }
    sp.used = FALSE;
    g_array_append_val (result, sp);
  }
  return result;
}

static GArray *
_path_to_segments (const GArray *path)
{
  GArray *segs = g_array_new (FALSE, FALSE, sizeof(BezierSegment));
  BezierSegment bs;
  int i;
  BezPoint *last_move = &g_array_index (path, BezPoint, 0);

  for (i = 1; i < path->len; ++i) {
    if (g_array_index (path, BezPoint, i).type == BEZ_MOVE_TO)
      last_move = &g_array_index (path, BezPoint, i);
    if (_segment_from_path (&bs, path, i))
      g_array_append_val (segs, bs);
  }
  /* if the path is not closed do an explicit line-to */
  if (distance_point_point (&last_move->p1, &bs.p3) < EPSILON) {
    /* if the error is small enough just modify the last point */
    BezierSegment *e = &g_array_index (segs, BezierSegment, segs->len - 1);
    if (_segment_is_lineto (e))
      e->p1 = e->p2 = e->p3 = last_move->p1;
    else
      e->p3 = last_move->p1;
  } else {
    bs.p0 = bs.p3;
    bs.p1 = bs.p2 = bs.p3 = last_move->p1;
    g_array_append_val (segs, bs);
  }

  return segs;
}

/* GCompareFunc to sort Split */
static gint
_compare_split (gconstpointer as, gconstpointer bs)
{
  const Split *a = as;
  const Split *b = bs;
  if (a->seg > b->seg)
    return 1;
  if (a->seg < b->seg)
    return -1;
  if (a->split > b->split)
    return 1;
  if (a->split < b->split)
    return -1;
  return 0;
}

/*!
 * Given the original segments and splits apply
 * all segment splits and create unique segment index.
 *
 * Split.seg is the index to the segment to split before this function.
 * After the splits are applied every split.seq is unique.
 */
static void
_split_segments (GArray *segs, GArray *splits, const GArray *other)
{
  int i, sofs = 0;
  GArray *pending;

  /* splits must be sorted for the algorithm below */
  g_array_sort (splits, _compare_split);

  for (i = 0; i < splits->len; ++i) {
    int j, to;
    int from = i;
    int from_seg = g_array_index (splits, Split, i).seg;
    BezierSegment bs;
    real t = 0;

    g_return_if_fail (from_seg + sofs < segs->len);
    bs = g_array_index (segs, BezierSegment, from_seg + sofs);
    while (i < splits->len - 1 && from_seg == g_array_index (splits, Split, i+1).seg)
      ++i; /* advance while segment reference is the same */
    to = i;
    for (j = from; j <= to; j++) {
      BezierSegment left, right;
      /* scale t to split the right segment */
      real tL = g_array_index (splits, Split, j).split;
      real tR = (tL - t) / (1.0 - t);
      bezier_split_at (&bs, &left, &right, tR);
      bs = right;
      t = tL;
      /* overwrite the exisiting */
      g_return_if_fail (from_seg + sofs < segs->len);
      g_array_index (segs, BezierSegment, from_seg + sofs) = left;
      sofs += 1; /* increment segment offset for every segment added */
      /* insert a new one behind that ... */
      g_array_insert_val (segs, from_seg + sofs, right); /* ... potentially overwritten */
      /* adjust the segment reference */
      g_array_index (splits, Split, j).seg = from_seg + sofs;
    }
  }
  pending = g_array_new (FALSE, FALSE, sizeof(BezierSegment));
  /* for every sub-path determine if it is inside the full other path */
  for (i = 0; i < splits->len; ++i) {
    Split *sp = &g_array_index (splits, Split, i);
    BezierSegment *bs = &g_array_index (segs, BezierSegment, sp->seg);
    BezierSegment left, right;
    int to, j;

    if (i == 0 && sp->seg > 0)
      g_array_append_vals (pending, &g_array_index (segs, BezierSegment, 0), sp->seg);

    bezier_split (bs, &left, &right);
    sp->outside = distance_bez_shape_point (&g_array_index (other, BezPoint, 0), other->len,
					   0 /* line width */, &right.p0) > 0.0;
    /* also remember the sub-path */
    to = g_array_index (splits, Split, (i+1)%splits->len).seg;
    sp->path = g_array_new (FALSE, FALSE, sizeof(BezierSegment));
    if (to < sp->seg) {
      g_array_append_vals (sp->path, bs, segs->len - sp->seg);
#if 0
      /* XXX: this is only correct if there is no move-to within the segments */
      g_array_append_vals (sp->path, &g_array_index (segs, BezierSegment, 0), to);
#else
      g_array_append_vals (sp->path, &g_array_index (pending, BezierSegment, 0), pending->len);
      g_array_set_size (pending, 0);
#endif
    } else {
      for (j = sp->seg; j < to; ++j) {
	if (_segment_is_moveto (bs)) {
	  g_array_append_vals (sp->path, &g_array_index (pending, BezierSegment, 0), pending->len);
	  g_array_set_size (pending, 0);
	  break;
	}
	g_array_append_val (sp->path, *bs);
	bs++;
      }
      for (/* remains */; j < to; ++j) {
	g_array_append_val (pending, *bs);
	bs++;
      }
    }
  }
  g_array_free (pending, TRUE);
}

static void
_free_splits (GArray *splits)
{
  int i;

  g_return_if_fail (splits != NULL);

  for (i = 0; i < splits->len; ++i) {
    Split *sp = &g_array_index (splits, Split, i);
    if (sp->path)
      g_array_free (sp->path, TRUE);
  }
  g_array_free (splits, TRUE);
}

static GArray *
_find_intersections (GArray *one, GArray *two)
{
  GArray *crossing = g_array_new (FALSE, FALSE, sizeof(Intersection));
  int i, j, k;

  /* find intersections */
  for (i = 0; i < one->len; ++i) {
    BezierSegment a = g_array_index (one, BezierSegment, i);
    for (j = 0; j < two->len; ++j) {
      BezierSegment b = g_array_index (two, BezierSegment, j);
      int start = crossing->len;
      if (bezier_bezier_intersection (crossing, &a, &b, 1, 0.5, 0.5)) {
	/* Found intersection splits a and b into left and right.
	 * Any segment might be split more than once so seg_one and seg_two
	 * are not unique yet. Also the calculated split always refers to
	 * the whole segment. We could avoid the _split_segments() below by
	 * modifying `one� and `two� in place. But instead of later
	 * segmentation that would complicate the seq_* reference _and_ give
	 * worse runtime behavior because we would need to start over with
	 * every intersection found.
	 */
	for (k = start; k < crossing->len; ++k) {
	  Intersection *is = &g_array_index (crossing, Intersection, k);
	  is->seg_one = i;
	  is->seg_two = j;
	}
        g_print ("with a:b %d:%d\n", i, j);
      }
    }
  }

  if (crossing->len > 0)
    return crossing;
  g_array_free (crossing, TRUE);
  return NULL;
}

/*!
 * \brief Find the next sub path to connect
 *
 * Ignores the crossing point of the Split, but just looks at the
 * start and end of the given sub path.
 */
static gboolean
_find_split (GArray *splits, Point *pt, gboolean outside, Split **next)
{
  int i;

  for (i = 0; i < splits->len; ++i) {
    Split *sp = &g_array_index (splits, Split, i);
    /* one of two splits - prefer the one matching in start point */
    BezierSegment *bs = &g_array_index (sp->path, BezierSegment, 0);
    if (   !sp->used
	&& (sp->outside == outside)
	&& distance_point_point (&bs->p0, pt) < 1.4142 * EPSILON) {
      *next = sp;
      sp->used = TRUE;
      return TRUE;
    }
  }
  /* but also deliver segments ending in pt */
  for (i = 0; i < splits->len; ++i) {
    Split *sp = &g_array_index (splits, Split, i);
    BezierSegment *bs = &g_array_index (sp->path, BezierSegment, sp->path->len - 1);
    if (   !sp->used
	&& (sp->outside == outside)
        && distance_point_point (&bs->p3, pt) < 1.4142 * EPSILON) {
      *next = sp;
      sp->used = TRUE;
      return TRUE;
    }
  }
  return FALSE;
}

static Point
_append_segments (GArray  *path,
		  GArray  *segs)
{
  BezPoint bp;
  int      i;
  gboolean flip;
  BezPoint *ebp = &g_array_index (path, BezPoint, path->len - 1);
  const BezierSegment *sseg = &g_array_index (segs, BezierSegment, 0);
  const BezierSegment *eseg = &g_array_index (segs, BezierSegment, segs->len - 1);

  /* always try to join with what we have */
  if (distance_point_point (&sseg->p0,
      ebp->type == BEZ_CURVE_TO ? &ebp->p3 : &ebp->p1) < EPSILON) {
    /* matching in given direction */
    flip = FALSE;
  } else if (distance_point_point (&eseg->p3,
	     ebp->type == BEZ_CURVE_TO ? &ebp->p3 : &ebp->p1) < EPSILON) {
    /* change direction of segments */
    flip = TRUE;
  } else {
    /* neither matches so we can use any direction but should add a move-to */
    bp.type = BEZ_MOVE_TO;
    bp.p1 = sseg->p0;
    g_array_append_val (path, bp);
    flip = FALSE;
  }

  if (flip) {
    for (i = segs->len - 1; i >= 0; --i) { /* counting down - backwards append */
      _curve_from_segment (&bp, &g_array_index (segs, BezierSegment, i), flip);
      if (bp.type != BEZ_MOVE_TO) /* just ignore move-to here */
	g_array_append_val (path, bp);
    }
  } else {
    for (i = 0; i < segs->len; ++i) { /* preserve original direction */
      _curve_from_segment (&bp, &g_array_index (segs, BezierSegment, i), flip);
      if (bp.type != BEZ_MOVE_TO)
	g_array_append_val (path, bp);
    }
  }
  ebp = &g_array_index (path, BezPoint, path->len - 1);
  return ebp->type == BEZ_CURVE_TO ? ebp->p3 : ebp->p1;
}
/*!
 * \brief Just reassamble both paths again for debugging
 */
static GArray *
_make_path0 (GArray *one, /*!< array<BezierSegment> from first path */
	     GArray *one_splits, /*!< array<Split> for first path */
	     GArray *two, /*!< second path */
	     GArray *two_splits /*!< splits */
	     )
{
  GArray *result = g_array_new (FALSE, FALSE, sizeof(BezPoint));
  int sel;
  for (sel = 0; sel < 2; ++sel) {
    GArray *segs = sel == 0 ? one : two;
    GArray *splits = sel == 0 ? one_splits : two_splits;
    int i, isp = 0;
    BezPoint bp;
    bp.type = BEZ_MOVE_TO;
    bp.p1 = g_array_index (segs, BezierSegment, 0).p0;
    g_array_append_val (result, bp);
    for (i = 0; i < segs->len; ++i) {
      BezierSegment *seg = &g_array_index (segs, BezierSegment, i);
      /* every split starts with a move-to */
      if (   splits
	  && isp < splits->len
	  && i == g_array_index (splits, Split, isp).seg
	  && g_array_index (result, BezPoint, result->len - 1).type != BEZ_MOVE_TO) {
	bp.type = BEZ_MOVE_TO;
	bp.p1 = seg->p0;
	g_array_append_val (result, bp);
	++isp;
      }
      _curve_from_segment (&bp, seg, FALSE);
      g_array_append_val (result, bp);

    }
  }
  return result;
}
/*!
 * \brief Another reassambling for debugging
 */
static GArray *
_make_path1 (GArray *one, /*!< array<BezierSegment> from first path */
	     GArray *one_splits, /*!< array<Split> for first path */
	     GArray *two, /*!< second path */
	     GArray *two_splits /*!< splits */
	     )
{
  GArray *result = g_array_new (FALSE, FALSE, sizeof(BezPoint));
  int i, sel;

  for (sel = 0; sel < 2; ++sel) {
    GArray *splits = sel == 0 ? one_splits : two_splits;
    for (i = 0; i < splits->len; ++i) {
      Split *sp = &g_array_index (splits, Split, i);

      if (i == 0) { /* must at least start with move-to */
	BezPoint bp;
	bp.type = BEZ_MOVE_TO;
	bp.p1 = g_array_index (sp->path, BezierSegment, 0).p0;
	g_array_append_val (result, bp);
      }
      _append_segments (result, sp->path);
    }
  }
  return result;
}
/*!
 * \brief Convert back to a single BezPoint path
 */
static GArray *
_make_path (GArray *one, /*!< array<BezierSegment> from first path */
	    GArray *one_splits, /*!< array<Split> for first path */
	    GArray *two, /*!< second path */
	    GArray *two_splits, /*!< splits */
	    PathCombineMode mode)
{
  GArray *result = g_array_new (FALSE, FALSE, sizeof(BezPoint));
  Split *sp = NULL;
  int i, n = 0;
  BezPoint bp;
  Point cur_pt;
  GArray *splits;
  /* only intersection starts with an inside segment */
  gboolean outside = mode == PATH_INTERSECTION ? FALSE : TRUE;

  g_return_val_if_fail (mode != PATH_EXCLUSION, NULL);
  g_return_val_if_fail (one_splits->len != 0, NULL);

  bp.type = BEZ_MOVE_TO;
  /* start with the first point of segment one */
  for (i = 0; i < one_splits->len; ++i) {
    sp = &g_array_index (one_splits, Split, i);
    if (sp->outside == outside)
      break;
  }
  sp->used = TRUE;
  bp.p1 = g_array_index (one, BezierSegment, sp->seg).p0;
  g_array_append_val (result, bp);
  do {
    cur_pt = _append_segments (result, sp->path);
    n++;
    if (mode == PATH_DIFFERENCE)
      outside = (n % 2) == 0 ? TRUE : FALSE;
    splits = (n % 2) == 0 ? one_splits : two_splits;
    /* find next intersection start from the last point of this sub path */
    if (!_find_split (splits, &cur_pt, outside, &sp)) {
      /* if we can not find something connected search for an unused 'one' path
       * XXX: this might be part of the issue with lost move-to within the segment
       *  The other part might be in _append_segments or even _split_segments
       */
      outside = mode == PATH_INTERSECTION ? FALSE : TRUE;
      for (i = 0; i < one_splits->len; ++i) {
	sp = &g_array_index (one_splits, Split, i);
	if (!sp->used && (sp->outside == outside))
	  break;
	else
	  sp = NULL;
      }
      if (sp) { /* found a new start, make a move-to */
	sp->used = TRUE;
	bp.type = BEZ_MOVE_TO;
	bp.p1 = g_array_index (sp->path, BezierSegment, 0).p0;
	g_array_append_val (result, bp);
      }
    }
  } while (sp);

  return result;
}

static GArray *
_path_copy (const GArray *p)
{
  GArray *result = g_array_new (FALSE, FALSE, sizeof(BezPoint));

  g_array_append_vals (result,  &g_array_index (p, BezPoint, 0), p->len);

  return result;
}

/*!
 * \brief Combine two path into a single one with the given operation
 *
 * This should (but does not) consider
 *  - holes within the path more explicitely
 *  - self intersections in a path
 *  - winding rule?
 */
GArray *
path_combine (const GArray   *p1,
	      const GArray   *p2,
	      PathCombineMode mode)
{
  GArray *result = NULL;
  GArray *crossing = NULL;
  GArray *one, *two;
  static int debug = 0;

  g_return_val_if_fail (p1->len > 1 && p2->len > 1, NULL);

  /* convert both paths to segment representation - TODO: self intersections */
  one = _path_to_segments (p1);
  two = _path_to_segments (p2);
  crossing = _find_intersections (one, two);
  if (crossing) {
    /* Now crossing includes points in arbitrary order. Every point has four lines
     * going in or out - two from p1, two from p2. Split one and two into segments
     * at the crossing points.
     */
    GArray *one_splits = _extract_splits (crossing, TRUE);
    GArray *two_splits = _extract_splits (crossing, FALSE);
    _split_segments (one, one_splits, p2);
    _split_segments (two, two_splits, p1);

    /* convert segments back to a single path */
    if (one_splits->len < 2) { /* XXX: just joining again */
      result = _make_path0 (one, one_splits, two, two_splits);
    } else if (debug) {
      result = _make_path1 (one, one_splits, two, two_splits);
    } else {
      if (mode == PATH_EXCLUSION) { /* most simple impl. */
	GArray *res2;
	result = _make_path (one, one_splits, two, two_splits, PATH_DIFFERENCE);
	res2 = _make_path (two, two_splits, one, one_splits, PATH_DIFFERENCE);
	g_array_append_vals (result, &g_array_index (res2, BezPoint, 0), res2->len);
	g_array_free (res2, TRUE);
      } else {
	result = _make_path (one, one_splits, two, two_splits, mode);
      }
    }
    _free_splits (one_splits);
    _free_splits (two_splits);
    g_array_free (crossing, TRUE);
  } else {
    gboolean two_in_one = distance_bez_shape_point (&g_array_index (p1, BezPoint, 0), p1->len,
				    0 /* line width */, &g_array_index (p2, BezPoint, 0).p1) == 0;
    gboolean one_in_two = distance_bez_shape_point (&g_array_index (p2, BezPoint, 0), p2->len,
				    0 /* line width */, &g_array_index (p1, BezPoint, 0).p1) == 0;

    switch (mode) {
      case PATH_UNION: /* Union and Exclusion just join the pathes */
        if (two_in_one)
          result = _path_copy (p1);
        else if (one_in_two) /* the bigger one */
          result = _path_copy (p2);
        else
          result = _make_path0 (one, NULL, two, NULL);
        break;
      case PATH_DIFFERENCE: /* Difference does it too, if p2 is inside p1 */
        if (two_in_one)
          result = _make_path0 (one, NULL, two, NULL);
        else if (one_in_two)
          result = NULL;
        else
          result = _path_copy (p1);
        break;
      case PATH_INTERSECTION:
        if (two_in_one)
          result = _path_copy (p2);
        else if (one_in_two)
          result = _path_copy (p1);
        else
          result = NULL; /* Intersection is just emtpy w/o crossing */
        break;
      case PATH_EXCLUSION:
        if (two_in_one)/* with two_in_one this is like difference */
          result = _make_path0 (one, NULL, two, NULL);
        else if (one_in_two)
          result = _make_path0 (two, NULL, one, NULL);
        else /* join */
          result = _make_path0 (one, NULL, two, NULL);
        break;
      default:
        g_return_val_if_reached (NULL);
    }
  }
  g_array_free (one, TRUE);
  g_array_free (two, TRUE);
  if (!result || result->len < 2) {
    if (result)
      g_array_free (result, TRUE);
    return NULL;
  }
  return result;
}

