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

#include <glib/gstdio.h>

#include "intl.h"
#include "render_eps.h"
#include "diagramdata.h"
#include "font.h"
#include "diapsrenderer.h"


static gboolean export_render_eps (DiaPsRenderer *renderer,
                                   DiagramData   *data,
                                   DiaContext    *ctx,
                                   const char    *filename,
                                   const char    *diafilename,
                                   void          *user_data);


static gboolean
export_eps (DiagramData *data,
            DiaContext  *ctx,
            const char  *filename,
            const char  *diafilename,
            void        *user_data)
{
  gboolean ret;
  DiaPsRenderer *renderer = g_object_new (DIA_TYPE_PS_RENDERER, NULL);

  renderer->ctx = ctx;
  ret = export_render_eps (renderer, data, ctx, filename, diafilename, user_data);
  g_clear_object (&renderer);

  return ret;
}


static gboolean
export_render_eps (DiaPsRenderer *renderer,
                   DiagramData   *data,
                   DiaContext    *ctx,
                   const char    *filename,
                   const char    *diafilename,
                   void          *user_data)
{
  FILE *outfile;

  outfile = g_fopen (filename, "w");
  if (outfile == NULL) {
    dia_context_add_message_with_errno (ctx, errno, _("Can't open output file %s"),
                                        dia_context_get_filename(ctx));
    return FALSE;
  }
  renderer->file = outfile;
  renderer->scale = 28.346 * data->paper.scaling;
  renderer->extent = data->extents;
  renderer->pstype = GPOINTER_TO_UINT (user_data);
  if (renderer->pstype & PSTYPE_EPSI) {
    /* Must store the diagram for making a preview */
    renderer->diagram = data;
  }

  if (renderer->file) {
    renderer->title = g_strdup (diafilename);

    data_render(data, DIA_RENDERER(renderer), NULL, NULL, NULL);
  }
  fclose(outfile);

  return TRUE;
}

DiaRenderer *
new_psprint_renderer(DiagramData *dia, FILE *file)
{
  DiaPsRenderer *renderer;

  renderer = g_object_new (DIA_TYPE_PS_RENDERER, NULL);
  renderer->file = file;
  renderer->pstype = PSTYPE_PS;

  return DIA_RENDERER(renderer);
}

static const gchar *eps_extensions[] = { "eps", NULL };
#if 0
static const gchar *epsi_extensions[] = { "epsi", NULL };
#endif

DiaExportFilter eps_export_filter = {
  N_("Encapsulated PostScript (using PS Latin-1 fonts)"),
  eps_extensions,
  export_eps,
  GINT_TO_POINTER(PSTYPE_EPS), /* user_data */
  "eps-builtin"
};
/* Commented out until we can actually make the preview.
DiaExportFilter epsi_export_filter = {
  N_("Encapsulated PostScript with preview (using PostScript Latin-1 fonts)"),
  epsi_extensions,
  export_eps,
  PSTYPE_EPSI,
  "epsi-builtin"
};
*/
