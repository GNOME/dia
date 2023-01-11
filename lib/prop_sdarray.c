/* Dia -- a diagram creation/manipulation program -*- c -*-
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 * Major restructuration done in August 2001 by C. Chepelov
 *
 * Property types for static and dynamic arrays.
 * These arrays are simply arrays of records whose structures are themselves
 * described by property descriptors.
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
#include "prop_sdarray_widget.h"

/******************************************/
/* The SARRAY and DARRAY property types.  */
/******************************************/

static ArrayProperty *
arrayprop_new(const PropDescription *pdesc, PropDescToPropPredicate reason)
{
  ArrayProperty *prop = g_new0(ArrayProperty,1);
  const PropDescCommonArrayExtra *extra = pdesc->extra_data;

  initialize_property(&prop->common, pdesc, reason);
  prop->ex_props = prop_list_from_descs(extra->record,reason);
  prop->records = g_ptr_array_new();
  return prop;
}

static void
arrayprop_freerecords(ArrayProperty *prop) /* but not the array itself. */
{
  guint i;

  for (i=0; i < prop->records->len; i++)
    prop_list_free(g_ptr_array_index(prop->records,i));
}

static void
arrayprop_free(ArrayProperty *prop)
{
  arrayprop_freerecords(prop);
  g_ptr_array_free(prop->records,TRUE);
  g_clear_pointer (&prop, g_free);
}

static ArrayProperty *
arrayprop_copy(ArrayProperty *src)
{
  ArrayProperty *prop =
    (ArrayProperty *)src->common.ops->new_prop(src->common.descr,
                                                src->common.reason);
  guint i;
  copy_init_property(&prop->common,&src->common);
  prop->ex_props = prop_list_copy(src->ex_props);
  prop->records = g_ptr_array_new();
  for (i = 0; i < src->records->len; i++) {
    g_ptr_array_add(prop->records,
                    prop_list_copy(g_ptr_array_index(src->records,i)));
  }
  return prop;
}

