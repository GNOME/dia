/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * render_gnomeprint.[ch] -- gnome-print renderer for dia
 * Copyright (C) 1999 James Henstridge
 *
 * diagnomeprintrenderer.[ch] - resurrection as plug-in and porting to
 *                              DiaRenderer interface
 * Copyright (C) 2004 Hans Breuer
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
 
#include "filter.h"
#include "plug-ins.h"

#include "diagnomeprintrenderer.h"

#include <libgnomeprint/gnome-print-ps2.h>
#include <libgnomeprint/gnome-print-pdf.h>
#include <libgnomeprint/gnome-print-svg.h>

/* dia export funtion */
static void
export_data(DiagramData *data, const gchar *filename, 
            const gchar *diafilename, void* user_data)
{
  GnomePrintConfig *config;
  GnomePrintContext *ctx;
  DiaGnomePrintRenderer *renderer;
  real width, height;
  gboolean portrait = data->paper.is_portrait;
  gdouble matrix[6] = { 1.0, 0.0, -1.0, 0.0, 0.0, 0.0 };

  width  = (data->extents.right - data->extents.left);
  height = (data->extents.bottom - data->extents.top);
  // better
  width = data->paper.width * data->paper.scaling;
  height = data->paper.height * data->paper.scaling;

  config = gnome_print_config_default ();
  gnome_print_config_set (config, GNOME_PRINT_KEY_OUTPUT_FILENAME, filename);
  gnome_print_config_set_boolean (config, "Settings.Output.Job.PrintToFile", TRUE);

  gnome_print_config_set (config, GNOME_PRINT_KEY_PAPER_ORIENTATION, 
                          portrait ? "R0" : "R90");
  gnome_print_config_set_double (config, GNOME_PRINT_KEY_PAPER_WIDTH, width);
  gnome_print_config_set_double (config, GNOME_PRINT_KEY_PAPER_HEIGHT, height);

  /* a dirty hack: allow to pass in GnomePrintContext* via user_data */
  if ((int)user_data == 0)
    ctx = gnome_print_ps2_new (config);
  else if ((int)user_data == 1)
    ctx = gnome_print_pdf_new (config);
  else if ((int)user_data == 2)
    ctx = gnome_print_svg_new (config);
  else {
    ctx = GNOME_PRINT_CONTEXT (user_data);
    g_object_ref (ctx);
  }

  renderer = g_object_new (DIA_GNOME_PRINT_TYPE_RENDERER,
                           "config", config,
                           "context", ctx,
                           NULL);

  gnome_print_beginpage (ctx, NULL);

  gnome_print_scale (ctx, 1 * data->paper.scaling, -1 * data->paper.scaling);
  gnome_print_translate (ctx, 0, portrait ? -height : -width);

  data_render(data, DIA_RENDERER(renderer), NULL, NULL, NULL);

  gnome_print_showpage (ctx);

  g_object_unref(renderer);

  gnome_print_context_close (ctx);

  g_object_unref (ctx);
  gnome_print_config_unref (config);
}

static const gchar *ps_extensions[] = { "ps", NULL };
static DiaExportFilter ps_export_filter = {
    N_("GNOME PostScript"),
    ps_extensions,
    export_data,
    (void*)0,
    "ps-gnome" /* unique name */
};

static const gchar *pdf_extensions[] = { "pdf", NULL };
static DiaExportFilter pdf_export_filter = {
    N_("GNOME Portable Document Format"),
    pdf_extensions,
    export_data,
    (void*)1,
    "pdf-gnome"
};

static const gchar *svg_extensions[] = { "svg", NULL };
static DiaExportFilter svg_export_filter = {
    N_("GNOME Scalable Vector Graphic"),
    svg_extensions,
    export_data,
    (void*)2,
    "svg-gnome"
};

static gboolean
_plugin_can_unload (PluginInfo *info)
{
    /* XXX: to make this work something like filter_unregister() would 
     * be required, but is it worth the trouble ?
     */
    return FALSE;
}

static gboolean
_plugin_unload (PluginInfo *info)
{
    return TRUE;
}

/* --- dia plug-in interface --- */

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "GNOME Print",
                            _("GNOME Print based Rendering"),
                            _plugin_can_unload,
                            _plugin_unload))
    return DIA_PLUGIN_INIT_ERROR;

  filter_register_export(&ps_export_filter);
  filter_register_export(&pdf_export_filter);
  filter_register_export(&svg_export_filter);

  return DIA_PLUGIN_INIT_OK;
}
