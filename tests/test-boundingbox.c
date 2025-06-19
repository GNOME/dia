/* test-boundingbo.c -- Unit test for Dia bounding box calculattions
 * Copyright (C) 2008 Hans Breuer
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

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <glib.h>
#include <glib-object.h>

#include "dialib.h"

/*
 * To test DiaObject bounding boxes a nice approach would be to implement a BoundingboxRenderer
 * which uses it's state while rendering objects via DiaObject::draw () to compare the bounding got
 * dia_object_get_bounding_box (). For this to be integrated with test-objects.c it is kind of required
 * that the bounding box primitives are working ...
 */


#include "boundingbox.h"
#include "geometry.h"

static BezPoint _bz1[2] = { /* SE */
                 /* x, y */
  {BEZ_MOVE_TO,  {0.0, 2.0} },
  {BEZ_CURVE_TO, {0.0, 0.0}, {2.0, 0.0}, {2.0, 2.0} },
};
static BezPoint _bz2[2] = { /* SW */
                 /* x, y */
  {BEZ_MOVE_TO,  {-2.0, 2.0} },
  {BEZ_CURVE_TO, {-2.0, 0.0}, {0.0, 0.0}, {0.0, 2.0} },
};
static BezPoint _bz3[2] = { /* NW */
                 /* x, y */
  {BEZ_MOVE_TO,  {-2.0, 0.0} },
  {BEZ_CURVE_TO, {-2.0,-2.0}, {0.0,-2.0}, {0.0, 0.0} },
};
static BezPoint _bz4[2] = { /* NE */
                 /* x, y */
  {BEZ_MOVE_TO,  {0.0, 0.0} },
  {BEZ_CURVE_TO, {0.0,-2.0}, {2.0,-2.0}, {2.0, 0.0} },
};
static BezPoint _bz5[2] = { /* swapped control points */
                 /* x, y */
  {BEZ_MOVE_TO,  {0.0, 2.0} },
  {BEZ_CURVE_TO, {2.0, 0.0}, {0.0, 0.0}, {2.0, 2.0} },
};
static BezPoint _bz6[3] = {
  {BEZ_MOVE_TO,  {1.0, 2.0} },
  {BEZ_CURVE_TO, {0.0, 2.0}, {0.0, 0.0}, {1.0, 0.0} },
  {BEZ_CURVE_TO, {2.0, 0.0}, {2.0, 2.0}, {1.0, 2.0} },
};
static BezPoint _bz7[5] = {
  { BEZ_MOVE_TO, {1,1} },
  { BEZ_CURVE_TO, {0,0}, {0,2}, {1,1} },
  { BEZ_CURVE_TO, {2,0}, {2,2}, {1,1} },
  { BEZ_CURVE_TO, {0,0}, {2,0}, {1,1} },
  { BEZ_CURVE_TO, {0,2}, {2,2}, {1,1} },
};
static BezPoint _bz8[] = { /* ying: wrong bbox {-0.05,-0.05,3.05,5.0} */
  { BEZ_MOVE_TO, {2,0} },
  { BEZ_CURVE_TO, {1.2855,0}, {0.6252,0.3812}, {0.2679,1} },
  { BEZ_CURVE_TO, {-0.0893,1.6188}, {-0.0893,2.3812}, {0.2679,3} },
  { BEZ_CURVE_TO, {0.6252,3.6188}, {1.2855,4}, {2,4} },
  { BEZ_CURVE_TO, {1.6427,4}, {1.3126,3.8094}, {1.134,3.5} },
  { BEZ_CURVE_TO, {0.9553,3.1906}, {0.9553,2.8094}, {1.134,2.5} },
  { BEZ_CURVE_TO, {1.3126,2.1906}, {1.6427,2}, {2,2} },
  { BEZ_CURVE_TO, {2.3573,2}, {2.6874,1.8094}, {2.866,1.5} },
  { BEZ_CURVE_TO, {3.0447,1.1906}, {3.0447,0.8094}, {2.866,0.5} },
  { BEZ_CURVE_TO, {2.6874,0.1906}, {2.3573,0}, {2,0} },
  { BEZ_MOVE_TO, {2,2.6} },
  { BEZ_CURVE_TO, {2.22,2.6}, {2.4,2.78}, {2.4,3} },
  { BEZ_CURVE_TO, {2.4,3.22}, {2.22,3.4}, {2,3.4} },
  { BEZ_CURVE_TO, {1.78,3.4}, {1.6,3.22}, {1.6,3} },
  { BEZ_CURVE_TO, {1.6,2.78}, {1.78,2.6}, {2,2.6} },
  { BEZ_MOVE_TO, {2,0.6} },
  { BEZ_CURVE_TO, {2.22,0.6}, {2.4,0.78}, {2.4,1} },
  { BEZ_CURVE_TO, {2.4,1.22}, {2.22,1.4}, {2,1.4} },
  { BEZ_CURVE_TO, {1.78,1.4}, {1.6,1.22}, {1.6,1} },
  { BEZ_CURVE_TO, {1.6,0.78}, {1.78,0.6}, {2,0.6} }
};

