/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Dynamic calculat
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

#include <pango/pango.h>
#include <gdk/gdk.h>
#ifdef GDK_WINDOWING_WIN32
/* avoid namespace clashes caused by inclusion of windows.h */
#define WIN32_LEAN_AND_MEAN
#include <pango/pangowin32.h>
#endif

#include "font.h"

static const char *test_families[] = {
  "sans",
  "serif",
  "monospace",
  "Arial",
  "Times New Roman",
  "Courier New",
  NULL
};
static real pixels_per_cm = 20.0;

typedef enum {
  DUMP_FACTORS = (1<<0),
  DUMP_ABSOLUTE = (1<<1)
} DumpFlags;

static void
dump_font_sizes (PangoContext *context, FILE *f, guint flags)
{
  PangoFont *font;
  PangoFontDescription *pfd;
  int nf;
  real height;

  fprintf (f, "height/cm");
  for (nf = 0; test_families[nf] != NULL; ++nf)
    fprintf (f, "\t%s", test_families[nf]);
  fprintf (f, "\n");

  for (height = 0.1; height <= 10.0; height += 0.1) {
    fprintf (f, "%g", height);
    for (nf = 0; test_families[nf] != NULL; ++nf) {
      pfd = pango_font_description_new ();
      pango_font_description_set_family (pfd, test_families[nf]);
      //pango_font_description_set_size (pfd, height * pixels_per_cm * PANGO_SCALE);
      if (flags & DUMP_ABSOLUTE)
        pango_font_description_set_absolute_size (pfd, height * pixels_per_cm * PANGO_SCALE);
      else
        pango_font_description_set_size (pfd, height * pixels_per_cm * PANGO_SCALE);

      font = pango_context_load_font (context, pfd);
      if (font) {
        PangoFontMetrics *metrics = pango_font_get_metrics (font, NULL);
        /* now make a font-size where the font/line-height matches the given pixel size */
        double total = ((double) pango_font_metrics_get_ascent (metrics)
                             + pango_font_metrics_get_descent (metrics)) / PANGO_SCALE;
        double factor = height*pixels_per_cm/total;
        double line_height;

        if (flags & DUMP_ABSOLUTE)
          pango_font_description_set_absolute_size (pfd, factor * height * pixels_per_cm * PANGO_SCALE);
        else {
          pango_font_description_set_size (pfd, factor * height * pixels_per_cm * PANGO_SCALE);
        }

        pango_font_metrics_unref (metrics);
        g_clear_object (&font);

        font = pango_context_load_font (context, pfd);
        metrics = pango_font_get_metrics (font, NULL);

        line_height = ((double) pango_font_metrics_get_ascent (metrics)
                              + pango_font_metrics_get_descent (metrics)) / PANGO_SCALE;
        fprintf (f, "\t%.3g", flags & DUMP_FACTORS ? factor : line_height);
        g_clear_object (&font);
      }
      pango_font_description_free (pfd);
    }
    fprintf (f, "\n");
  }
}

int
main (int argc, char **argv)
{
  guint flags = DUMP_ABSOLUTE;
  int i;
  PangoContext *context;

  for (i = 1; i < argc; ++i) {
    if (strcmp (argv[i], "--factors") == 0)
      flags |= DUMP_FACTORS;
    else if (strcmp (argv[i], "--relative") == 0)
      flags &= ~(DUMP_ABSOLUTE);
  }
#ifdef G_OS_WIN32
  context = pango_win32_get_context ();
#else
  context = pango_ft2_get_context(75,75);
#endif

  dump_font_sizes (context, fdopen(1, "wb"), flags);
}