static void
arrayprop_load(ArrayProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
{
  const PropDescCommonArrayExtra *extra = prop->common.descr->extra_data;
  DataNode composite;

  arrayprop_freerecords(prop);
  g_ptr_array_set_size(prop->records,0);

  for (composite = data;
       composite != NULL;
       composite = data_next(composite)) {
    GPtrArray *record = prop_list_from_descs(extra->record,
                                             prop->common.reason);
    if (!prop_list_load(record,composite, ctx)) {
      /* context already has the message */
    }
    g_ptr_array_add(prop->records,record);
  }
}

static void
arrayprop_save(ArrayProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  guint i;
  const PropDescCommonArrayExtra *extra = prop->common.descr->extra_data;

  for (i = 0; i < prop->records->len; i++) {
    prop_list_save(g_ptr_array_index(prop->records,i),
                   data_add_composite(attr, extra->composite_type, ctx), ctx);
  }
}

static void
sarrayprop_get_from_offset(ArrayProperty *prop,
                           void *base, guint offset, guint offset2)
{
  const PropDescSArrayExtra *extra = prop->common.descr->extra_data;
  PropOffset *suboffsets = extra->common.offsets;
  guint i;

  prop_offset_list_calculate_quarks(suboffsets);

  arrayprop_freerecords(prop);
  g_ptr_array_set_size(prop->records,extra->array_len);

  for (i=0; i < prop->records->len; i++) {
    void *rec_in_obj = &struct_member(base,
                                      offset + (i * extra->element_size),
                                      char);
    GPtrArray *subprops = prop_list_copy(prop->ex_props);

    do_get_props_from_offsets(rec_in_obj,subprops,suboffsets);

    g_ptr_array_index(prop->records,i) = subprops;
  }
}

static void
sarrayprop_set_from_offset(ArrayProperty *prop,
                         void *base, guint offset, guint offset2)
{
  const PropDescSArrayExtra *extra = prop->common.descr->extra_data;
  PropOffset *suboffsets = extra->common.offsets;
  guint i;

  g_assert(prop->records->len == extra->array_len);

  prop_offset_list_calculate_quarks(suboffsets);

  for (i=0; i < prop->records->len; i++) {
    void *rec_in_obj = &struct_member(base,
                                      offset + (i * extra->element_size),
                                      char);
    GPtrArray *subprops = g_ptr_array_index(prop->records,i);

    do_set_props_from_offsets(rec_in_obj,subprops,suboffsets);
  }
}

static void
darrayprop_get_from_offset(ArrayProperty *prop,
                           void *base, guint offset, guint offset2)
{
  /* This sucks. We do almost exactly like in sarrayprop_get_from_offset(). */
  const PropDescSArrayExtra *extra = prop->common.descr->extra_data;
  PropOffset *suboffsets = extra->common.offsets;
  guint i;
  GList *obj_rec = struct_member(base,offset,GList *);

  prop_offset_list_calculate_quarks(suboffsets);

  arrayprop_freerecords(prop);
  g_ptr_array_set_size(prop->records,0);

  for (obj_rec = g_list_first(obj_rec), i = 0;
       obj_rec != NULL;
       obj_rec = g_list_next(obj_rec), i++) {
    void *rec_in_obj = obj_rec->data;
    GPtrArray *subprops = prop_list_copy(prop->ex_props);

    do_get_props_from_offsets(rec_in_obj,subprops,suboffsets);

    g_ptr_array_add(prop->records,subprops);
  }
}

static GList *
darrayprop_adjust_object_records(ArrayProperty *prop,
                                 const PropDescDArrayExtra *extra,
                                 GList *obj_rec) {
  guint list_len = g_list_length(obj_rec);

  while (list_len > prop->records->len) {
    gpointer rec = obj_rec->data;
    obj_rec = g_list_remove(obj_rec,rec);
    extra->freerec(rec);
    list_len--;
  }
  while (list_len < prop->records->len) {
    obj_rec = g_list_append(obj_rec,extra->newrec());
    list_len++;
  }
  return obj_rec;
}

static void
darrayprop_set_from_offset(ArrayProperty *prop,
                           void *base, guint offset, guint offset2)
{
  /* This sucks. We do almost exactly like in darrayprop_set_from_offset(). */
  const PropDescDArrayExtra *extra = prop->common.descr->extra_data;
  PropOffset *suboffsets = extra->common.offsets;
  guint i;
  GList *obj_rec = struct_member(base,offset,GList *);

  prop_offset_list_calculate_quarks(suboffsets);

  obj_rec = darrayprop_adjust_object_records(prop,extra,obj_rec);
  struct_member(base,offset,GList *) = obj_rec;

  for (obj_rec = g_list_first(obj_rec), i = 0;
       obj_rec != NULL;
       obj_rec = g_list_next(obj_rec), i++) {
    void *rec_in_obj = obj_rec->data;
    GPtrArray *subprops = g_ptr_array_index(prop->records,i);

    do_set_props_from_offsets(rec_in_obj,subprops,suboffsets);
  }
}

/*!
 * Create a dialog containing the list of array properties
 */
static WIDGET *
arrayprop_get_widget(ArrayProperty *prop, PropDialog *dialog)
{
  GtkWidget *ret = _arrayprop_get_widget (prop, dialog);

  return ret;
}

static void
arrayprop_reset_widget(ArrayProperty *prop, WIDGET *widget)
{
  _arrayprop_reset_widget (prop, widget);
}

static void
arrayprop_set_from_widget(ArrayProperty *prop, WIDGET *widget)
{
  _arrayprop_set_from_widget (prop, widget);
}

static gboolean
arrayprop_can_merge(const PropDescription *pd1, const PropDescription *pd2)
{
  if (pd1->extra_data != pd2->extra_data) return FALSE;
  return TRUE;
}

static const PropertyOps sarrayprop_ops = {
  (PropertyType_New) arrayprop_new,
  (PropertyType_Free) arrayprop_free,
  (PropertyType_Copy) arrayprop_copy,
  (PropertyType_Load) arrayprop_load,
  (PropertyType_Save) arrayprop_save,

  (PropertyType_GetWidget) arrayprop_get_widget,
  (PropertyType_ResetWidget) arrayprop_reset_widget,
  (PropertyType_SetFromWidget) arrayprop_set_from_widget,

  (PropertyType_CanMerge) arrayprop_can_merge,
  (PropertyType_GetFromOffset) sarrayprop_get_from_offset,
  (PropertyType_SetFromOffset) sarrayprop_set_from_offset
};

static const PropertyOps darrayprop_ops = {
  (PropertyType_New) arrayprop_new,
  (PropertyType_Free) arrayprop_free,
  (PropertyType_Copy) arrayprop_copy,
  (PropertyType_Load) arrayprop_load,
  (PropertyType_Save) arrayprop_save,

  (PropertyType_GetWidget) arrayprop_get_widget,
  (PropertyType_ResetWidget) arrayprop_reset_widget,
  (PropertyType_SetFromWidget) arrayprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) darrayprop_get_from_offset,
  (PropertyType_SetFromOffset) darrayprop_set_from_offset
};

/* ************************************************************** */

void
prop_sdarray_register(void)
{
  prop_type_register(PROP_TYPE_SARRAY,&sarrayprop_ops);
  prop_type_register(PROP_TYPE_DARRAY,&darrayprop_ops);
}
