/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 * Major restructuration done in August 2001 by C. Chepelov
 *
 * proplist.c: Property list handling routines.
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

#include "properties.h"
#include "propinternals.h"
#include "dia_xml.h"

/* ------------------------------------------------------------------------- */
/* Construction of a list of properties from a filtered list of descriptors. */
/* This is a little halfway between properties and property descriptor       */
/* lists...                                                                  */
gboolean pdtpp_true(const PropDescription *pdesc)
{ return TRUE; }
gboolean pdtpp_synthetic(const PropDescription *pdesc)
{ return TRUE; }
gboolean pdtpp_from_object(const PropDescription *pdesc)
{ return TRUE; }
gboolean pdtpp_is_visible(const PropDescription *pdesc)
{ return (pdesc->flags & PROP_FLAG_VISIBLE) != 0; }
gboolean pdtpp_is_visible_no_standard(const PropDescription *pdesc)
{ return (pdesc->flags & PROP_FLAG_VISIBLE) != 0 &&
         (pdesc->flags & PROP_FLAG_STANDARD) == 0; }
gboolean pdtpp_is_not_visible(const PropDescription *pdesc)
{ return (pdesc->flags & PROP_FLAG_VISIBLE) == 0; }
gboolean pdtpp_do_save(const PropDescription *pdesc)
{ return (pdesc->flags & (PROP_FLAG_DONT_SAVE|PROP_FLAG_LOAD_ONLY)) == 0; }
gboolean pdtpp_do_save_no_standard(const PropDescription *pdesc)
{ return (pdesc->flags & (PROP_FLAG_DONT_SAVE|PROP_FLAG_LOAD_ONLY|PROP_FLAG_STANDARD)) == 0; }
gboolean pdtpp_do_load(const PropDescription *pdesc)
{ return (((pdesc->flags & PROP_FLAG_DONT_SAVE) == 0) ||
            ((pdesc->flags & PROP_FLAG_LOAD_ONLY) != 0)); }
gboolean pdtpp_defaults(const PropDescription *pdesc)
{ return (pdesc->flags & PROP_FLAG_NO_DEFAULTS) == 0; }
gboolean pdtpp_do_not_save(const PropDescription *pdesc)
{ return (pdesc->flags & PROP_FLAG_DONT_SAVE) != 0; }

GPtrArray *
prop_list_from_descs(const PropDescription *plist,
                     PropDescToPropPredicate pred)
{
  GPtrArray *ret;
  guint count = 0, i;

  prop_desc_list_calculate_quarks((PropDescription *)plist);

  for (i = 0; plist[i].name != NULL; i++)
    if (pred(&plist[i])) count++;

  ret = g_ptr_array_new();
  g_ptr_array_set_size(ret,count);

  count = 0;
  for (i = 0; plist[i].name != NULL; i++) {
#if 0
      g_message("about to append property %s/%s to list"
              "predicate is %s %d %d",plist[i].type,plist[i].name,
              pred(&plist[i])?"TRUE":"FALSE",
              plist[i].flags,plist[i].flags & PROP_FLAG_DONT_SAVE);
#endif
    if (pred(&plist[i])) {
      Property *prop = plist[i].ops->new_prop(&plist[i],pred);
      g_ptr_array_index(ret,count++) = prop;
    }
  }

  return ret;
}

void
prop_list_free(GPtrArray *plist)
{
  guint i;
  if (!plist) return;

  for (i = 0; i < plist->len; i++) {
    Property *prop = g_ptr_array_index(plist,i);
    prop->ops->free(prop);
  }
  g_ptr_array_free(plist,TRUE);
}

/* copies the whole property structure, including the data. */
GPtrArray *
prop_list_copy(GPtrArray *src)
{
  guint i;
  GPtrArray *dest;

  dest = g_ptr_array_new();
  g_ptr_array_set_size(dest, src->len);

  for (i=0; i < src->len; i++) {
    Property *psrc = g_ptr_array_index(src,i);
    Property *pdest = psrc->ops->copy(psrc);
    g_ptr_array_index(dest,i) = pdest;
  }
  return dest;
}

