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
 * - tweaking renderer parameters otherwise not in the UI, e.g. DiaLineCaps
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

#define G_LOG_DOMAIN "DiaRenderScript"

#include "config.h"

#include <glib/gi18n-lib.h>

#include "filter.h"
#include "plug-ins.h"
#include "diagramdata.h"
#include "dia-io.h"
#include "dia-layer.h"
#include "dia-render-script-renderer.h"
#include "dia-version-info.h"

#include "dia-render-script.h"


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
export_data (DiagramData *data,
             DiaContext  *ctx,
             const char  *filename,
             const char  *diafilename,
             void        *user_data)
{
  DrsRenderer *renderer;
  char *dtd_name;
  xmlDtdPtr dtd;
  xmlDocPtr doc;

  renderer = DRS_RENDERER (g_object_new (DRS_TYPE_RENDERER, NULL));
  /* store also object properties */
  renderer->save_props = (user_data == NULL);
  /* remember context for object_save_props */
  renderer->ctx = g_object_ref (ctx);

  dtd_name = g_strdup_printf ("-//DIA//DTD DRS %s//EN", dia_version_string ());

  /* set up the root node */
  doc = xmlNewDoc ((const xmlChar *) "1.0");
  doc->encoding = xmlStrdup ((const xmlChar *) "UTF-8");
  doc->standalone = FALSE;
  dtd = xmlCreateIntSubset (doc,
                            (const xmlChar *) "drs",
                            (const xmlChar *) dtd_name,
                            (const xmlChar *) "http://projects.gnome.org/dia/dia-render-script.dtd");
  xmlAddChild ((xmlNodePtr) doc, (xmlNodePtr) dtd);
  renderer->root = xmlNewDocNode (doc, NULL, (const xmlChar *) "drs", NULL);
  xmlAddSibling (doc->children, (xmlNodePtr) renderer->root);

  drs_data_render (data, DIA_RENDERER (renderer));

  dia_io_save_document (filename, doc, FALSE, ctx);

  g_clear_pointer (&doc, xmlFreeDoc);
  g_clear_pointer (&dtd_name, g_free);
  g_clear_object (&renderer);

  return TRUE;
}


static const char *extensions[] = { "drs", NULL };
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
