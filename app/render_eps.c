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
 */

/* The eps_dump_truetype_body function and much inspiration for font dumping
 * came from ttfps, which bears the following license notice:

 Copyright (c) 1997 by Juliusz Chroboczek 

 Copying
 *******

 This software is provided with no guarantee, not even of any kind.

 Feel free to do whatever you wish with it as long as you don't ask me
 to maintain it.

*/

/* The Document Structure Definitions used for the output is available at
 * http://www-cdf.fnal.gov/offline/PostScript/psstruct.ps
 * (Appendix G of the Red and White Book)
 */

/* Note: There is a use of setmatrix in ellipse, which is supposed to be
 * avoided.  Could it be?
 */

/* Note that the EPS renderer now has two phases:  One to collect font
 * info (and conceivably more, like color defs), and one to actually render.
 */

#include <config.h>

#include <string.h>
#include <time.h>
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <locale.h>
#include <errno.h>

#include "intl.h"
#include "render_eps.h"
#include "message.h"
#include "diagramdata.h"
#include "font.h"
#include "diapsrenderer.h"

#ifdef HAVE_FREETYPE
#include "diapsft2renderer.h"
#endif

static void
export_eps(DiagramData *data, const gchar *filename, 
           const gchar *diafilename, void* user_data)
{
  DiaPsRenderer *renderer;
  FILE *outfile;

#ifdef HAVE_FREETYPE
  renderer = g_object_new (DIA_TYPE_PS_FT2_RENDERER, NULL);
#else
  renderer = g_object_new (DIA_TYPE_PS_RENDERER, NULL);
#endif
  outfile = fopen(filename, "w");
  if (outfile == NULL) {
    message_error(_("Can't open output file %s: %s\n"), filename, strerror(errno));
    g_object_unref(renderer);
    return;
  }
  renderer->file = outfile;
  renderer->scale = 28.346 * data->paper.scaling;
  renderer->extent = data->extents;
  renderer->is_eps = TRUE;

  if (renderer->file) {
    renderer->title = g_strdup (diafilename);

    data_render(data, DIA_RENDERER(renderer), NULL, NULL, NULL);
  }
  g_object_unref (renderer);
  fclose(outfile);
}

DiaRenderer *
new_psprint_renderer(Diagram *dia, FILE *file)
{
  DiaPsRenderer *renderer;

#ifdef HAVE_FREETYPE
  renderer = g_object_new (DIA_TYPE_PS_FT2_RENDERER, NULL);
#else
  renderer = g_object_new (DIA_TYPE_PS_RENDERER, NULL);
#endif
  renderer->file = file;
  renderer->is_eps = FALSE;

  return DIA_RENDERER(renderer);
}

static const gchar *extensions[] = { "eps", "epsi", NULL };
DiaExportFilter eps_export_filter = {
  N_("Encapsulated Postscript"),
  extensions,
  export_eps
};

