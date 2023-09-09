/* Dia -- a diagram creation/manipulation program -*- c -*-
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 * Major restructuration done in August 2001 by C. Chepelov
 *
 * Property types for integral types (and arrays thereof).
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

#include <stdlib.h> /* strtol */

#include <gtk/gtk.h>
#define WIDGET GtkWidget
#include "properties.h"
#include "propinternals.h"
#include "diacontext.h"

/***************************/
/* The CHAR property type. */
/***************************/

static CharProperty *
charprop_new(const PropDescription *pdesc, PropDescToPropPredicate reason)
{
  CharProperty *prop = g_new0(CharProperty,1);
  initialize_property(&prop->common,pdesc,reason);
  prop->char_data = 0;
  return prop;
}

static CharProperty *
charprop_copy(CharProperty *src)
{
  CharProperty *prop =
    (CharProperty *)src->common.ops->new_prop(src->common.descr,
                                               src->common.reason);
  copy_init_property(&prop->common,&src->common);
  prop->char_data = src->char_data;
  return prop;
}

static WIDGET *
charprop_get_widget(CharProperty *prop, PropDialog *dialog)
{
  GtkWidget *ret = gtk_entry_new();
  gtk_entry_set_activates_default(GTK_ENTRY(ret), TRUE);
  prophandler_connect(&prop->common, G_OBJECT(ret), "changed");
  return ret;
}

static void
charprop_reset_widget(CharProperty *prop, WIDGET *widget)
{
  gchar ch[7];
  int unilen = g_unichar_to_utf8 (prop->char_data, ch);
  ch[unilen] = 0;
  gtk_entry_set_text(GTK_ENTRY(widget), ch);
}

static void
charprop_set_from_widget(CharProperty *prop, WIDGET *widget)
{
  gchar *buf = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, 1);
  prop->char_data = g_utf8_get_char(buf);
  g_clear_pointer (&buf, g_free);
}

static void
charprop_load(CharProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
{
  gchar *str = data_string(data, ctx);

  if (str && str[0]) {
    prop->char_data = g_utf8_get_char(str);
    g_clear_pointer (&str, g_free);
  } else {
    g_warning("Could not read character data for attribute %s",
              prop->common.descr->name);
  }
}

static void
charprop_save(CharProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  gchar utf[7];
  gint n = g_unichar_to_utf8 (prop->char_data, utf);
  utf[n] = 0;
  data_add_string (attr, utf, ctx);
}

static void
charprop_get_from_offset(CharProperty *prop,
                         void *base, guint offset, guint offset2)
{
  prop->char_data = struct_member(base,offset,gunichar);
}

static void
charprop_set_from_offset(CharProperty *prop,
                         void *base, guint offset, guint offset2)
{
  struct_member(base,offset,gunichar) = prop->char_data;
}

static int
charprop_get_data_size(void)
{
  CharProperty prop;
  return sizeof (prop.char_data);
}


static const PropertyOps charprop_ops = {
  (PropertyType_New) charprop_new,
  (PropertyType_Free) noopprop_free,
  (PropertyType_Copy) charprop_copy,
  (PropertyType_Load) charprop_load,
  (PropertyType_Save) charprop_save,
  (PropertyType_GetWidget) charprop_get_widget,
  (PropertyType_ResetWidget) charprop_reset_widget,
  (PropertyType_SetFromWidget) charprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) charprop_get_from_offset,
  (PropertyType_SetFromOffset) charprop_set_from_offset,
  (PropertyType_GetDataSize) charprop_get_data_size
};

/* ************************************ */


/***************************/
/* The BOOL property type. */
/***************************/

static Property *
boolprop_new(const PropDescription *pdesc, PropDescToPropPredicate reason)
{
  BoolProperty *prop = g_new0(BoolProperty,1);
  initialize_property(&prop->common, pdesc, reason);
  prop->bool_data = FALSE;
  return &prop->common;
}

static BoolProperty *
boolprop_copy(BoolProperty *src)
{
  BoolProperty *prop =
    (BoolProperty *)src->common.ops->new_prop(src->common.descr,
                                               src->common.reason);
  copy_init_property(&prop->common,&src->common);
  prop->bool_data = src->bool_data;
  return prop;
}

static WIDGET *
boolprop_get_widget(BoolProperty *prop, PropDialog *dialog)
{
  GtkWidget *ret = gtk_switch_new();
  prophandler_connect_notify(&prop->common, G_OBJECT(ret), "notify::active");
  return ret;
}

