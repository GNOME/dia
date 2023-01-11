/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * Custom Lines -- line shapes defined in XML rather than C.
 * Based on the original Custom Objects plugin.
 * Copyright (C) 1999 James Henstridge.
 *
 * Adapted for Custom Lines plugin by Marcel Toele.
 * Modifications (C) 2007 Kern Automatiseringsdiensten BV.
 *
 * Rewrite to use only public API (C) 2008 Hans Breuer
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

#include <gmodule.h>
#include <math.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "object.h"

#include "line_info.h"
#include "custom_linetypes.h"

#include "properties.h"
#include "propinternals.h"

#include "pixmaps/default.xpm"

static void customline_apply_properties( DiaObject* line, LineInfo* info );
static DiaObject* customline_create(Point *startpoint,
                                    void *user_data,
                                    Handle **handle1,
                                    Handle **handle2);
static DiaObject *custom_zigzagline_load (ObjectNode obj_node, int version, DiaContext *ctx);
static DiaObject *custom_polyline_load   (ObjectNode obj_node, int version, DiaContext *ctx);
static DiaObject *custom_bezierline_load (ObjectNode obj_node, int version, DiaContext *ctx);

static void customline_save (DiaObject *object, ObjectNode obj_node, DiaContext *ctx);

static ObjectTypeOps
custom_zigzagline_type_ops = {
  (CreateFunc)customline_create,   /* create */
  (LoadFunc)  custom_zigzagline_load,     /* load */
  (SaveFunc)  customline_save,     /* save */
  (GetDefaultsFunc)   NULL /* get_defaults*/,
  (ApplyDefaultsFunc) NULL /* apply_defaults*/
};

static ObjectTypeOps
custom_polyline_type_ops = {
  (CreateFunc)customline_create,   /* create */
  (LoadFunc)  custom_polyline_load,     /* load */
  (SaveFunc)  customline_save,     /* save */
  (GetDefaultsFunc)   NULL /* get_defaults*/,
  (ApplyDefaultsFunc) NULL /* apply_defaults*/
};

static ObjectTypeOps
custom_bezierline_type_ops = {
  (CreateFunc)customline_create,   /* create */
  (LoadFunc)  custom_bezierline_load, /* load */
  (SaveFunc)  customline_save,     /* save */
  (GetDefaultsFunc)   NULL /* get_defaults*/,
  (ApplyDefaultsFunc) NULL /* apply_defaults*/
};

/* our delegates types, intialized on demand */
static DiaObjectType *polyline_ot = NULL;
static DiaObjectType *bezier_ot = NULL;
static DiaObjectType *zigzag_ot = NULL;


static gboolean
ensure_standard_types (void)
{
  if (!zigzag_ot)
    zigzag_ot = object_get_type ("Standard - ZigZagLine");
  if (!polyline_ot)
    polyline_ot = object_get_type ("Standard - PolyLine");
  if (!bezier_ot)
    bezier_ot = object_get_type ("Standard - BezierLine");

  return (polyline_ot && bezier_ot && zigzag_ot);
}

static DiaObject *
_custom_zigzagline_load (ObjectNode obj_node, int version, DiaContext *ctx, DiaObjectType *delegate)
{
  DiaObject *obj;
  DiaObjectType *ot;
  LineInfo *line_info;
  xmlChar *typestr;

  typestr = xmlGetProp(obj_node, (const xmlChar *)"type");
  ot = object_get_type ((gchar *)typestr);
  line_info = ot->default_user_data;

  if (typestr)
    xmlFree(typestr);

  obj = delegate->ops->load (obj_node, version, ctx);
  obj->type = line_info->object_type;

  return obj;
}
static DiaObject *
custom_zigzagline_load (ObjectNode obj_node, int version, DiaContext *ctx)
{
  ensure_standard_types ();
  if (!zigzag_ot) {
    g_warning ("Can't delegate to 'Standard - ZigZagLine'");
    return NULL;
  }
  return _custom_zigzagline_load(obj_node, version, ctx, zigzag_ot);
}
static DiaObject *
custom_polyline_load (ObjectNode obj_node, int version, DiaContext *ctx)
{
  ensure_standard_types ();
  if (!polyline_ot) {
    g_warning ("Can't delegate to 'Standard - PolyLine'");
    return NULL;
  }
  return _custom_zigzagline_load(obj_node, version, ctx, polyline_ot);
}
static DiaObject *
custom_bezierline_load (ObjectNode obj_node, int version, DiaContext *ctx)
{
  ensure_standard_types ();
  if (!bezier_ot) {
    g_warning ("Can't delegate to 'Standard - BezierLine'");
    return NULL;
  }
  return _custom_zigzagline_load(obj_node, version, ctx, bezier_ot);
}

static void
customline_save (DiaObject *object, ObjectNode obj_node, DiaContext *ctx)
{
  g_assert (object->type &&  object->type->ops && object->type->ops->save);

  if (!ensure_standard_types()) {
    g_warning ("Can't create standard types");
    return;
  }

  if (object->type->ops == &custom_zigzagline_type_ops)
    zigzag_ot->ops->save (object, obj_node, ctx);
  else if (object->type->ops == &custom_polyline_type_ops)
    polyline_ot->ops->save (object, obj_node, ctx);
  else if (object->type->ops == &custom_bezierline_type_ops)
    bezier_ot->ops->save (object, obj_node, ctx);
  else
    g_warning ("customline_save() no delegate");
}

