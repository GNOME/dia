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
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright 2026 Zander Brown <zbrown@gnome.org>
 */

#include "config.h"

#include "dia-graphene.h"


static void
test_matrix_from_graphene (void)
{
  DiaMatrix d = { 0 };
  graphene_matrix_t g;

  graphene_matrix_init_identity (&g);

  g_assert_true (graphene_matrix_is_identity (&g));

  dia_matrix_from_graphene (&d, &g);

  g_assert_true (dia_matrix_is_identity (&d));
}


static void
test_matrix_to_graphene(void)
{
  DiaMatrix d = {
    .xx = 1.0, .yx = 0.0, .xy = 0.0, .yy = 1.0, .x0 = 0.0, .y0 = 0.0,
  };
  graphene_matrix_t g;

  g_assert_true (dia_matrix_is_identity (&d));

  dia_matrix_to_graphene (&d, &g);

  g_assert_true (graphene_matrix_is_identity (&g));
}


static void
test_rectangle_from_graphene (void)
{
  DiaRectangle d;
  graphene_rect_t g;

  graphene_rect_init (&g, 0.0, 0.0, 10.0, 10.0);

  dia_rectangle_from_graphene (&d, &g);

  g_assert_cmpfloat (d.top, ==, 0.0);
  g_assert_cmpfloat (d.left, ==, 0.0);
  g_assert_cmpfloat (d.bottom, ==, 10.0);
  g_assert_cmpfloat (d.right, ==, 10.0);
}


static void
test_rectangle_to_graphene (void)
{
  DiaRectangle d = { .top = 0.0, .left = 0.0, .bottom = 10.0, .right = 10.0 };
  graphene_rect_t g;
  graphene_point_t p;

  dia_rectangle_to_graphene (&d, &g);

  graphene_rect_get_top_left (&g, &p);
  g_assert_true (graphene_point_equal (&p, &GRAPHENE_POINT_INIT (0.0, 0.0)));

  graphene_rect_get_bottom_left (&g, &p);
  g_assert_true (graphene_point_equal (&p, &GRAPHENE_POINT_INIT (0.0, 10.0)));

  graphene_rect_get_top_right (&g, &p);
  g_assert_true (graphene_point_equal (&p, &GRAPHENE_POINT_INIT (10.0, 0.0)));

  graphene_rect_get_bottom_right (&g, &p);
  g_assert_true (graphene_point_equal (&p, &GRAPHENE_POINT_INIT (10.0, 10.0)));
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/dia/graphene/matrix/from", test_matrix_from_graphene);
  g_test_add_func ("/dia/graphene/matrix/to", test_matrix_to_graphene);
  g_test_add_func ("/dia/graphene/rectangle/from", test_rectangle_from_graphene);
  g_test_add_func ("/dia/graphene/rectangle/to", test_rectangle_to_graphene);

  return g_test_run ();
}