static void
boolprop_reset_widget(BoolProperty *prop, WIDGET *widget)
{
  gtk_switch_set_active(GTK_SWITCH(widget),prop->bool_data);
}

static void
boolprop_set_from_widget(BoolProperty *prop, WIDGET *widget)
{
  prop->bool_data = gtk_switch_get_active (GTK_SWITCH(widget));
}

static void
boolprop_load(BoolProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
{
  prop->bool_data = data_boolean(data,ctx);
}

static void
boolprop_save(BoolProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  data_add_boolean(attr, prop->bool_data, ctx);
}

static void
boolprop_get_from_offset(BoolProperty *prop,
                         void *base, guint offset, guint offset2)
{
  prop->bool_data = struct_member(base,offset,gboolean);
}

static void
boolprop_set_from_offset(BoolProperty *prop,
                         void *base, guint offset, guint offset2)
{
  struct_member(base,offset,gboolean) = prop->bool_data;
}

static int
boolprop_get_data_size(void)
{
  BoolProperty prop;
  return sizeof (prop.bool_data);
}


static const PropertyOps boolprop_ops = {
  (PropertyType_New) boolprop_new,
  (PropertyType_Free) noopprop_free,
  (PropertyType_Copy) boolprop_copy,
  (PropertyType_Load) boolprop_load,
  (PropertyType_Save) boolprop_save,
  (PropertyType_GetWidget) boolprop_get_widget,
  (PropertyType_ResetWidget) boolprop_reset_widget,
  (PropertyType_SetFromWidget) boolprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) boolprop_get_from_offset,
  (PropertyType_SetFromOffset) boolprop_set_from_offset,
  (PropertyType_GetDataSize) boolprop_get_data_size
};

/***************************/
/* The INT property type.  */
/***************************/

static IntProperty *
intprop_new(const PropDescription *pdesc, PropDescToPropPredicate reason)
{
  IntProperty *prop = g_new0(IntProperty,1);
  initialize_property(&prop->common, pdesc, reason);
  prop->int_data = 0;
  return prop;
}

static IntProperty *
intprop_copy(IntProperty *src)
{
  IntProperty *prop =
    (IntProperty *)src->common.ops->new_prop(src->common.descr,
                                              src->common.reason);
  copy_init_property(&prop->common,&src->common);
  prop->int_data = src->int_data;
  return prop;
}

static WIDGET *
intprop_get_widget(IntProperty *prop, PropDialog *dialog)
{
  GtkAdjustment *adj = GTK_ADJUSTMENT(gtk_adjustment_new(prop->int_data,
                                                         G_MININT, G_MAXINT,
                                                         1.0, 10.0, 0));
  GtkWidget *ret = gtk_spin_button_new(adj, 1.0, 0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ret),TRUE);
  prophandler_connect(&prop->common, G_OBJECT(ret), "value_changed");

  return ret;
}

static void
intprop_reset_widget(IntProperty *prop, WIDGET *widget)
{
  GtkAdjustment *adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON(widget));
  if (prop->common.descr->extra_data) {
    PropNumData *numdata = prop->common.descr->extra_data;
    gtk_adjustment_configure (adj,
			      prop->int_data,
                              numdata->min, numdata->max,
                              numdata->step, 10.0 * numdata->step, 0);
  } else {
    gtk_adjustment_configure (adj,
			      prop->int_data,
                              G_MININT, G_MAXINT,
                              1.0, 10.0, 0);
  }
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(widget), TRUE);
}

static void
intprop_set_from_widget(IntProperty *prop, WIDGET *widget)
{
  prop->int_data = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}

static void
intprop_load(IntProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
{
  prop->int_data = data_int(data,ctx);
}

static void
intprop_save(IntProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  data_add_int(attr, prop->int_data, ctx);
}

static void
intprop_get_from_offset(IntProperty *prop,
                        void *base, guint offset, guint offset2)
{
  prop->int_data = struct_member(base,offset,gint);
}

static void
intprop_set_from_offset(IntProperty *prop,
                        void *base, guint offset, guint offset2)
{
  struct_member(base,offset,gint) = prop->int_data;
}

static int
intprop_get_data_size(void)
{
  IntProperty prop;
  return sizeof (prop.int_data);
}

static const PropertyOps intprop_ops = {
  (PropertyType_New) intprop_new,
  (PropertyType_Free) noopprop_free,
  (PropertyType_Copy) intprop_copy,
  (PropertyType_Load) intprop_load,
  (PropertyType_Save) intprop_save,
  (PropertyType_GetWidget) intprop_get_widget,
  (PropertyType_ResetWidget) intprop_reset_widget,
  (PropertyType_SetFromWidget) intprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) intprop_get_from_offset,
  (PropertyType_SetFromOffset) intprop_set_from_offset,
  (PropertyType_GetDataSize) intprop_get_data_size
};

