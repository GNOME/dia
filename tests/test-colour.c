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

#include "dia-colour.h"


static void
test_colour_new_destroy (void)
{
  DiaColour *colour = dia_colour_new_rgb (1.0, 1.0, 1.0);

  g_assert_nonnull (colour);

  dia_colour_free (colour);
}


static void
test_colour_boxed (void)
{
  DiaColour source = DIA_COLOUR_WHITE;
  DiaColour *copy;

  g_assert_true (G_TYPE_IS_BOXED (DIA_TYPE_COLOUR));
  g_assert_cmpstr (g_type_name (DIA_TYPE_COLOUR), ==, "DiaColour");

  copy = g_boxed_copy (DIA_TYPE_COLOUR, &source);
  g_boxed_free (DIA_TYPE_COLOUR, copy);
}


static void
test_colour_new_rgb (void)
{
  DiaColour *colour_a = dia_colour_new_rgb (1.0, 1.0, 1.0);
  DiaColour *colour_b = dia_colour_new_rgb (0.0, 0.0, 0.0);

  g_assert_cmpfloat (colour_a->red, ==, 1.0);
  g_assert_cmpfloat (colour_a->blue, ==, 1.0);
  g_assert_cmpfloat (colour_a->green, ==, 1.0);
  g_assert_cmpfloat (colour_a->alpha, ==, 1.0);

  g_assert_cmpfloat (colour_b->red, ==, 0.0);
  g_assert_cmpfloat (colour_b->blue, ==, 0.0);
  g_assert_cmpfloat (colour_b->green, ==, 0.0);
  g_assert_cmpfloat (colour_b->alpha, ==, 1.0);

  g_assert_true (dia_colour_is_white (colour_a));
  g_assert_true (dia_colour_is_black (colour_b));

  g_clear_pointer (&colour_a, dia_colour_free);
  g_clear_pointer (&colour_b, dia_colour_free);
}


static void
test_colour_new_rgba (void)
{
  DiaColour *colour_a = dia_colour_new_rgba (1.0, 1.0, 1.0, 1.0);
  DiaColour *colour_b = dia_colour_new_rgba (1.0, 1.0, 1.0, 0.5);
  DiaColour *colour_c = dia_colour_new_rgba (0.0, 0.0, 0.0, 1.0);
  DiaColour *colour_d = dia_colour_new_rgba (0.0, 0.0, 0.0, 0.5);

  g_assert_cmpfloat (colour_a->red, ==, 1.0);
  g_assert_cmpfloat (colour_a->blue, ==, 1.0);
  g_assert_cmpfloat (colour_a->green, ==, 1.0);
  g_assert_cmpfloat (colour_a->alpha, ==, 1.0);

  g_assert_cmpfloat (colour_b->red, ==, 1.0);
  g_assert_cmpfloat (colour_b->blue, ==, 1.0);
  g_assert_cmpfloat (colour_b->green, ==, 1.0);
  g_assert_cmpfloat (colour_b->alpha, ==, 0.5);

  g_assert_cmpfloat (colour_c->red, ==, 0.0);
  g_assert_cmpfloat (colour_c->blue, ==, 0.0);
  g_assert_cmpfloat (colour_c->green, ==, 0.0);
  g_assert_cmpfloat (colour_c->alpha, ==, 1.0);

  g_assert_cmpfloat (colour_d->red, ==, 0.0);
  g_assert_cmpfloat (colour_d->blue, ==, 0.0);
  g_assert_cmpfloat (colour_d->green, ==, 0.0);
  g_assert_cmpfloat (colour_d->alpha, ==, 0.5);

  g_assert_true (dia_colour_is_white (colour_a));
  g_assert_false (dia_colour_is_white (colour_b));
  g_assert_true (dia_colour_is_black (colour_c));
  g_assert_false (dia_colour_is_black (colour_d));

  g_clear_pointer (&colour_a, dia_colour_free);
  g_clear_pointer (&colour_b, dia_colour_free);
  g_clear_pointer (&colour_c, dia_colour_free);
  g_clear_pointer (&colour_d, dia_colour_free);
}


static void
test_colour_copy (void)
{
  DiaColour source = DIA_COLOUR_WHITE;
  DiaColour *copy_a = dia_colour_copy (&source);
  DiaColour *copy_b = dia_colour_copy (&source);

  g_assert_nonnull (copy_a);
  g_assert_nonnull (copy_b);

  g_assert_cmpfloat (source.red, ==, copy_a->red);
  g_assert_cmpfloat (source.blue, ==, copy_a->blue);
  g_assert_cmpfloat (source.green, ==, copy_a->green);
  g_assert_cmpfloat (source.alpha, ==, copy_a->alpha);

  g_assert_cmpfloat (copy_a->red, ==, copy_b->red);
  g_assert_cmpfloat (copy_a->blue, ==, copy_b->blue);
  g_assert_cmpfloat (copy_a->green, ==, copy_b->green);
  g_assert_cmpfloat (copy_a->alpha, ==, copy_b->alpha);

  g_assert_true (copy_a != copy_b);

  g_clear_pointer (&copy_a, dia_colour_free);
  g_clear_pointer (&copy_b, dia_colour_free);
}


