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
#include <stdio.h>
#include <string.h>
#include <math.h>

#undef G_DISABLE_ASSERT
#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "Dia"

#include <glib.h>
#include <glib-object.h>

#if GLIB_CHECK_VERSION(2,16,0)
#include <glib/gtestutils.h>
#endif
#include "libdia.h"

/*
 * To test DiaObject bounding boxes a nice approach would be to implment a BoundingboxRenderer
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

#define T (0.1)
#define BEZ(x) sizeof(_bz ##x)/sizeof(BezPoint), _bz ##x

static struct _TestBeziers {
  int       num;
  BezPoint *pts;
  Rectangle box;
} _test_beziers[] = {                   /* top, left, bottom, right */
  { BEZ(1), { 0.5-T, 0.0-T, 2.0+T, 2.0+T } },
  { BEZ(2), { 0.5-T,-2.0-T, 2.0+T, 0.0+T } },
  { BEZ(3), {-1.5-T,-2.0-T, 0.0+T, 0.0+T } },
  { BEZ(4), {-1.5-T, 0.0-T, 0.0+T, 2.0+T } },
  /* it only 'jumps' out of the box with a line width>0 */
  { BEZ(5), { 0.5-T, 0.0-T, 2.0+T, 2.0+T } },
  { BEZ(6), { 0.0-T, 0.2-T, 2.0+T, 1.8+T } },
  { BEZ(7), { 0.25-T,0.25-T, 1.75+T, 1.75+T} },
};
#undef BEZ

static void
_check_one_bezier (const struct _TestBeziers *test)
{
  Rectangle rect;
  PolyBBExtras extra = {0, T*.7, T*.7, T*.7, 0 };
  
  polybezier_bbox (test->pts, test->num, &extra, FALSE, &rect);
  g_assert (rectangle_in_rectangle (&test->box, &rect));
}
static void
_add_bezier_tests (void)
{
  int i, num = sizeof(_test_beziers)/sizeof(struct _TestBeziers);
  
  for (i = 0; i < num; ++i)
    {
      gchar *testpath = g_strdup_printf ("/Dia/BoundingBox/Bezier%d", i+1);
#if GLIB_CHECK_VERSION(2,16,0)
      g_test_add_data_func (testpath, &_test_beziers[i], _check_one_bezier);
#endif
      g_free (testpath);
    }
}

int
main (int argc, char** argv)
{
  GList *plugins = NULL;
  int ret;
  
#if GLIB_CHECK_VERSION(2,16,0)
  g_test_init (&argc, &argv, NULL);
  /* not really needed - or are there message_warnings in the bbox code? */
  libdia_init (DIA_MESSAGE_STDERR);

  _add_bezier_tests ();

  ret = g_test_run ();
#else
  g_print ("GLib version does not support g_test_*()");
#endif
  return ret;
}