/********************************/
/* The INTARRAY property type.  */
/********************************/

static IntarrayProperty *
intarrayprop_new(const PropDescription *pdesc,
                 PropDescToPropPredicate reason)
{
  IntarrayProperty *prop = g_new0(IntarrayProperty,1);
  initialize_property(&prop->common, pdesc, reason);
  prop->intarray_data = g_array_new(FALSE,TRUE,sizeof(gint));
  return prop;
}

static void
intarrayprop_free(IntarrayProperty *prop)
{
  g_array_free(prop->intarray_data,TRUE);
  g_clear_pointer (&prop, g_free);
}

static IntarrayProperty *
intarrayprop_copy(IntarrayProperty *src)
{
  guint i;
  IntarrayProperty *prop =
    (IntarrayProperty *)src->common.ops->new_prop(src->common.descr,
                                                   src->common.reason);
  copy_init_property(&prop->common,&src->common);
  g_array_set_size(prop->intarray_data,src->intarray_data->len);
  for (i = 0 ; i < src->intarray_data->len; i++)
    g_array_index(prop->intarray_data,gint,i) =
      g_array_index(src->intarray_data,gint,i);
  return prop;
}

static void
intarrayprop_load(IntarrayProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
{
  guint nvals = attribute_num_data(attr);
  guint i;
  g_array_set_size(prop->intarray_data,nvals);
  for (i=0; (i < nvals) && data; i++, data = data_next(data))
    g_array_index(prop->intarray_data,gint,i) = data_int(data,ctx);
  if (i != nvals)
    g_warning("attribute_num_data() and actual data count mismatch "
              "(shouldn't happen)");
}

static void
intarrayprop_save(IntarrayProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  guint i;
  for (i = 0; i < prop->intarray_data->len; i++)
    data_add_int(attr, g_array_index(prop->intarray_data,gint,i), ctx);
}


static void
intarrayprop_get_from_offset (IntarrayProperty *prop,
                              void             *base,
                              guint             offset,
                              guint             offset2)
{
  guint nvals = struct_member (base, offset2, guint);
  guint i;
  void *ofs_val = struct_member(base, offset, void *);
  g_array_set_size(prop->intarray_data, nvals);
  for (i = 0; i < nvals; i++) {
    g_array_index (prop->intarray_data, int, i) =
      struct_member (ofs_val, i * sizeof (int), int);
  }
}


static void
intarrayprop_set_from_offset (IntarrayProperty *prop,
                              void             *base,
                              guint             offset,
                              guint             offset2)
{
  guint nvals = prop->intarray_data->len;
  int *vals = g_memdup2 (&g_array_index (prop->intarray_data, int, 0),
                         sizeof (int) * nvals);
  g_clear_pointer (&struct_member (base, offset, int *), g_free);
  struct_member (base, offset, int *) = vals;
  struct_member (base, offset2, guint) = nvals;
}


static const PropertyOps intarrayprop_ops = {
  (PropertyType_New) intarrayprop_new,
  (PropertyType_Free) intarrayprop_free,
  (PropertyType_Copy) intarrayprop_copy,
  (PropertyType_Load) intarrayprop_load,
  (PropertyType_Save) intarrayprop_save,
  (PropertyType_GetWidget) noopprop_get_widget,
  (PropertyType_ResetWidget) noopprop_reset_widget,
  (PropertyType_SetFromWidget) noopprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) intarrayprop_get_from_offset,
  (PropertyType_SetFromOffset) intarrayprop_set_from_offset
};

/***************************/
/* The ENUM property type.  */
/***************************/

static EnumProperty *
enumprop_new(const PropDescription *pdesc, PropDescToPropPredicate reason)
{
  EnumProperty *prop = g_new0(EnumProperty,1);
  initialize_property(&prop->common, pdesc, reason);
  prop->enum_data = 0;
  return prop;
}