/* copies the whole property structure, excluding the data. */
GPtrArray *
prop_list_copy_empty(GPtrArray *plist)
{
  guint i;
  GPtrArray *dest;

  dest = g_ptr_array_new();
  g_ptr_array_set_size(dest, plist->len);

  for (i=0; i < plist->len; i++) {
    Property *psrc = g_ptr_array_index(plist,i);
    Property *pdest = psrc->ops->new_prop(psrc->descr,psrc->reason);
    g_ptr_array_index(dest,i) = pdest;
  }
  return dest;
}

gboolean
prop_list_load(GPtrArray *props, DataNode data_node, DiaContext *ctx)
{
  guint i;
  gboolean ret = TRUE;

  for (i = 0; i < props->len; i++) {
    Property *prop = g_ptr_array_index(props,i);
    AttributeNode attr = object_find_attribute(data_node, prop->descr->name);
    DataNode data = attr ? attribute_first_data(attr) : NULL;
    if ((!attr || !data) && prop->descr->flags & PROP_FLAG_OPTIONAL) {
      prop->experience |= PXP_NOTSET;
      continue;
    }
    if ((!attr) || (!data)) {
      dia_context_add_message(ctx,
			      _("No attribute '%s' (%p) or no data (%p) in this attribute"),
			      prop->descr->name,attr,data);
      prop->experience |= PXP_NOTSET;
      ret = FALSE;
      continue;
    }
    prop->ops->load(prop,attr,data,ctx);
  }
  return ret;
}

void
prop_list_save(GPtrArray *props, DataNode data, DiaContext *ctx)
{
  guint i;
  for (i = 0; i < props->len; i++) {
    Property *prop = g_ptr_array_index(props,i);
    AttributeNode attr = new_attribute(data,prop->descr->name);
    prop->ops->save(prop,attr,ctx);
  }
}

/*!
 * \brief Search a property list for a named property
 *
 * Check if a given property is included in the list
 * With a better naming convention this function would be
 * called prop_list_find_prop_by_name()
 */
Property *
find_prop_by_name (const GPtrArray *props, const char *name)
{
  guint i;
  GQuark prop_quark = g_quark_from_string(name);

  for (i = 0; i < props->len; i++) {
    Property *prop = g_ptr_array_index(props,i);
    if (prop->name_quark == prop_quark) return prop;
  }
  return NULL;
}


Property *
find_prop_by_name_and_type (const GPtrArray *props,
                            const char      *name,
                            PropertyType     type)
{
  Property *ret = find_prop_by_name(props,name);
  GQuark type_quark = g_quark_from_string(type);
  if (!ret) return NULL;
  if (type_quark != ret->type_quark) return NULL;
  return ret;
}


void
prop_list_add_list (GPtrArray *props, const GPtrArray *ptoadd)
{
  guint i;
  for (i = 0 ; i < ptoadd->len; i++) {
    Property *prop = g_ptr_array_index(ptoadd,i);

    g_ptr_array_add(props,prop->ops->copy(prop));
  }
}

GPtrArray *
prop_list_from_single(Property *prop)
{
  GPtrArray *plist = g_ptr_array_new();
  g_ptr_array_add(plist,prop);
  return plist;
}

void
prop_list_add_line_width  (GPtrArray *plist, real line_width)
{
  Property *prop = make_new_prop (PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, 0);

  ((RealProperty *)prop)->real_data = line_width;
  g_ptr_array_add (plist, prop);
}


void
prop_list_add_line_style (GPtrArray    *plist,
                          DiaLineStyle  line_style,
                          double        dash)
{
  Property *prop = make_new_prop ("line_style", PROP_TYPE_LINESTYLE, 0);

  ((LinestyleProperty *) prop)->style = line_style;
  ((LinestyleProperty *) prop)->dash = dash;
  g_ptr_array_add (plist, prop);
}


