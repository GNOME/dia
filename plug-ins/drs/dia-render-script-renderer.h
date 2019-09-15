/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * dia-render-script.h - plugin for dia
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
#include "diarenderer.h"
#include <libxml/tree.h>

G_BEGIN_DECLS

#define DRS_TYPE_RENDERER           (drs_renderer_get_type ())
#define DRS_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DRS_TYPE_RENDERER, DrsRenderer))
#define DRS_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DRS_TYPE_RENDERER, DrsRendererClass))
#define DRS_IS_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DRS_TYPE_RENDERER))
#define DRS_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DRS_TYPE_RENDERER, DrsRendererClass))

GType drs_renderer_get_type (void) G_GNUC_CONST;

typedef struct _DrsRenderer DrsRenderer;
typedef struct _DrsRendererClass DrsRendererClass;

/*!
 * \brief Dia RenderScript Renderer
 * The DRS renderer implements an XML file format around Dia's renderer interface.
 * It is useful for testing of _DiaObject draw implementations. The XML output includes
 * not only all the rendering commands, but also the object properties and types used.
 * This can be used with the \ref DiaRenderScriptImport to restore objects from file
 * whether the original type is available or not.
 * \extends _DiaRenderer
 */
struct _DrsRenderer
{
  DiaRenderer parent_instance;

  gboolean save_props;

  /*<  private >*/
  /*! current root */
  xmlNodePtr root;
  /* track the parents */
  GQueue *parents;
  /* track transformation matrix */
  GQueue *matrices;
  /* to actually render transformed */
  DiaRenderer *transformer;
  /* initially NULL, only to be used during export_data */
  DiaContext *ctx;

  DiaFont *font;
  double font_height;
};

struct _DrsRendererClass
{
  DiaRendererClass parent_class;
};

G_END_DECLS