static EnumProperty *
enumprop_copy(EnumProperty *src)
{
  EnumProperty *prop =
    (EnumProperty *)src->common.ops->new_prop(src->common.descr,
                                               src->common.reason);
  copy_init_property(&prop->common,&src->common);
  prop->enum_data = src->enum_data;
  return prop;
}

static WIDGET *
enumprop_get_widget(EnumProperty *prop, PropDialog *dialog)
{
  GtkWidget *ret;

  if (prop->common.descr->extra_data) {
    PropEnumData *enumdata = prop->common.descr->extra_data;
    guint i;

    ret = gtk_combo_box_text_new ();

    for (i = 0; enumdata[i].name != NULL; i++)
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (ret), _(enumdata[i].name));

    prophandler_connect(&prop->common, G_OBJECT (ret), "changed");
  } else {
    ret = gtk_entry_new(); /* should use spin button/option menu */
  }
  return ret;
}

static void
enumprop_reset_widget(EnumProperty *prop, WIDGET *widget)
{
  if (prop->common.descr->extra_data) {
    PropEnumData *enumdata = prop->common.descr->extra_data;
    guint i, pos = 0;

    for (i = 0; enumdata[i].name != NULL; i++) {
      if (enumdata[i].enumv == prop->enum_data) {
        pos = i;
        break;
        }
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (widget), pos);
  } else {
    char buf[16];
    g_snprintf(buf, sizeof(buf), "%d", prop->enum_data);
    gtk_entry_set_text(GTK_ENTRY(widget), buf);
  }
}

static void
enumprop_set_from_widget(EnumProperty *prop, WIDGET *widget)
{
  if (GTK_IS_COMBO_BOX (widget)) {
    guint pos = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
    PropEnumData *enumdata = prop->common.descr->extra_data;

    g_return_if_fail (enumdata != NULL);

    prop->enum_data = enumdata[pos].enumv;
  } else {
    prop->enum_data = strtol(gtk_entry_get_text(GTK_ENTRY(widget)), NULL, 0);
  }
}

static void
enumprop_load(EnumProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
{
  DataType dt = data_type (data, ctx);
  if (DATATYPE_ENUM == dt)
    prop->enum_data = data_enum(data, ctx);
  else if (DATATYPE_INT == dt) {
    gboolean cast_ok = FALSE;
    PropEnumData *enumdata = prop->common.descr->extra_data;
    guint i, v = data_int(data,ctx);
    for (i = 0; enumdata[i].name != NULL; ++i) {
      if (v == enumdata[i].enumv) {
        prop->enum_data = v;
	cast_ok = TRUE;
	break;
      }
    }
    if (!cast_ok) {
      prop->enum_data = enumdata[0].enumv;
      dia_context_add_message (ctx, _("Property cast from int to enum out of range"));
    }
  }
}

static void
enumprop_save(EnumProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  data_add_enum(attr, prop->enum_data, ctx);
}

static void
enumprop_get_from_offset(EnumProperty *prop,
                         void *base, guint offset, guint offset2)
{
  if (offset2 == 0) {
    prop->enum_data = struct_member(base,offset,gint);
  } else {
    void *base2 = struct_member(base,offset,void*);
    g_return_if_fail (base2 != NULL);
    prop->enum_data = struct_member(base2,offset2,gint);
  }
}

static void
enumprop_set_from_offset(EnumProperty *prop,
                         void *base, guint offset, guint offset2)
{
  if (offset2 == 0) {
    struct_member(base,offset,gint) = prop->enum_data;
  } else {
    void *base2 = struct_member(base,offset,void*);
    g_return_if_fail (base2 != NULL);
    struct_member(base2,offset2,gint) = prop->enum_data;
    g_return_if_fail (offset2 == offsetof(Text, alignment));
    text_set_alignment (base2, prop->enum_data);
  }
}

static const PropertyOps enumprop_ops = {
  (PropertyType_New) enumprop_new,
  (PropertyType_Free) noopprop_free,
  (PropertyType_Copy) enumprop_copy,
  (PropertyType_Load) enumprop_load,
  (PropertyType_Save) enumprop_save,
  (PropertyType_GetWidget) enumprop_get_widget,
  (PropertyType_ResetWidget) enumprop_reset_widget,
  (PropertyType_SetFromWidget) enumprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) enumprop_get_from_offset,
  (PropertyType_SetFromOffset) enumprop_set_from_offset
};

