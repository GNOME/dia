/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * pdf.c - poppler based PDF import plug-in for Dia
 * Copyright (C) 2012 Hans Breuer <hans@breuer.org>
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

#include "filter.h"
#include "plug-ins.h"

#ifndef HAVE_POPPLER
static gboolean
no_import_pdf(const gchar *filename, DiagramData *dia, DiaContext *ctx, void* user_data)
{
  dia_context_add_message (ctx, _("PDF import not available."));
  return FALSE;
}
#endif

gboolean import_pdf(const gchar *filename, DiagramData *dia, DiaContext *ctx, void* user_data);

static const gchar *pdf_extensions[] = { "pdf", NULL };
static DiaImportFilter pdf_import_filter = {
    N_("Portable Document File"),
    pdf_extensions,
#ifdef HAVE_POPPLER
    import_pdf,
#else
    no_import_pdf,
#endif
    NULL, /* user data */
    "pdf"
};

/* --- dia plug-in interface --- */
DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "PDF",
                            _("PDF import filter"),
                              NULL, NULL))
      return DIA_PLUGIN_INIT_ERROR;
  filter_register_import(&pdf_import_filter);
  return DIA_PLUGIN_INIT_OK;
}

