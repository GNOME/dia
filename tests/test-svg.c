/* test-svg.c -- Unit test for Dia svg parsing helpers
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

#include <config.h>

#include <glib/gi18n-lib.h>

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <glib.h>
#include <glib-object.h>

#include "dialib.h"
#include "geometry.h"
#include "dia_svg.h"

typedef struct _PathData
{
  int         num_points; /* >1 to be valid */
  int         num_splits;
  gboolean    closed;
  const char* data;
} PathData;

PathData _test_path[] = {
  {  2, 0, FALSE, "M0,0L1,1" },
  {  3, 0, TRUE,  "M0,0L1,1z" },
  {  3, 1, TRUE,  "M0,0L1,1z z" },  /* ignore extra closing */
  {  3, 0, FALSE, "M0,0 0,1 1,1" }, /* implicit line-to */
  {  4, 0, TRUE,  "M0,0 0,1 1,1z" },
  /* 5: variations of elements (from path-variations.svg) */
  {  5, 0, TRUE,  "M0,0 20,0 20,20 0,20 0,0z"},
  {  5, 0, TRUE,  "M0,0h20v20h-20z"},
  {  5, 0, FALSE, "M20,20L40,20 40,40 20,40 20,20"},
  {  5, 0, TRUE,  "m180,20 20,0 0,20 -20,0 z"}, /* relative move does not matter */
  { 10, 1, TRUE,  "M20,20 80,20 80,80 20,80z M40,40 40,60 60,60 60,40 40,40z"}, /* with hole */
  { 10, 1, TRUE,  "M20,20 80,20 80,80 20,80z M40,40 60,40 60,60 40,60 40,40z"}, /* changed direction */
  /* 11: variations from test-boundingbox */
  {  2, 0, FALSE, "M0.0,2.0C0.0,0.0,2.0,0.0,2.0,2.0"},
  {  2, 0, FALSE, "M-2.0,2.0 C-2.0,0.0, 0.0,0.0, 0.0,2.0"},
  {  3, 0, FALSE, "M1.0,2.0 C 0.0,2.0 0.0,0.0 1.0,0.0\n2.0,0.0 2.0,2.0 1.0,2.0\n"},
  {  5, 0, FALSE, "M C0,0, 0,2, 1,1 C2,0, 2,2, 1,1 C0,0, 2,0, 1,1 C0,2, 2,2, 1,1"},
  /* 15: paths-data-01-t.svg */
  {  6, 1, FALSE, "M 210 130 C 145 130 110 80 110 80 S 75 25 10 25 m 0 105 c 65 0 100 -50 100 -50 s 35 -55 100 -55" },
  /* 16: paths-data-02-t.svg */
  {  5, 1, TRUE,  "M15 20 Q30 120 130 30 M 180 80 q -75 -100 -163 -60z" },
  {  4, 0, TRUE,  "M208 168Q258 268 308 168T258 118Q128 88 208 168z" },
  {  7, 0, TRUE,  "M240 296q25-100 47 0t47 0t47 0t47 0t47 0z" },
  /* 19: paths-data-03-f.svg */
  {  6, 0, TRUE,  "M25 70 A40 40 0 1 0 25 69 Z" }, /* number of points depends on the coordinates */
  /* 20: paths-data-06-t.svg */
  {  9, 0, TRUE,  "M240 56H270V86H300V116H330V146H240V56Z"},
  /* 21: paths-data-06-t.svg */
  {  9, 0, TRUE,  "m240 190h30v30h30v30h30v30h-90v-90z" },
  /* xxx: other */
  {  0,-1, TRUE,  "M0,0Z"}, /* Dia does not want this */
/*
  {  2, 0, FALSE, ""},
  {  4, 0, TRUE,  ""},
 */
};

static void
_check_one_path (gconstpointer _p)
{
  const PathData* pd = _p;
  const gchar* p;
  gchar* unparsed = NULL;
  gboolean closed = FALSE;
  Point current_point = {0.0, 0.0};
  GArray* points = g_array_new(FALSE, FALSE, sizeof(BezPoint));
  int calls = 0;
  g_array_set_size(points, 0);

  p = pd->data;
  while (dia_svg_parse_path (points, p, &unparsed, &closed, &current_point))
    {
      calls++;
      if (!unparsed)
	break;
      p = unparsed;
    }

  g_assert (pd->num_points == points->len);
  g_assert (pd->num_splits == calls - 1);
  g_assert (pd->closed == closed);

  g_array_free (points, TRUE);
}


static void
_add_path_tests (void)
{
  int i, num = sizeof (_test_path) / sizeof (PathData);

  for (i = 0; i < num; ++i) {
    char *testpath = g_strdup_printf ("/Dia/svg/test-path%d", i);

    g_test_add_data_func (testpath, &_test_path[i], _check_one_path);

    g_clear_pointer (&testpath, g_free);
  }
}


int
main (int argc, char** argv)
{
  int ret;

  g_test_init (&argc, &argv, NULL);

  libdia_init (DIA_MESSAGE_STDERR);

  _add_path_tests ();

  ret = g_test_run ();

  return ret;
}