/********************************/
/* The ENUMARRAY property type.  */
/********************************/

static EnumarrayProperty *
enumarrayprop_new(const PropDescription *pdesc,
                  PropDescToPropPredicate reason)
{
  EnumarrayProperty *prop = g_new0(EnumarrayProperty,1);
  initialize_property(&prop->common, pdesc, reason);
  prop->enumarray_data = g_array_new(FALSE,TRUE,sizeof(gint));
  return prop;
}

static void
enumarrayprop_free(EnumarrayProperty *prop)
{
  g_array_free(prop->enumarray_data,TRUE);
  g_clear_pointer (&prop, g_free);
}

static EnumarrayProperty *
enumarrayprop_copy(EnumarrayProperty *src)
{
  guint i;
  EnumarrayProperty *prop =
    (EnumarrayProperty *)src->common.ops->new_prop(src->common.descr,
                                                    src->common.reason);
  copy_init_property(&prop->common,&src->common);
  g_array_set_size(prop->enumarray_data,src->enumarray_data->len);
  for (i = 0 ; i < src->enumarray_data->len; i++)
    g_array_index(prop->enumarray_data,gint,i) =
      g_array_index(src->enumarray_data,gint,i);
  return prop;
}

static void
enumarrayprop_load(EnumarrayProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
{
  guint nvals = attribute_num_data(attr);
  guint i;
  g_array_set_size(prop->enumarray_data,nvals);

  for (i=0; (i < nvals) && data; i++, data = data_next(data))
    g_array_index(prop->enumarray_data,gint,i) = data_enum(data, ctx);
  if (i != nvals)
    g_warning("attribute_num_data() and actual data count mismatch "
              "(shouldn't happen)");
}

static void
enumarrayprop_save(EnumarrayProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  guint i;
  for (i = 0; i < prop->enumarray_data->len; i++)
    data_add_enum(attr, g_array_index(prop->enumarray_data,gint,i), ctx);
}


static void
enumarrayprop_get_from_offset (EnumarrayProperty *prop,
                               void              *base,
                               guint              offset,
                               guint              offset2)
{
  guint nvals = struct_member (base, offset2, guint);
  guint i;
  void *ofs_val = struct_member (base, offset, void *);
  g_array_set_size (prop->enumarray_data, nvals);
  for (i = 0; i < nvals; i++) {
    g_array_index (prop->enumarray_data, int, i) =
      struct_member(ofs_val, i * sizeof (int), int);
  }
}


static void
enumarrayprop_set_from_offset (EnumarrayProperty *prop,
                               void              *base,
                               guint              offset,
                               guint              offset2)
{
  guint nvals = prop->enumarray_data->len;
  int *vals = g_memdup2 (&g_array_index (prop->enumarray_data, int, 0),
                         sizeof (int) * nvals);
  g_clear_pointer (&struct_member (base, offset, int *), g_free);
  struct_member (base, offset, int *) = vals;
  struct_member (base, offset2, guint) = nvals;
}


static const PropertyOps enumarrayprop_ops = {
  (PropertyType_New) enumarrayprop_new,
  (PropertyType_Free) enumarrayprop_free,
  (PropertyType_Copy) enumarrayprop_copy,
  (PropertyType_Load) enumarrayprop_load,
  (PropertyType_Save) enumarrayprop_save,
  (PropertyType_GetWidget) noopprop_get_widget,
  (PropertyType_ResetWidget) noopprop_reset_widget,
  (PropertyType_SetFromWidget) noopprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) enumarrayprop_get_from_offset,
  (PropertyType_SetFromOffset) enumarrayprop_set_from_offset
};

/* ************************************************************** */

void
prop_inttypes_register(void)
{
  prop_type_register(PROP_TYPE_CHAR,&charprop_ops);
  prop_type_register(PROP_TYPE_BOOL,&boolprop_ops);
  prop_type_register(PROP_TYPE_INT,&intprop_ops);
  prop_type_register(PROP_TYPE_INTARRAY,&intarrayprop_ops);
  prop_type_register(PROP_TYPE_ENUM,&enumprop_ops);
  prop_type_register(PROP_TYPE_ENUMARRAY,&enumarrayprop_ops);
}
