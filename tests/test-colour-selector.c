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

#include "dia-test-utils.h"

#include "dia-colour-selector.h"


static void
test_colour_selector_new_destroy (void)
{
  GtkWidget *selector = dia_colour_selector_new ();

  g_object_ref_sink (selector);

  g_assert_true (DIA_IS_COLOUR_SELECTOR (selector));

  g_assert_finalize_object (selector);
}


static void
test_colour_selector_get_set_use_alpha (void)
{
  DiaColourSelector *selector = g_object_new (DIA_TYPE_COLOUR_SELECTOR, NULL);
  gboolean use_alpha;

  g_object_ref_sink (selector);

  dia_test_property_notify (selector, "use-alpha", TRUE, FALSE);

  g_object_get (selector, "use-alpha", &use_alpha, NULL);
  g_assert_false (use_alpha);

  g_assert_finalize_object (selector);
}


static void
test_colour_selector_get_set_current (void)
{
  DiaColourSelector *selector = g_object_new (DIA_TYPE_COLOUR_SELECTOR, NULL);
  DiaColour colour_a;
  DiaColour colour_b;
  DiaColour *colour_out_a = NULL;
  DiaColour colour_out_b;

  g_object_ref_sink (selector);

  dia_colour_parse (&colour_a, "#3c6332");
  dia_colour_parse (&colour_b, "#613583");

  dia_test_property_notify (selector, "current", &colour_a, &colour_b);

  g_object_get (selector, "current", &colour_out_a, NULL);
  dia_colour_selector_get_colour (selector, &colour_out_b);

  g_assert_true (color_equals (colour_out_a, &colour_out_b));
  g_assert_true (color_equals (colour_out_a, &colour_b));

  g_assert_finalize_object (selector);

  g_clear_pointer (&colour_out_a, dia_colour_free);
}


static void
test_colour_selector_set_colour (void)
{
  DiaColourSelector *selector = g_object_new (DIA_TYPE_COLOUR_SELECTOR, NULL);
  DiaColour colour_a;
  DiaColour colour_b;
  DiaColour *colour_out_a = NULL;
  DiaColour colour_out;

  g_object_ref_sink (selector);

  dia_colour_parse (&colour_a, "#3c6332");
  dia_colour_parse (&colour_b, "#613583");

  /* No colour yet, so black is reported */
  dia_colour_selector_get_colour (selector, &colour_out);
  g_assert_cmpfloat (colour_out.red, ==, 0.0);
  g_assert_cmpfloat (colour_out.green, ==, 0.0);
  g_assert_cmpfloat (colour_out.blue, ==, 0.0);
  g_assert_cmpfloat (colour_out.alpha, ==, 1.0);

  /* Become colour A, a colour not in the default set */
  dia_expect_property_notify (selector, "current");
  dia_colour_selector_set_colour (selector, &colour_a);
  dia_assert_expected_notifies (selector);

  dia_colour_selector_get_colour (selector, &colour_out);
  g_assert_true (color_equals (&colour_out, &colour_a));

  /* This time already A, so ignored */
  dia_expect_no_notify (selector);
  dia_colour_selector_set_colour (selector, &colour_a);
  dia_assert_expected_notifies (selector);

  /* Switch to B */
  dia_expect_property_notify (selector, "current");
  dia_colour_selector_set_colour (selector, &colour_b);
  dia_assert_expected_notifies (selector);

  dia_colour_selector_get_colour (selector, &colour_out);
  g_assert_true (color_equals (&colour_out, &colour_b));

  /* Return to A, hopefully finding the old entry */
  dia_expect_property_notify (selector, "current");
  dia_colour_selector_set_colour (selector, &colour_a);
  dia_assert_expected_notifies (selector);

  dia_colour_selector_get_colour (selector, &colour_out);
  g_assert_true (color_equals (&colour_out, &colour_a));

  g_assert_finalize_object (selector);

  g_clear_pointer (&colour_out_a, dia_colour_free);
}


int
main (int argc, char *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/dia/colour-selector/new-destroy",
                   test_colour_selector_new_destroy);
  g_test_add_func ("/dia/colour-selector/get-set/use-alpha",
                   test_colour_selector_get_set_use_alpha);
  g_test_add_func ("/dia/colour-selector/get-set/current",
                   test_colour_selector_get_set_current);
  g_test_add_func ("/dia/colour-selector/set_colour",
                   test_colour_selector_set_colour);

  return g_test_run ();
}
