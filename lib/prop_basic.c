/* Dia -- a diagram creation/manipulation program -*- c -*-
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 * Major restructuration done in August 2001 by C. Chepelov
 *
 * Basic Property types definition.
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

#include <string.h>

#include <gtk/gtk.h>
#define WIDGET GtkWidget

#include "properties.h"
#include "prop_basic.h"
#include "propinternals.h"

typedef struct {
  const gchar *name;
  const gchar *type;
} PropMoniker;

static guint
desc_hash_hash(const PropMoniker *desc)
{
  guint name_hash = g_str_hash(desc->name);
  guint type_hash = g_str_hash(desc->type);

  return (name_hash ^ ((type_hash & 0xFFFF0000) >> 16) ^
          ((type_hash & 0x0000FFFF) << 16) ^ (type_hash));
}

static guint
desc_hash_compare(const PropMoniker *a, const PropMoniker *b)
{
  return ((0 == strcmp(a->name,b->name)) && (0 == strcmp(a->type,b->type)));
}

Property *
make_new_prop(const char *name, PropertyType type, guint flags)
{
  static GHashTable *hash = NULL;
  PropMoniker *moniker = g_new0(PropMoniker, 1);
  PropDescription *descr;

  moniker->name = name;
  moniker->type = type;

  if (!hash) hash = g_hash_table_new((GHashFunc)desc_hash_hash,
                                     (GCompareFunc)desc_hash_compare);

  descr = g_hash_table_lookup (hash,moniker);
  if (descr) {
    g_clear_pointer (&moniker, g_free);
  } else {
    descr = g_new0 (PropDescription,1);
    descr->name = name;
    descr->type = type;
    descr->flags = flags;
    descr->quark = g_quark_from_static_string(descr->name);
    descr->type_quark = g_quark_from_static_string(descr->type);
    descr->ops = prop_type_get_ops(type);
    g_hash_table_insert(hash,moniker,descr);
    /* we don't ever free anything allocated here. */
  }
  return descr->ops->new_prop(descr,pdtpp_synthetic);
}


/**************************************************************************/
/* The COMMON property type. prop->ops will always point to these ops,    */
/* whose role is to update the prop->experience flag field and then call  */
/* the original property routines. Exception is commonprop_new which does */
/* not exist.                                                             */
/**************************************************************************/

static Property *
commonprop_new(const PropDescription *pdesc, PropDescToPropPredicate reason)
{
  /* Will be called mostly when doing copies and empty copies.
   Will NOT be called upon the creation of an original property. */
  return pdesc->ops->new_prop(pdesc,reason);
}

static void
commonprop_free(Property *prop)
{
  prop->real_ops->free(prop);
}

static Property *
commonprop_copy(Property *src)
{
  Property *prop = src->real_ops->copy(src);
  src->experience |= PXP_COPIED;
  prop->experience |= PXP_COPY;
  return prop;
}

static WIDGET *
commonprop_get_widget(Property *prop, PropDialog *dialog)
{
  WIDGET *wid = prop->real_ops->get_widget(prop,dialog);
  prop->experience |= PXP_GET_WIDGET;
  return wid;
}

static void
commonprop_reset_widget(Property *prop, WIDGET *widget)
{
  prop->real_ops->reset_widget(prop,widget);
  /* reset widget ususally emits the change signal on property,
   * but the property itself did not change */
  prop->experience |= PXP_NOTSET;
  prop->experience |= PXP_RESET_WIDGET;
}

static void
commonprop_set_from_widget(Property *prop, WIDGET *widget)
{
  prop->real_ops->set_from_widget(prop,widget);
  prop->experience |= PXP_SET_FROM_WIDGET;
}