static BezPoint _bz9[] = { /* heart from assorted shapes */
  { BEZ_MOVE_TO, {0.219064,0.919322} },
  { BEZ_CURVE_TO, {-0.563306,-0.0586407}, {1.00143,-0.449826}, {1.00143,0.723729} },
  { BEZ_CURVE_TO, {1.00143,0.723729}, {1.00143,0.723729}, {1.00143,0.723729} },
  { BEZ_CURVE_TO, {1.00143,-0.449826}, {2.56617,-0.0586407}, {1.7838,0.919322} },
  { BEZ_CURVE_TO, {1.7838,0.919322}, {1.00143,1.89728}, {1.00143,1.89728} },
  { BEZ_CURVE_TO, {1.00143,1.89728}, {0.219064,0.919322}, {0.219064,0.919322} },
};

#define T (0.1)
#define BEZ(x) sizeof(_bz ##x)/sizeof(BezPoint), _bz ##x

static struct _TestBeziers {
  int          num;
  BezPoint    *pts;
  DiaRectangle box;
} _test_beziers[] = {                   /* left, top, right, bottom */
  { BEZ(1), { 0.0-T, 0.5-T, 2.0+T, 2.0+T } },
  { BEZ(2), {-2.0-T, 0.5-T, 0.0+T, 2.0+T } },
  { BEZ(3), {-2.0-T,-1.5-T, 0.0+T, 0.0+T } },
  { BEZ(4), { 0.0-T,-1.5-T, 2.0+T, 0.0+T } },
  /* it only 'jumps' out of the box with a line width>0 */
  { BEZ(5), { 0.0-T, 0.5-T, 2.0+T, 2.0+T } },
  { BEZ(6), { 0.2-T, 0.0-T, 1.8+T, 2.0+T } },
  { BEZ(7), { 0.25-T,0.25-T, 1.75+T, 1.75+T} },
  { BEZ(8), { 0-T, 0-T, 3+T, 4+T} },
  { BEZ(9), { 0-T, 0-T, 2.052+T, 1.897+T } },
};
#undef BEZ

static void
_check_one_bezier (gconstpointer p)
{
  const struct _TestBeziers *test = p;
  DiaRectangle rect;
  PolyBBExtras extra = {0, T*.7, T*.7, T*.7, 0 };

  polybezier_bbox (test->pts, test->num, &extra, FALSE, &rect);
  g_assert (rectangle_in_rectangle (&test->box, &rect));
}


static void
_add_bezier_tests (void)
{
  int i, num = sizeof (_test_beziers) / sizeof (struct _TestBeziers);

  for (i = 0; i < num; ++i) {
    char *testpath = g_strdup_printf ("/Dia/BoundingBox/Bezier%d", i+1);

    g_test_add_data_func (testpath, &_test_beziers[i], _check_one_bezier);

    g_clear_pointer (&testpath, g_free);
  }
}

#ifdef G_OS_WIN32
#include <windows.h>
#endif

int
main (int argc, char** argv)
{
  int ret;

#ifdef G_OS_WIN32
  /* No dialog if it fails, please. */
  SetErrorMode(SetErrorMode(0) | SEM_NOGPFAULTERRORBOX);
#endif

  g_test_init (&argc, &argv, NULL);
  /* not really needed - or are there message_warnings in the bbox code? */
  libdia_init (DIA_MESSAGE_STDERR);

  _add_bezier_tests ();

  ret = g_test_run ();

  return ret;
}

