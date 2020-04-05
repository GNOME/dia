/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * dia-render-script.c - plugin for dia
 * Copyright (C) 2009, Hans Breuer, <Hans@Breuer.Org>
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

/* Dia Render File - Serialization of Dia  Renderers.
 * May also provide some DiaObject implementation which could offer parameters like
 * - scale: overall scaling of the 'object diagram' done on the renderer level
 * - tweaking renderer parameters otherwise not in the UI, e.g. LineCaps
 *
 * The format is similar to Windows MetaFile.i.e a serialization of function calls together
 * with their parameters.
 * An XML representation as well as some binary variant could be provided.
 *
 * <diagram>
 *   <meta />
 *   <layer name="" >
 *     <object type="">
 *       <!-- optional stdprops to restore the real object type -->
 *       <properties />
 *       <render>
 *          <set-font />
 *          <draw-rectangle />
 *          <fill-ellipse />
 *          <draw-string />
 *          <!-- ... -->
 *       </render>
 *     </object>
 *   </layer>
 * </diagram>
 */

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define G_LOG_DOMAIN "DiaRenderScript"
#include <glib.h>
#include <glib/gstdio.h>

#include "intl.h"
#include "filter.h"
#include "plug-ins.h"
#include "diagramdata.h"
#include "dia_xml_libxml.h"
#include "dia-layer.h"

#include "dia-render-script.h"
#include "dia-render-script-renderer.h"

static void
drs_render_layer (DiaRenderer *self, DiaLayer *layer, gboolean active)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;
  GList *list;
  DiaObject *obj;

  g_queue_push_tail (renderer->parents, renderer->root);
  renderer->root = node = xmlNewChild(renderer->root, NULL, (const xmlChar *)"layer", NULL);
  xmlSetProp (node,
              (const xmlChar *) "name",
              (xmlChar *) dia_layer_get_name (layer));
  xmlSetProp (node,
              (const xmlChar *) "visible",
              (xmlChar *) (dia_layer_is_visible (layer) ? "true" : "false"));
  if (active) {
    xmlSetProp (node, (const xmlChar *) "active", (xmlChar *) "true");
  }

  /* Draw all objects: */
  list = dia_layer_get_object_list (layer);
  while (list!=NULL) {
    obj = (DiaObject *) list->data;
    dia_renderer_draw_object (self, obj, NULL);
    list = g_list_next (list);
  }

  renderer->root = g_queue_pop_tail (renderer->parents);
}


/*! own version to render invisible layers, too */
static void
drs_data_render (DiagramData *data, DiaRenderer *renderer)
{
  DiaLayer *active = dia_diagram_data_get_active_layer (data);

  dia_renderer_begin_render (renderer, NULL);

  DIA_FOR_LAYER_IN_DIAGRAM (data, layer, i, {
    drs_render_layer (renderer, layer, layer == active);
  });

  dia_renderer_end_render (renderer);
}


/* dia export funtion */
static gboolean
export_data(DiagramData *data, DiaContext *ctx,
	    const gchar *filename, const gchar *diafilename,
	    void* user_data)
{
  DrsRenderer *renderer;
  xmlDtdPtr dtd;
  xmlDocPtr doc;

  /* write check - almost same code in every renderer */
  {
    FILE *file = g_fopen(filename, "w");

    if (!file) {
      dia_context_add_message_with_errno (ctx, errno, _("Can't open output file %s."),
					  dia_context_get_filename(ctx));
      return FALSE;
    }
    fclose(file);
  }
  renderer = DRS_RENDERER (g_object_new(DRS_TYPE_RENDERER, NULL));
  /* store also object properties */
  renderer->save_props = (user_data == NULL);
  /* remember context for object_save_props */
  renderer->ctx = g_object_ref (ctx);

  /* set up the root node */
  doc = xmlNewDoc((const xmlChar *)"1.0");
  doc->encoding = xmlStrdup((const xmlChar *)"UTF-8");
  doc->standalone = FALSE;
  dtd = xmlCreateIntSubset(doc, (const xmlChar *)"drs",
		     (const xmlChar *)"-//DIA//DTD DRS " VERSION "//EN",
		     (const xmlChar *)"http://projects.gnome.org/dia/dia-render-script.dtd");
  xmlAddChild((xmlNodePtr) doc, (xmlNodePtr) dtd);
  renderer->root = xmlNewDocNode(doc, NULL, (const xmlChar *)"drs", NULL);
  xmlAddSibling(doc->children, (xmlNodePtr) renderer->root);

  drs_data_render(data, DIA_RENDERER(renderer));

  xmlSetDocCompressMode (doc, 1);
  xmlDiaSaveFile (filename, doc);
  xmlFreeDoc (doc);

  g_clear_object (&renderer);

  return TRUE;
}

static const gchar *extensions[] = { "drs", NULL };
static DiaExportFilter export_filter = {
  N_("DiaRenderScript"),
  extensions,
  export_data
};
static DiaImportFilter import_filter = {
  N_("DiaRenderScript"),
  extensions,
  import_drs
};

static gboolean
_plugin_can_unload (PluginInfo *info)
{
  return TRUE;
}

static void
_plugin_unload (PluginInfo *info)
{
  filter_unregister_export(&export_filter);
  filter_unregister_import(&import_filter);
}

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "drs",
			    N_("DiaRenderScript filter"),
			    _plugin_can_unload,
                            _plugin_unload))
    return DIA_PLUGIN_INIT_ERROR;

  filter_register_export(&export_filter);
  filter_register_import(&import_filter);

  return DIA_PLUGIN_INIT_OK;
}
