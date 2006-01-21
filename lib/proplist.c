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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "properties.h"
#include "propinternals.h"
#include "dia_xml.h"
#include "diaerror.h"

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
  int i;
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
  int i;
  GPtrArray *dest;
  
  dest = g_ptr_array_new();
  g_ptr_array_set_size(dest, src->len);

  for (i=0; i < src->len; i++) {
    Property *psrc = g_ptr_array_index(src,i);
    Property *pdest = pdest = psrc->ops->copy(psrc);
    g_ptr_array_index(dest,i) = pdest;
  }
  return dest;
}

/* copies the whole property structure, excluding the data. */
GPtrArray *
prop_list_copy_empty(GPtrArray *plist)
{
  int i;
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
prop_list_load(GPtrArray *props, DataNode data, GError **err)
{
  int i;
  gboolean ret = TRUE;

  for (i = 0; i < props->len; i++) {
    Property *prop = g_ptr_array_index(props,i);
    AttributeNode attr = object_find_attribute(data, prop->name);
    DataNode data = attr ? attribute_first_data(attr) : NULL;
    if ((!attr || !data) && prop->descr->flags & PROP_FLAG_OPTIONAL) {
      prop->experience |= PXP_NOTSET;
      continue;
    }
    if ((!attr) || (!data)) {
      if (err && !*err)
	*err = g_error_new (DIA_ERROR,
                            DIA_ERROR_FORMAT,
			    _("No attribute '%s' (%p) or no data(%p) in this attribute"),
			    prop->name,attr,data);
      prop->experience |= PXP_NOTSET;
      ret = FALSE;
      continue;
    }
    prop->ops->load(prop,attr,data);
  }
  return ret;
}

void 
prop_list_save(GPtrArray *props, DataNode data)
{
  int i;
  for (i = 0; i < props->len; i++) {
    Property *prop = g_ptr_array_index(props,i);
    AttributeNode attr = new_attribute(data,prop->name);
    prop->ops->save(prop,attr);
  }
}

Property *
find_prop_by_name(const GPtrArray *props, const gchar *name) 
{
  int i;
  GQuark prop_quark = g_quark_from_string(name);

  for (i = 0; i < props->len; i++) {
    Property *prop = g_ptr_array_index(props,i);
    if (prop->name_quark == prop_quark) return prop;
  }
  return NULL;
}

Property *
find_prop_by_name_and_type(const GPtrArray *props, const gchar *name,
                           PropertyType type)
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
prop_list_from_single(Property *prop) {
  GPtrArray *plist = g_ptr_array_new();
  g_ptr_array_add(plist,prop);
  return plist;
}