/* the order here must match the one in customline_apply_properties */
static PropDescription
_customline_prop_descs[] = {
  { "line_colour", PROP_TYPE_COLOUR },
  { "line_style", PROP_TYPE_LINESTYLE },
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH },
  { "corner_radius", PROP_TYPE_REAL },
  { "start_arrow", PROP_TYPE_ARROW },
  { "end_arrow", PROP_TYPE_ARROW },
   PROP_DESC_END
};

void
customline_apply_properties( DiaObject* line, LineInfo* info )
{
  GPtrArray *props;
  LinestyleProperty *lsprop;
  ColorProperty     *cprop;
  RealProperty      *rprop;
  ArrowProperty     *aprop;

  props = prop_list_from_descs( _customline_prop_descs, pdtpp_true );
  g_assert( props->len == 6 );

  /* order/index/type must match _customline_prop_descs */
  cprop = g_ptr_array_index( props, 0 );
  cprop->color_data = info->line_color;

  lsprop = g_ptr_array_index( props, 1 );
  lsprop->style = info->line_style;
  lsprop->dash = info->dashlength;

  rprop = g_ptr_array_index( props, 2 );
  rprop->real_data = info->line_width;

  rprop = g_ptr_array_index( props, 3 );
  rprop->real_data = info->corner_radius;

  aprop = g_ptr_array_index( props, 4 );
  aprop->arrow_data = info->start_arrow;

  aprop = g_ptr_array_index( props, 5 );
  aprop->arrow_data = info->end_arrow;

  line->ops->set_props( line, props );

  prop_list_free(props);
}

DiaObject *
customline_create(Point *startpoint,
                  void *user_data,
                  Handle **handle1,
                  Handle **handle2)
{
  DiaObject* res = NULL;
  LineInfo* line_info = (LineInfo*)user_data;

  if (!ensure_standard_types()) {
    g_warning ("Can't create standard types.");
    return NULL;
  }

  /* last chance to make the type information complete */
  if (!line_info->object_type->prop_offsets)
  {
    DiaObjectType *ot = line_info->object_type;

    if (line_info->type == CUSTOM_LINETYPE_ZIGZAGLINE)
      ot->prop_offsets = zigzag_ot->prop_offsets;
    else if (line_info->type == CUSTOM_LINETYPE_POLYLINE)
      ot->prop_offsets = polyline_ot->prop_offsets;
    else if (line_info->type == CUSTOM_LINETYPE_BEZIERLINE)
      ot->prop_offsets = bezier_ot->prop_offsets;
    else
      g_warning("INTERNAL: CustomLines: Illegal line type in LineInfo object %s.", ot->name);
  }

  if (line_info->type == CUSTOM_LINETYPE_ZIGZAGLINE)
    res = zigzag_ot->ops->create( startpoint, NULL, handle1, handle2 );
  else if (line_info->type == CUSTOM_LINETYPE_POLYLINE)
    res = polyline_ot->ops->create( startpoint, NULL, handle1, handle2 );
  else if (line_info->type == CUSTOM_LINETYPE_BEZIERLINE)
    res = bezier_ot->ops->create( startpoint, NULL, handle1, handle2 );
  else
    g_warning(_("INTERNAL: CustomLines: Illegal line type in LineInfo object."));

  if (res) {
    customline_apply_properties (res, line_info);
    res->type = line_info->object_type;
  }

  return( res );
}

void
custom_linetype_new(LineInfo *info, DiaObjectType **otype)
{
  DiaObjectType *obj = g_new0(DiaObjectType, 1);

  obj->version = 1;
  obj->pixmap = default_xpm;

  if (info->type == CUSTOM_LINETYPE_ZIGZAGLINE)
    obj->ops = &custom_zigzagline_type_ops;
  else if (info->type == CUSTOM_LINETYPE_POLYLINE)
    obj->ops = &custom_polyline_type_ops;
  else if (info->type == CUSTOM_LINETYPE_BEZIERLINE)
    obj->ops = &custom_bezierline_type_ops;
  else
    g_warning("INTERNAL: CustomLines: Illegal line type in LineInfo object %s.", obj->name);

  obj->name = info->name;
  obj->default_user_data = info;
  /* we have to either intialize both, or ...
   * provide our own vtable entries for describe_props and get_props
   */
  obj->prop_descs = _customline_prop_descs;
  if (ensure_standard_types())
  {
    if (info->type == CUSTOM_LINETYPE_ZIGZAGLINE)
      obj->prop_offsets = zigzag_ot->prop_offsets;
    else if (info->type == CUSTOM_LINETYPE_POLYLINE)
      obj->prop_offsets = polyline_ot->prop_offsets;
    else if (info->type == CUSTOM_LINETYPE_BEZIERLINE)
      obj->prop_offsets = bezier_ot->prop_offsets;
    else
      g_warning("INTERNAL: CustomLines: Illegal line type in LineInfo object %s.", obj->name);
  }

  if (info->icon_filename) {
    if (g_file_test (info->icon_filename, G_FILE_TEST_EXISTS)) {
      obj->pixmap = NULL;
      obj->pixmap_file = info->icon_filename;
    } else {
      g_warning ("Cannot open icon file %s for object type '%s'.",
                 info->icon_filename,
                 obj->name);
    }
  }

  info->object_type = obj; /* <-- Reciproce type linking */

  obj->default_user_data = (void*)info;

  *otype = obj;
}