static void
commonprop_load(Property *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
{
  prop->real_ops->load(prop,attr,data,ctx);
  prop->experience |= PXP_LOADED;
}

static void
commonprop_save(Property *prop, AttributeNode attr, DiaContext *ctx)
{
  prop->real_ops->save(prop,attr,ctx);
  prop->experience |= PXP_SAVED;
}

static gboolean
commonprop_can_merge(const PropDescription *pd1, const PropDescription *pd2)
{
  return pd1->ops->can_merge(pd1,pd2);
}

static void
commonprop_get_from_offset(const Property *prop,
                           void *base, guint offset, guint offset2)
{
  prop->real_ops->get_from_offset(prop,base,offset,offset2);
  ((Property *)prop)->experience |= PXP_GFO;
}

static void
commonprop_set_from_offset(Property *prop,
                           void *base, guint offset, guint offset2)
{
  prop->real_ops->set_from_offset(prop,base,offset,offset2);
  prop->experience |= PXP_SFO;
}

static const PropertyOps commonprop_ops = {
  (PropertyType_New) commonprop_new,
  (PropertyType_Free) commonprop_free,
  (PropertyType_Copy) commonprop_copy,
  (PropertyType_Load) commonprop_load,
  (PropertyType_Save) commonprop_save,
  (PropertyType_GetWidget) commonprop_get_widget,
  (PropertyType_ResetWidget) commonprop_reset_widget,
  (PropertyType_SetFromWidget) commonprop_set_from_widget,

  (PropertyType_CanMerge) commonprop_can_merge,
  (PropertyType_GetFromOffset) commonprop_get_from_offset,
  (PropertyType_SetFromOffset) commonprop_set_from_offset
};



void initialize_property(Property *prop, const PropDescription *pdesc,
                         PropDescToPropPredicate reason)
{
  prop->reason = reason;
  prop->name_quark = pdesc->quark;
  if (!prop->name_quark) {
    prop->name_quark = g_quark_from_string(prop->descr->name);
    g_error("%s: late quark construction for property %s",
            G_STRFUNC,prop->descr->name);
  }
  prop->type_quark = pdesc->type_quark;
  prop->ops = &commonprop_ops;
  prop->real_ops = pdesc->ops;
  /* if late quark construction, we'll have NULL here */
  prop->descr = pdesc;
  prop->reason = reason;
  /* prop->self remains 0 until we get a widget from this. */
  prop->event_handler = pdesc->event_handler;

  prop->experience = 0;
}

void copy_init_property(Property *dest, const Property *src)
{ /* XXX: inline this ? */
  memcpy(dest,src,sizeof(*dest));
  dest->experience = 0;
}


/**************************************************************************/
/* The NOOP property type. It is mostly useful to serve as both a library */
/* and a cut-n-paste template library; this is the reason why its member  */
/* functions are not declared static (in the general case, they should).  */
/**************************************************************************/

NoopProperty *
noopprop_new(const PropDescription *pdesc, PropDescToPropPredicate reason)
{
  NoopProperty *prop = g_new0 (NoopProperty, 1);
  initialize_property (&prop->common, pdesc, reason);
  return prop;
}


void
noopprop_free (NoopProperty *prop)
{
  g_clear_pointer (&prop, g_free);
}


NoopProperty *
noopprop_copy(NoopProperty *src)
{
  NoopProperty *prop =
    (NoopProperty *)src->common.ops->new_prop(src->common.descr,
                                               src->common.reason);
  copy_init_property(&prop->common,&src->common);
  return prop; /* Yes, this one could be just be _new().
                  It's actually a placeholder. */
}

WIDGET *
noopprop_get_widget(NoopProperty *prop, PropDialog *dialog)
{
  return NULL;
}

void
noopprop_reset_widget(NoopProperty *prop, WIDGET *widget)
{
}

void
noopprop_set_from_widget(NoopProperty *prop, WIDGET *widget)
{
}

void
noopprop_load(NoopProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
{
}

void
noopprop_save(NoopProperty *prop, AttributeNode attr, DiaContext *ctx)
{
}

gboolean
noopprop_can_merge(const PropDescription *pd1, const PropDescription *pd2)
{
  return TRUE;
}

gboolean
noopprop_cannot_merge(const PropDescription *pd1, const PropDescription *pd2)
{
  return TRUE;
}

void
noopprop_get_from_offset(const NoopProperty *prop,
                         void *base, guint offset, guint offset2)
{
}

void
noopprop_set_from_offset(NoopProperty *prop,
                         void *base, guint offset, guint offset2)
{
}

static const PropertyOps noopprop_ops = {
  (PropertyType_New) noopprop_new,
  (PropertyType_Free) noopprop_free,
  (PropertyType_Copy) noopprop_copy,
  (PropertyType_Load) noopprop_load,
  (PropertyType_Save) noopprop_save,
  (PropertyType_GetWidget) noopprop_get_widget,
  (PropertyType_ResetWidget) noopprop_reset_widget,
  (PropertyType_SetFromWidget) noopprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge, /* but we have noopprop_cannot_merge */
  (PropertyType_GetFromOffset) noopprop_get_from_offset,
  (PropertyType_SetFromOffset) noopprop_set_from_offset
};

/***************************************************************************/
/* The INVALID property type. It is mostly useful to serve a library.      */
/* Each time its code is executed, a g_assert_not_reached() will be run... */
/***************************************************************************/

InvalidProperty *
invalidprop_new(const PropDescription *pdesc,
                PropDescToPropPredicate reason)
{
  g_assert_not_reached();
  return NULL;
}

void
invalidprop_free(InvalidProperty *prop)
{
  g_assert_not_reached();
}

InvalidProperty *
invalidprop_copy(InvalidProperty *src)
{
  g_assert_not_reached();
  return NULL;
}

WIDGET *
invalidprop_get_widget(InvalidProperty *prop, PropDialog *dialog)
{
  g_assert_not_reached();
  return NULL;
}

void
invalidprop_reset_widget(InvalidProperty *prop, WIDGET *widget)
{
  g_assert_not_reached();
}

void
invalidprop_set_from_widget(InvalidProperty *prop, WIDGET *widget)
{
  g_assert_not_reached();
}

void
invalidprop_load(InvalidProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
{
  g_assert_not_reached();
}

void
invalidprop_save(InvalidProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  g_assert_not_reached();
}

gboolean
invalidprop_can_merge(const PropDescription *pd1, const PropDescription *pd2)
{
  g_assert_not_reached();
  return TRUE;
}

void
invalidprop_get_from_offset(const InvalidProperty *prop,
                            void *base, guint offset, guint offset2)
{
  g_assert_not_reached();
}

void
invalidprop_set_from_offset(InvalidProperty *prop,
                            void *base, guint offset, guint offset2)
{
  g_assert_not_reached();
}

static const PropertyOps invalidprop_ops = {
  (PropertyType_New) invalidprop_new,
  (PropertyType_Free) invalidprop_free,
  (PropertyType_Copy) invalidprop_copy,
  (PropertyType_Load) invalidprop_load,
  (PropertyType_Save) invalidprop_save,
  (PropertyType_GetWidget) invalidprop_get_widget,
  (PropertyType_ResetWidget) invalidprop_reset_widget,
  (PropertyType_SetFromWidget) invalidprop_set_from_widget,

  (PropertyType_CanMerge) invalidprop_can_merge,
  (PropertyType_GetFromOffset) invalidprop_get_from_offset,
  (PropertyType_SetFromOffset) invalidprop_set_from_offset
};

/***************************************************************************/
/* The UNIMPLEMENTED property type. It is mostly useful to serve a library */
/* for unfinished property types.                                          */
/***************************************************************************/

UnimplementedProperty *
unimplementedprop_new(const PropDescription *pdesc,
                      PropDescToPropPredicate reason)
{
  g_warning("%s: for property %s",G_STRFUNC,pdesc->name);
  return NULL;
}

void
unimplementedprop_free(UnimplementedProperty *prop)
{
  g_warning("%s: for property %s",G_STRFUNC,prop->common.descr->name);
}

UnimplementedProperty *
unimplementedprop_copy(UnimplementedProperty *src)
{
  g_warning("%s: for property %s",G_STRFUNC,src->common.descr->name);
  return NULL;
}

WIDGET *
unimplementedprop_get_widget(UnimplementedProperty *prop, PropDialog *dialog)
{
  g_warning("%s: for property %s",G_STRFUNC,prop->common.descr->name);
  return NULL;
}

void
unimplementedprop_reset_widget(UnimplementedProperty *prop, WIDGET *widget)
{
  g_warning("%s: for property %s",G_STRFUNC,prop->common.descr->name);
}

void
unimplementedprop_set_from_widget(UnimplementedProperty *prop, WIDGET *widget)
{
  g_warning("%s: for property %s",G_STRFUNC,prop->common.descr->name);
}

void
unimplementedprop_load(UnimplementedProperty *prop,
                       AttributeNode attr, DataNode data,
		       DiaContext *ctx)
{
 g_warning("%s: for property %s",G_STRFUNC,prop->common.descr->name);
}

void
unimplementedprop_save(UnimplementedProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  g_warning("%s: for property %s",G_STRFUNC,prop->common.descr->name);
}

gboolean
unimplementedprop_can_merge(const PropDescription *pd1, const PropDescription *pd2)
{
  g_warning("%s: for property %s/%s",G_STRFUNC,pd1->name,pd2->name);
  return FALSE;
}

void
unimplementedprop_get_from_offset(const UnimplementedProperty *prop,
                                  void *base, guint offset, guint offset2)
{
  g_warning("%s: for property %s",G_STRFUNC,prop->common.descr->name);
}

void
unimplementedprop_set_from_offset(UnimplementedProperty *prop,
                                  void *base, guint offset, guint offset2)
{
  g_warning("%s: for property %s",G_STRFUNC,prop->common.descr->name);
}

static const PropertyOps unimplementedprop_ops = {
  (PropertyType_New) unimplementedprop_new,
  (PropertyType_Free) unimplementedprop_free,
  (PropertyType_Copy) unimplementedprop_copy,
  (PropertyType_Load) unimplementedprop_load,
  (PropertyType_Save) unimplementedprop_save,
  (PropertyType_GetWidget) unimplementedprop_get_widget,
  (PropertyType_ResetWidget) unimplementedprop_reset_widget,
  (PropertyType_SetFromWidget) unimplementedprop_set_from_widget,

  (PropertyType_CanMerge) unimplementedprop_can_merge,
  (PropertyType_GetFromOffset) unimplementedprop_get_from_offset,
  (PropertyType_SetFromOffset) unimplementedprop_set_from_offset
};

/* ************************************************************** */

void
prop_basic_register(void)
{
  prop_type_register(PROP_TYPE_NOOP,&noopprop_ops);
  prop_type_register(PROP_TYPE_INVALID,&invalidprop_ops);
  prop_type_register(PROP_TYPE_UNIMPLEMENTED,&unimplementedprop_ops);
}
