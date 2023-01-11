/* Dia -- a diagram creation/manipulation program -*- c -*-
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 *
 * Copyright (C) 2013 Hans Breuer
 * Property types for pattern.
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

#include <gtk/gtk.h>
#define WIDGET GtkWidget
#include "properties.h"
#include "propinternals.h"
#include "pattern.h"
#include "prop_pattern.h"
#include "diapatternselector.h"

static PatternProperty *
patternprop_new(const PropDescription *pdesc, PropDescToPropPredicate reason)
{
  PatternProperty *prop = g_new0(PatternProperty,1);

  initialize_property(&prop->common, pdesc, reason);
  /* empty by default */
  prop->pattern = NULL;

  return prop;
}


static void
patternprop_free (PatternProperty *prop)
{
  g_clear_object (&prop->pattern);
  g_free (prop);
}


static PatternProperty *
patternprop_copy(PatternProperty *src)
{
  PatternProperty *prop =
    (PatternProperty *)src->common.ops->new_prop(src->common.descr,
                                              src->common.reason);
  if (src->pattern) /* TODO: rethink on edit - ...copy() ? */
    prop->pattern = g_object_ref (src->pattern);

  return prop;
}

DiaPattern *
data_pattern (DataNode node, DiaContext *ctx)
{
  AttributeNode attr;
  DiaPattern *pattern;
  DiaPatternType type = DIA_LINEAR_GRADIENT;
  guint flags = 0;
  Point p = {0.0, 0.0};
  real r;

  attr = composite_find_attribute(node, "gradient");
  if (attr)
    type = data_int (attribute_first_data(attr), ctx);
  attr = composite_find_attribute(node, "flags");
  if (attr)
    flags = data_int (attribute_first_data(attr), ctx);
  attr = composite_find_attribute(node, "p1");
  if (attr)
    data_point (attribute_first_data(attr), &p, ctx);
  pattern = dia_pattern_new (type, flags, p.x, p.y);
  if (pattern) {
    /* need to apply the radius first as it may limit p2 */
    attr = composite_find_attribute(node, "r");
    if (attr) {
      r = data_real (attribute_first_data(attr), ctx);
      dia_pattern_set_radius (pattern, r);
    }
    attr = composite_find_attribute(node, "p2");
    if (attr) {
      data_point (attribute_first_data(attr), &p, ctx);
      dia_pattern_set_point (pattern, p.x, p.y);
    }
    /* restore color-stops */
    attr =  composite_find_attribute(node, "data");
    if (attr) {
      DataNode data = attribute_first_data(attr);
      guint nvals = attribute_num_data(attr);
      guint i;
      real offset = 0.0;
      Color color = color_black;

      for (i=0; (i < nvals) && data; i++, data = data_next(data)) {
	attr = composite_find_attribute(data, "offset");
	if (attr)
	  offset = data_real (attribute_first_data(attr), ctx);
	attr = composite_find_attribute(data, "color");
	if (attr)
	  data_color (attribute_first_data(attr), &color, ctx);
	dia_pattern_add_color (pattern, offset, &color);
      }
    }
  }
  return pattern;
}

static void
patternprop_load(PatternProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
{
  prop->pattern = data_pattern (data, ctx);
}

typedef struct
{
  AttributeNode node;
  DiaContext   *ctx;
} StopUserData;

static gboolean
_data_add_stop (real ofs, const Color *col, gpointer user_data)
{
  StopUserData *ud = (StopUserData *)user_data;
  AttributeNode attr = ud->node;
  DiaContext *ctx = ud->ctx;
  ObjectNode composite = data_add_composite(attr, "color-stop", ctx);

  data_add_real (composite_add_attribute(composite, "offset"), ofs, ctx);
  data_add_color(composite_add_attribute(composite, "color"), col, ctx);

  return TRUE;
}

void
data_add_pattern (AttributeNode attr, DiaPattern *pattern, DiaContext *ctx)
{
  ObjectNode composite = data_add_composite(attr, "pattern", ctx);
  StopUserData ud;
  DiaPatternType type;
  guint flags;
  Point p1, p2;

  ud.node = composite_add_attribute (composite, "data");
  ud.ctx = ctx;

  dia_pattern_get_settings (pattern, &type, &flags);
  data_add_int (composite_add_attribute(composite, "gradient"), type, ctx);
  data_add_int (composite_add_attribute(composite, "flags"), flags, ctx);
  dia_pattern_get_points (pattern, &p1, &p2);
  data_add_point (composite_add_attribute(composite, "p1"), &p1, ctx);
  data_add_point (composite_add_attribute(composite, "p2"), &p2, ctx);
  if (type == DIA_RADIAL_GRADIENT) {
    real r;
    dia_pattern_get_radius (pattern, &r);
    data_add_real (composite_add_attribute(composite, "r"), r, ctx);
  }
  dia_pattern_foreach (pattern, _data_add_stop, &ud);
}

static void
patternprop_save(PatternProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  if (prop->pattern) {
    data_add_pattern (attr, prop->pattern, ctx);
  }
}

static void
patternprop_get_from_offset(PatternProperty *prop,
                         void *base, guint offset, guint offset2)
{
  /* before we start editing a simple reference should be enough */
  DiaPattern *pattern = struct_member(base,offset,DiaPattern *);

  if (pattern)
    prop->pattern = g_object_ref (pattern);
  else
    prop->pattern = NULL;
}


static void
patternprop_set_from_offset (PatternProperty *prop,
                             void            *base,
                             guint            offset,
                             guint            offset2)
{
  DiaPattern *dest = struct_member (base, offset, DiaPattern *);

  g_clear_object (&dest);

  if (prop->pattern) {
    struct_member (base, offset, DiaPattern *) = g_object_ref (prop->pattern);
  } else {
    struct_member (base, offset, DiaPattern *) = NULL;
  }
}


static GtkWidget *
patternprop_get_widget (PatternProperty *prop, PropDialog *dialog)
{
  GtkWidget *ret = dia_pattern_selector_new ();
  prophandler_connect(&prop->common, G_OBJECT(ret), "value_changed");
  return ret;
}

static void
patternprop_reset_widget(PatternProperty *prop, GtkWidget *widget)
{
  dia_pattern_selector_set_pattern (widget, prop->pattern);
}


static void
patternprop_set_from_widget (PatternProperty *prop, GtkWidget *widget)
{
  DiaPattern *pat = dia_pattern_selector_get_pattern (widget);

  g_set_object (&prop->pattern, pat);
}


static const PropertyOps patternprop_ops = {
  (PropertyType_New) patternprop_new,
  (PropertyType_Free) patternprop_free,
  (PropertyType_Copy) patternprop_copy,
  (PropertyType_Load) patternprop_load,
  (PropertyType_Save) patternprop_save,
  (PropertyType_GetWidget) patternprop_get_widget,
  (PropertyType_ResetWidget) patternprop_reset_widget,
  (PropertyType_SetFromWidget) patternprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) patternprop_get_from_offset,
  (PropertyType_SetFromOffset) patternprop_set_from_offset
};

void
prop_patterntypes_register(void)
{
  prop_type_register(PROP_TYPE_PATTERN, &patternprop_ops);
}
