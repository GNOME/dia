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
 * Copyright 2025 Zander Brown <zbrown@gnome.org>
 */

#include "config.h"

#include <gio/gio.h>


static void
diff_files (const char *file1, const char *file2)
{
  GSubprocess *process;
  GBytes *output;
  GError *error = NULL;

  process = g_subprocess_new (G_SUBPROCESS_FLAGS_STDOUT_PIPE,
                              &error,
                              DIFF_BIN, "-q", file1, file2, NULL);

  g_assert_nonnull (process);
  g_assert_no_error (error);

  g_subprocess_communicate (process,
                            NULL,
                            NULL,
                            &output,
                            NULL,
                            &error);

  g_assert_no_error (error);
  g_assert_true (g_subprocess_get_if_exited (process));

  if (g_subprocess_get_exit_status (process) != 0) {
    g_test_fail_printf ("Exported file doesn't match reference. diff output:");
    g_test_fail_printf ("%s", (const char *) g_bytes_get_data (output, NULL));
    goto out;
  }

  g_assert_true (g_subprocess_get_successful (process));
  g_assert_cmpint (g_bytes_get_size (output), ==, 0);

out:
  g_clear_pointer (&output, g_bytes_unref);
  g_clear_object (&process);
}


struct export_test {
  const char *plugin;
  const char *basename;
};


const char *plugins[] = {
  "cgm",
  "dxf",
  "eps",
  "fig",
  "mp",
  "plt",
  "hpgl",
  "png",
  "jpg",
  "shape",
  "svg",
  "tex",
  "wpg",
};


const char *basenames[] = {
  "Arcs",
  "Bezierlines",
  "Ellipses",
  "Polygons",
  "Texts",
  "Beziergons",
  "Boxes",
  "Lines",
  "Polylines",
  "Zigzaglines",
};


static void
test_export (gconstpointer data)
{
  const struct export_test *test = data;
  GSubprocess *process;
  GError *error = NULL;
  char *name_with_dia = g_strconcat (test->basename, ".dia", NULL);
  char *name_with_ext = g_strdup_printf ("%s.%s", test->basename, test->plugin);
  const char *src_path = g_test_get_filename (G_TEST_DIST,
                                              "exports",
                                              name_with_dia,
                                              NULL);
  const char *ref_path = g_test_get_filename (G_TEST_DIST,
                                              "exports",
                                              test->plugin,
                                              name_with_ext,
                                              NULL);
  const char *out_path = g_test_get_filename (G_TEST_BUILT,
                                              name_with_ext,
                                              NULL);

  process = g_subprocess_new (G_SUBPROCESS_FLAGS_NONE,
                              &error,
                              DIA_BIN, "-t", test->plugin, "-e", out_path, src_path, NULL);

  g_assert_nonnull (process);
  g_assert_no_error (error);

  g_subprocess_communicate (process, NULL, NULL, NULL, NULL, &error);

  g_assert_no_error (error);
  g_assert_true (g_subprocess_get_if_exited (process));
  g_assert_true (g_subprocess_get_successful (process));

  diff_files (ref_path, out_path);

  g_clear_pointer (&name_with_ext, g_free);
  g_clear_pointer (&name_with_dia, g_free);
  g_clear_object (&process);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  for (size_t i = 0; i < G_N_ELEMENTS (plugins); i++) {
    for (size_t j = 0; j < G_N_ELEMENTS (basenames); j++) {
      struct export_test *test = g_new0 (struct export_test, 1);
      char *path = g_strdup_printf ("/dia/exports/%s/%s", plugins[i], basenames[j]);

      test->plugin = plugins[i];
      test->basename = basenames[j];

      g_test_add_data_func_full (path, test, test_export, g_free);

      g_clear_pointer (&path, g_free);
    }
  }

  return g_test_run ();
}