static void
_prop_list_add_colour (GPtrArray *plist, const char *name, const Color *color)
{
  Property *prop = make_new_prop (name, PROP_TYPE_COLOUR, 0);

  ((ColorProperty *)prop)->color_data = *color;
  g_ptr_array_add (plist, prop);
}
void
prop_list_add_line_colour (GPtrArray *plist, const Color *color)
{
  _prop_list_add_colour (plist, "line_colour", color);
}
void
prop_list_add_fill_colour (GPtrArray *plist, const Color *color)
{
  _prop_list_add_colour (plist, "fill_colour", color);
}
void
prop_list_add_show_background (GPtrArray *plist, gboolean fill)
{
  Property *prop = make_new_prop ("show_background", PROP_TYPE_BOOL, 0);

  ((BoolProperty *)prop)->bool_data = fill;
  g_ptr_array_add (plist, prop);
}
void
prop_list_add_point (GPtrArray *plist, const char *name, const Point *point)
{
  Property *prop = make_new_prop (name, PROP_TYPE_POINT, 0);

  ((PointProperty *)prop)->point_data = *point;
  g_ptr_array_add (plist, prop);
}
void
prop_list_add_real (GPtrArray *plist, const char *name, real value)
{
  Property *prop = make_new_prop (name, PROP_TYPE_REAL, 0);

  ((RealProperty *)prop)->real_data = value;
  g_ptr_array_add (plist, prop);
}
void
prop_list_add_fontsize (GPtrArray *plist, const char *name, real value)
{
  Property *prop = make_new_prop (name, PROP_TYPE_FONTSIZE, 0);

  ((RealProperty *)prop)->real_data = value;
  g_ptr_array_add (plist, prop);
}


void
prop_list_add_string (GPtrArray *plist, const char *name, const char *value)
{
  Property *prop = make_new_prop (name, PROP_TYPE_STRING, 0);

  g_clear_pointer (&((StringProperty *) prop)->string_data, g_free);
  ((StringProperty *)prop)->string_data = g_strdup (value);
  g_ptr_array_add (plist, prop);
}


void
prop_list_add_text (GPtrArray *plist, const char *name, const char *value)
{
  Property *prop = make_new_prop (name, PROP_TYPE_TEXT, 0);

  g_clear_pointer (&((TextProperty *) prop)->text_data, g_free);
  ((TextProperty *)prop)->text_data = g_strdup (value);
  g_ptr_array_add (plist, prop);
}


void
prop_list_add_filename (GPtrArray *plist, const char *name, const char *value)
{
  Property *prop = make_new_prop (name, PROP_TYPE_FILE, 0);

  g_clear_pointer (&((StringProperty *) prop)->string_data, g_free);
  ((StringProperty *)prop)->string_data = g_strdup (value);
  g_ptr_array_add (plist, prop);
}


void
prop_list_add_font (GPtrArray *plist, const char *name, DiaFont *font)
{
  Property *prop = make_new_prop (name, PROP_TYPE_FONT, 0);
  FontProperty *fp = (FontProperty *)prop;

  if (g_set_object (&fp->font_data, font)) {
    g_ptr_array_add (plist, prop);
  }
}


void
prop_list_add_enum (GPtrArray *plist, const char *name, int value)
{
  Property *prop = make_new_prop (name, PROP_TYPE_ENUM, 0);

  ((EnumProperty *)prop)->enum_data = value;
  g_ptr_array_add (plist, prop);
}


void
prop_list_add_text_colour (GPtrArray *plist, const Color *color)
{
  _prop_list_add_colour (plist, "text_colour", color);
}


void
prop_list_add_matrix (GPtrArray *plist, const DiaMatrix *m)
{
  Property *prop = make_new_prop ("matrix", PROP_TYPE_MATRIX, 0);

  g_clear_pointer (&((MatrixProperty *) prop)->matrix, g_free);
  (( MatrixProperty *) prop)->matrix = g_memdup2 (m, sizeof (DiaMatrix));
  g_ptr_array_add (plist, prop);
}