struct parse_test {
  const char *str;
  gboolean fail;
  DiaColour expected;
} parse_cases[] = {
  { "#000000", FALSE, DIA_COLOUR_BLACK },
  { "#FFFFFF", FALSE, DIA_COLOUR_WHITE },
  { "#FFFFXX", TRUE, DIA_COLOUR_WHITE },
  { "#FFFFF", TRUE, DIA_COLOUR_WHITE },
  { "#FFFFFFFF", FALSE, DIA_COLOUR_WHITE },
  { "#FFFFFFXX", TRUE, DIA_COLOUR_WHITE },
};


static void
test_colour_parse (gconstpointer user_data)
{
  const struct parse_test *test = user_data;

  if (g_test_subprocess ()) {
    DiaColour res;

    dia_colour_parse (&res, test->str);

    g_assert_cmpfloat (res.red, ==, test->expected.red);
    g_assert_cmpfloat (res.blue, ==, test->expected.blue);
    g_assert_cmpfloat (res.green, ==, test->expected.green);
    g_assert_cmpfloat (res.alpha, ==, test->expected.alpha);

    return;
  }

  g_test_trap_subprocess (NULL, 0, 0);

  if (test->fail) {
    g_test_trap_assert_failed ();
    g_test_trap_assert_stderr ("*should not be reached*");
  } else {
    g_test_trap_assert_passed ();
  }
}


struct string_test {
  DiaColour colour;
  const char *expected;
} string_cases[] = {
  { DIA_COLOUR_BLACK, "#000000FF" },
  { DIA_COLOUR_WHITE, "#FFFFFFFF" },
  { { 0.38, 0.21, 0.52, 0.93 }, "#603584ED" },
  { { 1.1, -0.1, 0.0, -0.0 }, "#FF000000" },
};


static void
test_colour_to_string (gconstpointer user_data)
{
  struct string_test *test = (struct string_test *) user_data;
  char *res = dia_colour_to_string (&test->colour);

  g_assert_cmpstr (test->expected, ==, res);

  g_clear_pointer (&res, g_free);
}


static void
test_colour_gdk (void)
{
  DiaColour source = DIA_COLOUR_BLACK;
  DiaColour res;
  GdkRGBA rgba;

  dia_colour_as_gdk (&source, &rgba);
  dia_colour_from_gdk (&res, &rgba);

  g_assert_true (dia_colour_equals (&source, &res));
}


static void
test_colour_equals (void)
{
  g_assert_true (dia_colour_equals (&DIA_COLOUR_BLACK, &DIA_COLOUR_BLACK));
  g_assert_true (dia_colour_equals (&DIA_COLOUR_WHITE, &DIA_COLOUR_WHITE));
  g_assert_false (dia_colour_equals (&DIA_COLOUR_BLACK, &DIA_COLOUR_WHITE));
  g_assert_false (dia_colour_equals (&DIA_COLOUR_WHITE, &DIA_COLOUR_BLACK));
}


static void
test_colour_is_white (void)
{
  g_assert_true (dia_colour_is_white (&DIA_COLOUR_WHITE));
  g_assert_false (dia_colour_is_white (&DIA_COLOUR_BLACK));
}


static void
test_colour_is_black (void)
{
  g_assert_true (dia_colour_is_black (&DIA_COLOUR_BLACK));
  g_assert_false (dia_colour_is_black (&DIA_COLOUR_WHITE));
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/dia/colour/new-destroy",
                   test_colour_new_destroy);
  g_test_add_func ("/dia/colour/boxed",
                   test_colour_boxed);
  g_test_add_func ("/dia/colour/new-rgb",
                   test_colour_new_rgb);
  g_test_add_func ("/dia/colour/new-rgba",
                   test_colour_new_rgba);
  g_test_add_func ("/dia/colour/copy",
                   test_colour_copy);

  for (size_t i = 0; i < G_N_ELEMENTS (parse_cases); i++) {
    g_autofree char *path =
      g_strdup_printf ("/dia/colour/parse/case_%" G_GSIZE_FORMAT, i);

    g_test_add_data_func (path, &parse_cases[i], test_colour_parse);
  }

  for (size_t i = 0; i < G_N_ELEMENTS (string_cases); i++) {
    g_autofree char *path =
      g_strdup_printf ("/dia/colour/to_string/case_%" G_GSIZE_FORMAT, i);

    g_test_add_data_func (path, &string_cases[i], test_colour_to_string);
  }

  g_test_add_func ("/dia/colour/gdk",
                   test_colour_gdk);
  g_test_add_func ("/dia/colour/equals",
                   test_colour_equals);
  g_test_add_func ("/dia/colour/is_white",
                   test_colour_is_white);
  g_test_add_func ("/dia/colour/is_black",
                   test_colour_is_black);

  return g_test_run ();
}
