/* Dia -- a diagram creation/manipulation program -*- c -*-
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 * Major restructuration done in August 2001 by C. Chepelov
 *
 * Property types for "geometric" types (real, points, rectangles, etc.)
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
#include "propinternals.h"
#include "geometry.h"
#include "connpoint_line.h"
#include "prop_geomtypes.h"
#include "prefs.h"
#include "dia-unit-spinner.h"

static void fontsizeprop_reset_widget(FontsizeProperty *prop, WIDGET *widget);

/****************************/
/* The REAL property type.  */
/****************************/

static RealProperty *
realprop_new(const PropDescription *pdesc, PropDescToPropPredicate reason)
{
  RealProperty *prop = g_new0(RealProperty,1);
  initialize_property(&prop->common, pdesc, reason);
  prop->real_data = 0.0;
  return prop;
}

static RealProperty *
realprop_copy(RealProperty *src)
{
  RealProperty *prop =
    (RealProperty *)src->common.ops->new_prop(src->common.descr,
                                               src->common.reason);
  copy_init_property(&prop->common,&src->common);
  prop->real_data = src->real_data;
  return prop;
}

static WIDGET *
realprop_get_widget(RealProperty *prop, PropDialog *dialog)
{
  GtkAdjustment *adj = GTK_ADJUSTMENT(gtk_adjustment_new(prop->real_data,
                                                         G_MINFLOAT,
                                                         G_MAXFLOAT,
                                                         0.1, 1.0, 0));
  GtkWidget *ret = gtk_spin_button_new(adj, 1.0, 2);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ret),TRUE);
  prophandler_connect(&prop->common, G_OBJECT(ret), "value_changed");

  return ret;
}

static void
realprop_reset_widget(RealProperty *prop, WIDGET *widget)
{
  GtkAdjustment *adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON(widget));
  if (prop->common.descr->extra_data) {
    PropNumData *numdata = prop->common.descr->extra_data;
    gtk_adjustment_configure (adj, prop->real_data,
			      numdata->min, numdata->max,
			      numdata->step, 10.0 * numdata->step, 0);
  } else {
    gtk_adjustment_configure (adj, prop->real_data,
			      G_MINFLOAT, G_MAXFLOAT,
			      0.1, 1.0, 0);
  }
}

static void
realprop_set_from_widget(RealProperty *prop, WIDGET *widget)
{
  prop->real_data = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
}

static void
realprop_load(RealProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
{
  prop->real_data = data_real(data, ctx);
}

static void
realprop_save(RealProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  data_add_real(attr, prop->real_data, ctx);
}

static void
realprop_get_from_offset(RealProperty *prop,
                         void *base, guint offset, guint offset2)
{
  prop->real_data = struct_member(base,offset,real);
}

static void
realprop_set_from_offset(RealProperty *prop,
                         void *base, guint offset, guint offset2)
{
  struct_member(base,offset,real) = prop->real_data;
}

static int
realprop_get_data_size(void)
{
  RealProperty prop;
  return sizeof (prop.real_data);
}

static const PropertyOps realprop_ops = {
  (PropertyType_New) realprop_new,
  (PropertyType_Free) noopprop_free,
  (PropertyType_Copy) realprop_copy,
  (PropertyType_Load) realprop_load,
  (PropertyType_Save) realprop_save,
  (PropertyType_GetWidget) realprop_get_widget,
  (PropertyType_ResetWidget) realprop_reset_widget,
  (PropertyType_SetFromWidget) realprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) realprop_get_from_offset,
  (PropertyType_SetFromOffset) realprop_set_from_offset,
  (PropertyType_GetDataSize) realprop_get_data_size
};

/****************************/
/* The LENGTH property type.  */
/****************************/

static LengthProperty *
lengthprop_new(const PropDescription *pdesc, PropDescToPropPredicate reason)
{
  LengthProperty *prop = g_new0(LengthProperty,1);
  initialize_property(&prop->common, pdesc, reason);
  prop->length_data = 0.0;
  return prop;
}

static LengthProperty *
lengthprop_copy(LengthProperty *src)
{
  LengthProperty *prop =
    (LengthProperty *)src->common.ops->new_prop(src->common.descr,
                                               src->common.reason);
  copy_init_property(&prop->common,&src->common);
  prop->length_data = src->length_data;
  return prop;
}

static WIDGET *
lengthprop_get_widget(LengthProperty *prop, PropDialog *dialog)
{
  GtkAdjustment *adj = GTK_ADJUSTMENT(gtk_adjustment_new(prop->length_data,
                                                         G_MINFLOAT,
                                                         G_MAXFLOAT,
                                                         0.1, 1.0, 0));
  GtkWidget *ret = dia_unit_spinner_new(adj, prefs_get_length_unit());
  /*  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ret),TRUE);*/
  prophandler_connect(&prop->common, G_OBJECT(ret), "value-changed");

  return ret;
}

static void
lengthprop_reset_widget(LengthProperty *prop, WIDGET *widget)
{
  dia_unit_spinner_set_value(DIA_UNIT_SPINNER(widget), prop->length_data);
}

static void
lengthprop_set_from_widget(LengthProperty *prop, WIDGET *widget)
{
  prop->length_data = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(widget));
}

static void
lengthprop_load(LengthProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
{
  prop->length_data = data_real(data, ctx);
}

static void
lengthprop_save(LengthProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  data_add_real(attr, prop->length_data, ctx);
}

static void
lengthprop_get_from_offset(LengthProperty *prop,
                         void *base, guint offset, guint offset2)
{
  prop->length_data = struct_member(base,offset,real);
}

static void
lengthprop_set_from_offset(LengthProperty *prop,
                         void *base, guint offset, guint offset2)
{
  struct_member(base,offset,real) = prop->length_data;
}

static int
lengthprop_get_data_size(void)
{
  LengthProperty prop;
  return sizeof (prop.length_data);
}

static const PropertyOps lengthprop_ops = {
  (PropertyType_New) lengthprop_new,
  (PropertyType_Free) noopprop_free,
  (PropertyType_Copy) lengthprop_copy,
  (PropertyType_Load) lengthprop_load,
  (PropertyType_Save) lengthprop_save,
  (PropertyType_GetWidget) lengthprop_get_widget,
  (PropertyType_ResetWidget) lengthprop_reset_widget,
  (PropertyType_SetFromWidget) lengthprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) lengthprop_get_from_offset,
  (PropertyType_SetFromOffset) lengthprop_set_from_offset,
  (PropertyType_GetDataSize) lengthprop_get_data_size
};

/****************************/
/* The FONTSIZE property type.  */
/****************************/

static FontsizeProperty *
fontsizeprop_new(const PropDescription *pdesc, PropDescToPropPredicate reason)
{
  FontsizeProperty *prop = g_new0(FontsizeProperty,1);
  initialize_property(&prop->common, pdesc, reason);
  prop->fontsize_data = 0.0;
  return prop;
}

static FontsizeProperty *
fontsizeprop_copy(FontsizeProperty *src)
{
  FontsizeProperty *prop =
    (FontsizeProperty *)src->common.ops->new_prop(src->common.descr,
                                               src->common.reason);
  copy_init_property(&prop->common,&src->common);
  prop->fontsize_data = src->fontsize_data;
  return prop;
}

static WIDGET *
fontsizeprop_get_widget(FontsizeProperty *prop, PropDialog *dialog)
{
  GtkAdjustment *adj = GTK_ADJUSTMENT(gtk_adjustment_new(prop->fontsize_data,
                                                         G_MINFLOAT,
                                                         G_MAXFLOAT,
                                                         0.1, 1.0, 0));
  GtkWidget *ret = dia_unit_spinner_new(adj, prefs_get_fontsize_unit());
  prophandler_connect(&prop->common, G_OBJECT(ret), "value-changed");
  return ret;
}

static void
fontsizeprop_reset_widget(FontsizeProperty *prop, WIDGET *widget)
{
  dia_unit_spinner_set_value(DIA_UNIT_SPINNER(widget), prop->fontsize_data);
}

static void
fontsizeprop_set_from_widget(FontsizeProperty *prop, WIDGET *widget)
{
  prop->fontsize_data = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(widget));
}

static void
fontsizeprop_load(FontsizeProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
{
  PropNumData *numdata = prop->common.descr->extra_data;
  real value = data_real(data, ctx);

  if (numdata) {
    if (value < numdata->min)
      value = numdata->min;
    else if (value > numdata->max)
      value = numdata->max;
  }
  prop->fontsize_data = value;
}

static void
fontsizeprop_save(FontsizeProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  data_add_real(attr, prop->fontsize_data, ctx);
}

static void
fontsizeprop_get_from_offset(FontsizeProperty *prop,
                         void *base, guint offset, guint offset2)
{
  if (offset2 == 0) {
    prop->fontsize_data = struct_member(base,offset,real);
  } else {
    void *base2 = struct_member(base,offset,void*);
    prop->fontsize_data = struct_member(base2,offset2,real);
  }
}

static void
fontsizeprop_set_from_offset(FontsizeProperty *prop,
                         void *base, guint offset, guint offset2)
{
  PropNumData *numdata = prop->common.descr->extra_data;
  real value = prop->fontsize_data;

  if (numdata) {
    if (value < numdata->min)
      value = numdata->min;
    else if (value > numdata->max)
      value = numdata->max;
  }
  if (offset2 == 0) {
    struct_member(base,offset,real) = value;
  } else {
    void *base2 = struct_member(base,offset,void*);
    struct_member(base2,offset2,real) = value;
    g_return_if_fail (offset2 == offsetof(Text, height));
    text_set_height ((Text*)base2, value);
  }
}

static int
fontsizeprop_get_data_size(void)
{
  FontsizeProperty prop;
  return sizeof (prop.fontsize_data);
}

static const PropertyOps fontsizeprop_ops = {
  (PropertyType_New) fontsizeprop_new,
  (PropertyType_Free) noopprop_free,
  (PropertyType_Copy) fontsizeprop_copy,
  (PropertyType_Load) fontsizeprop_load,
  (PropertyType_Save) fontsizeprop_save,
  (PropertyType_GetWidget) fontsizeprop_get_widget,
  (PropertyType_ResetWidget) fontsizeprop_reset_widget,
  (PropertyType_SetFromWidget) fontsizeprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) fontsizeprop_get_from_offset,
  (PropertyType_SetFromOffset) fontsizeprop_set_from_offset,
  (PropertyType_GetDataSize) fontsizeprop_get_data_size
};

/*****************************/
/* The POINT property type.  */
/*****************************/

static PointProperty *
pointprop_new(const PropDescription *pdesc, PropDescToPropPredicate reason)
{
  PointProperty *prop = g_new0(PointProperty,1);
  initialize_property(&prop->common, pdesc, reason);
  prop->point_data.x = 0.0;
  prop->point_data.y = 0.0;
  return prop;
}

static PointProperty *
pointprop_copy(PointProperty *src)
{
  PointProperty *prop =
    (PointProperty *)src->common.ops->new_prop(src->common.descr,
                                                src->common.reason);
  copy_init_property(&prop->common,&src->common);
  prop->point_data = src->point_data;
  return prop;
}

static void
pointprop_load(PointProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
{
  data_point(data,&prop->point_data, ctx);
}

static void
pointprop_save(PointProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  data_add_point(attr, &prop->point_data, ctx);
}

static void
pointprop_get_from_offset(PointProperty *prop,
                          void *base, guint offset, guint offset2)
{
  prop->point_data = struct_member(base,offset,Point);
}

static void
pointprop_set_from_offset(PointProperty *prop,
                          void *base, guint offset, guint offset2)
{
  struct_member(base,offset,Point) = prop->point_data;
}

static const PropertyOps pointprop_ops = {
  (PropertyType_New) pointprop_new,
  (PropertyType_Free) noopprop_free,
  (PropertyType_Copy) pointprop_copy,
  (PropertyType_Load) pointprop_load,
  (PropertyType_Save) pointprop_save,
  (PropertyType_GetWidget) invalidprop_get_widget,
  (PropertyType_ResetWidget) invalidprop_reset_widget,
  (PropertyType_SetFromWidget) invalidprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) pointprop_get_from_offset,
  (PropertyType_SetFromOffset) pointprop_set_from_offset
};

/********************************/
/* The POINTARRAY property type.  */
/********************************/

static PointarrayProperty *
pointarrayprop_new(const PropDescription *pdesc,
                   PropDescToPropPredicate reason)
{
  PointarrayProperty *prop = g_new0(PointarrayProperty,1);
  initialize_property(&prop->common,pdesc,reason);
  prop->pointarray_data = g_array_new(FALSE,TRUE,sizeof(Point));
  return prop;
}


static void
pointarrayprop_free (PointarrayProperty *prop)
{
  g_array_free (prop->pointarray_data,TRUE);
  g_clear_pointer (&prop, g_free);
}


static PointarrayProperty *
pointarrayprop_copy(PointarrayProperty *src)
{
  guint i;
  PointarrayProperty *prop =
    (PointarrayProperty *)src->common.ops->new_prop(src->common.descr,
                                                     src->common.reason);
  copy_init_property(&prop->common,&src->common);
  g_array_set_size(prop->pointarray_data,src->pointarray_data->len);
  for (i = 0 ; i < src->pointarray_data->len; i++)
    g_array_index(prop->pointarray_data,Point,i) =
      g_array_index(src->pointarray_data,Point,i);
  return prop;
}

static void
pointarrayprop_load(PointarrayProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
{
  guint nvals = attribute_num_data(attr);
  guint i;
  g_array_set_size(prop->pointarray_data,nvals);
  for (i=0; (i < nvals) && data; i++, data = data_next(data))
    data_point(data,&g_array_index(prop->pointarray_data,Point,i),ctx);
  if (i != nvals)
    g_warning("attribute_num_data() and actual data count mismatch "
              "(shouldn't happen)");
}

static void
pointarrayprop_save(PointarrayProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  guint i;
  for (i = 0; i < prop->pointarray_data->len; i++)
    data_add_point(attr, &g_array_index(prop->pointarray_data,Point,i), ctx);
}

static void
pointarrayprop_get_from_offset(PointarrayProperty *prop,
                               void *base, guint offset, guint offset2)
{
  guint nvals = struct_member(base,offset2,guint);
  guint i;
  void *ofs_val = struct_member(base,offset,void *);
  g_array_set_size(prop->pointarray_data,nvals);
  for (i = 0; i < nvals; i++)
    g_array_index(prop->pointarray_data,Point,i) =
      struct_member(ofs_val,i * sizeof(Point),Point);
}


static void
pointarrayprop_set_from_offset (PointarrayProperty *prop,
                                void               *base,
                                guint               offset,
                                guint               offset2)
{
  guint nvals = prop->pointarray_data->len;
  Point *vals = g_memdup2 (&g_array_index (prop->pointarray_data, Point, 0),
                           sizeof(Point) * nvals);
  g_clear_pointer (&struct_member (base, offset, Point *), g_free);
  struct_member (base, offset, Point *) = vals;
  struct_member (base, offset2, guint) = nvals;
}


static const PropertyOps pointarrayprop_ops = {
  (PropertyType_New) pointarrayprop_new,
  (PropertyType_Free) pointarrayprop_free,
  (PropertyType_Copy) pointarrayprop_copy,
  (PropertyType_Load) pointarrayprop_load,
  (PropertyType_Save) pointarrayprop_save,
  (PropertyType_GetWidget) invalidprop_get_widget,
  (PropertyType_ResetWidget) invalidprop_reset_widget,
  (PropertyType_SetFromWidget) invalidprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) pointarrayprop_get_from_offset,
  (PropertyType_SetFromOffset) pointarrayprop_set_from_offset
};


/********************************/
/* The BEZPOINT property type.  */
/********************************/

static BezPointProperty *
bezpointprop_new(const PropDescription *pdesc,
                 PropDescToPropPredicate reason)
{
  BezPointProperty *prop = g_new0(BezPointProperty,1);
  initialize_property(&prop->common,pdesc,reason);
  memset(&prop->bezpoint_data,0,sizeof(prop->bezpoint_data));
  return prop;
}

static BezPointProperty *
bezpointprop_copy(BezPointProperty *src)
{
  BezPointProperty *prop =
    (BezPointProperty *)src->common.ops->new_prop(src->common.descr,
                                                   src->common.reason);
  copy_init_property(&prop->common,&src->common);
  prop->bezpoint_data = src->bezpoint_data;
  return prop;
}

static void
bezpointprop_load(BezPointProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
{
  data_bezpoint(data,&prop->bezpoint_data,ctx);
}

static void
bezpointprop_save(BezPointProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  data_add_bezpoint(attr, &prop->bezpoint_data, ctx);
}

static void
bezpointprop_get_from_offset(BezPointProperty *prop,
                             void *base, guint offset, guint offset2)
{
  prop->bezpoint_data = struct_member(base,offset,BezPoint);
}

static void
bezpointprop_set_from_offset(BezPointProperty *prop,
                             void *base, guint offset, guint offset2)
{
  struct_member(base,offset,BezPoint) = prop->bezpoint_data;
}

static const PropertyOps bezpointprop_ops = {
  (PropertyType_New) bezpointprop_new,
  (PropertyType_Free) noopprop_free,
  (PropertyType_Copy) bezpointprop_copy,
  (PropertyType_Load) bezpointprop_load,
  (PropertyType_Save) bezpointprop_save,
  (PropertyType_GetWidget) invalidprop_get_widget,
  (PropertyType_ResetWidget) invalidprop_reset_widget,
  (PropertyType_SetFromWidget) invalidprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) bezpointprop_get_from_offset,
  (PropertyType_SetFromOffset) bezpointprop_set_from_offset
};

/*************************************/
/* The BEZPOINTARRAY property type.  */
/*************************************/

static BezPointarrayProperty *
bezpointarrayprop_new(const PropDescription *pdesc,
                      PropDescToPropPredicate reason)
{
  BezPointarrayProperty *prop = g_new0(BezPointarrayProperty,1);
  initialize_property(&prop->common,pdesc,reason);
  prop->bezpointarray_data = g_array_new(FALSE,TRUE,sizeof(BezPoint));
  return prop;
}

static void
bezpointarrayprop_free(BezPointarrayProperty *prop)
{
  g_array_free(prop->bezpointarray_data,TRUE);
  g_clear_pointer (&prop, g_free);
}

static BezPointarrayProperty *
bezpointarrayprop_copy(BezPointarrayProperty *src)
{
  guint i;
  BezPointarrayProperty *prop =
    (BezPointarrayProperty *)src->common.ops->new_prop(src->common.descr,
                                                        src->common.reason);
  copy_init_property(&prop->common,&src->common);
  g_array_set_size(prop->bezpointarray_data,src->bezpointarray_data->len);
  for (i = 0 ; i < src->bezpointarray_data->len; i++)
    g_array_index(prop->bezpointarray_data,BezPoint,i) =
      g_array_index(src->bezpointarray_data,BezPoint,i);
  return prop;
}

static void
bezpointarrayprop_load(BezPointarrayProperty *prop,
                       AttributeNode attr, DataNode data,
		       DiaContext *ctx)
{
  guint nvals = attribute_num_data(attr);
  guint i;

  g_array_set_size(prop->bezpointarray_data,nvals);

  for (i=0; (i < nvals) && data; i++, data = data_next(data))
    data_bezpoint(data,&g_array_index(prop->bezpointarray_data,BezPoint,i),ctx);
  if (i != nvals)
    g_warning("attribute_num_data() and actual data count mismatch "
              "(shouldn't happen)");
}

static void
bezpointarrayprop_save(BezPointarrayProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  guint i;
  for (i = 0; i < prop->bezpointarray_data->len; i++)
    data_add_bezpoint(attr,
                      &g_array_index(prop->bezpointarray_data,BezPoint,i), ctx);
}


static void
bezpointarrayprop_get_from_offset (BezPointarrayProperty *prop,
                                   void                  *base,
                                   guint                  offset,
                                   guint                  offset2)
{
  guint nvals = struct_member (base, offset2, guint);
  guint i;
  void *ofs_val = struct_member (base, offset, void *);
  g_array_set_size (prop->bezpointarray_data, nvals);
  for (i = 0; i < nvals; i++) {
    g_array_index (prop->bezpointarray_data, BezPoint, i) =
      struct_member (ofs_val, i * sizeof (BezPoint), BezPoint);
  }
}


static void
bezpointarrayprop_set_from_offset(BezPointarrayProperty *prop,
                                  void                  *base,
                                  guint                  offset,
                                  guint                  offset2)
{
  guint nvals = prop->bezpointarray_data->len;
  BezPoint *vals = g_memdup2 (&g_array_index (prop->bezpointarray_data,
                                              BezPoint, 0),
                              sizeof (BezPoint) * nvals);
  g_clear_pointer (&struct_member (base, offset, int *), g_free);
  struct_member (base, offset, BezPoint *) = vals;
  struct_member (base, offset2, guint) = nvals;
}


static const PropertyOps bezpointarrayprop_ops = {
  (PropertyType_New) bezpointarrayprop_new,
  (PropertyType_Free) bezpointarrayprop_free,
  (PropertyType_Copy) bezpointarrayprop_copy,
  (PropertyType_Load) bezpointarrayprop_load,
  (PropertyType_Save) bezpointarrayprop_save,
  (PropertyType_GetWidget) invalidprop_get_widget,
  (PropertyType_ResetWidget) invalidprop_reset_widget,
  (PropertyType_SetFromWidget) invalidprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) bezpointarrayprop_get_from_offset,
  (PropertyType_SetFromOffset) bezpointarrayprop_set_from_offset
};

/*****************************/
/* The RECT property type.  */
/*****************************/

static RectProperty *
rectprop_new(const PropDescription *pdesc, PropDescToPropPredicate reason)
{
  RectProperty *prop = g_new0(RectProperty,1);
  initialize_property(&prop->common,pdesc,reason);
  memset(&prop->rect_data,0,sizeof(prop->rect_data));
  return prop;
}

static RectProperty *
rectprop_copy(RectProperty *src)
{
  RectProperty *prop =
    (RectProperty *)src->common.ops->new_prop(src->common.descr,
                                               src->common.reason);
  copy_init_property(&prop->common,&src->common);
  prop->rect_data = src->rect_data;
  return prop;
}

static void
rectprop_load(RectProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
{
  data_rectangle(data,&prop->rect_data,ctx);
}

static void
rectprop_save(RectProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  data_add_rectangle(attr, &prop->rect_data, ctx);
}

static void
rectprop_get_from_offset(RectProperty *prop,
                         void *base, guint offset, guint offset2)
{
  prop->rect_data = struct_member (base, offset, DiaRectangle);
}

static void
rectprop_set_from_offset(RectProperty *prop,
                         void *base, guint offset, guint offset2)
{
  struct_member (base, offset, DiaRectangle) = prop->rect_data;
}

static const PropertyOps rectprop_ops = {
  (PropertyType_New) rectprop_new,
  (PropertyType_Free) noopprop_free,
  (PropertyType_Copy) rectprop_copy,
  (PropertyType_Load) rectprop_load,
  (PropertyType_Save) rectprop_save,
  (PropertyType_GetWidget) invalidprop_get_widget,
  (PropertyType_ResetWidget) invalidprop_reset_widget,
  (PropertyType_SetFromWidget) invalidprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) rectprop_get_from_offset,
  (PropertyType_SetFromOffset) rectprop_set_from_offset
};

/*********************************/
/* The ENDPOINTS property type.  */
/*********************************/

static EndpointsProperty *
endpointsprop_new(const PropDescription *pdesc,
                  PropDescToPropPredicate reason)
{
  EndpointsProperty *prop = g_new0(EndpointsProperty,1);
  initialize_property(&prop->common,pdesc,reason);
  memset(&prop->endpoints_data,0,sizeof(prop->endpoints_data));
  return prop;
}

static EndpointsProperty *
endpointsprop_copy(EndpointsProperty *src)
{
  EndpointsProperty *prop =
    (EndpointsProperty *)src->common.ops->new_prop(src->common.descr,
                                                    src->common.reason);
  copy_init_property(&prop->common,&src->common);
  memcpy(&prop->endpoints_data,
         &src->endpoints_data,
         sizeof(prop->endpoints_data));
  return prop;
}

static void
endpointsprop_load(EndpointsProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
{
  data_point(data,&prop->endpoints_data[0], ctx);
  data = data_next(data);
  data_point(data,&prop->endpoints_data[1], ctx);
}

static void
endpointsprop_save(EndpointsProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  data_add_point(attr, &prop->endpoints_data[0], ctx);
  data_add_point(attr, &prop->endpoints_data[1], ctx);
}

static void
endpointsprop_get_from_offset(EndpointsProperty *prop,
                              void *base, guint offset, guint offset2)
{
  memcpy(&prop->endpoints_data,
         &struct_member(base,offset,Point),
         sizeof(prop->endpoints_data));
}

static void
endpointsprop_set_from_offset(EndpointsProperty *prop,
                              void *base, guint offset, guint offset2)
{
  memcpy(&struct_member(base,offset,Point),
         &prop->endpoints_data,
         sizeof(prop->endpoints_data));
}

static const PropertyOps endpointsprop_ops = {
  (PropertyType_New) endpointsprop_new,
  (PropertyType_Free) noopprop_free,
  (PropertyType_Copy) endpointsprop_copy,
  (PropertyType_Load) endpointsprop_load,
  (PropertyType_Save) endpointsprop_save,
  (PropertyType_GetWidget) invalidprop_get_widget,
  (PropertyType_ResetWidget) invalidprop_reset_widget,
  (PropertyType_SetFromWidget) invalidprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) endpointsprop_get_from_offset,
  (PropertyType_SetFromOffset) endpointsprop_set_from_offset
};

/***************************/
/* The CONNPOINT_LINE property type.  */
/***************************/

static Connpoint_LineProperty *
connpoint_lineprop_new(const PropDescription *pdesc,
                       PropDescToPropPredicate reason)
{
  Connpoint_LineProperty *prop = g_new0(Connpoint_LineProperty,1);
  initialize_property(&prop->common,pdesc,reason);
  prop->connpoint_line_data = 0;
  return prop;
}

static Connpoint_LineProperty *
connpoint_lineprop_copy(Connpoint_LineProperty *src)
{
  Connpoint_LineProperty *prop =
    (Connpoint_LineProperty *)src->common.ops->new_prop(src->common.descr,
                                                         src->common.reason);
  copy_init_property(&prop->common,&src->common);
  prop->connpoint_line_data = src->connpoint_line_data;
  return prop;
}

static void
connpoint_lineprop_load(Connpoint_LineProperty *prop, AttributeNode attr,
			DataNode data, DiaContext *ctx)
{
  prop->connpoint_line_data = data_int(data,ctx);
}

static void
connpoint_lineprop_save(Connpoint_LineProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  data_add_int(attr, prop->connpoint_line_data, ctx);
}

static void
connpoint_lineprop_get_from_offset(Connpoint_LineProperty *prop,
                                   void *base, guint offset, guint offset2)
{
  prop->connpoint_line_data =
    struct_member(base,offset,ConnPointLine *)->num_connections;
}

static void
connpoint_lineprop_set_from_offset(Connpoint_LineProperty *prop,
                                   void *base, guint offset, guint offset2)
{
  connpointline_adjust_count(struct_member(base,offset,ConnPointLine *),
                             prop->connpoint_line_data,
                             &struct_member(base,offset,ConnPointLine *)->end);
}

static const PropertyOps connpoint_lineprop_ops = {
  (PropertyType_New) connpoint_lineprop_new,
  (PropertyType_Free) noopprop_free,
  (PropertyType_Copy) connpoint_lineprop_copy,
  (PropertyType_Load) connpoint_lineprop_load,
  (PropertyType_Save) connpoint_lineprop_save,
  (PropertyType_GetWidget) invalidprop_get_widget,
  (PropertyType_ResetWidget) invalidprop_reset_widget,
  (PropertyType_SetFromWidget) invalidprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) connpoint_lineprop_get_from_offset,
  (PropertyType_SetFromOffset) connpoint_lineprop_set_from_offset
};


/* ************************************************************** */

void
prop_geomtypes_register(void)
{
  prop_type_register(PROP_TYPE_REAL,&realprop_ops);
  prop_type_register(PROP_TYPE_LENGTH,&lengthprop_ops);
  prop_type_register(PROP_TYPE_FONTSIZE,&fontsizeprop_ops);
  prop_type_register(PROP_TYPE_POINT,&pointprop_ops);
  prop_type_register(PROP_TYPE_POINTARRAY,&pointarrayprop_ops);
  prop_type_register(PROP_TYPE_BEZPOINT,&bezpointprop_ops);
  prop_type_register(PROP_TYPE_BEZPOINTARRAY,&bezpointarrayprop_ops);
  prop_type_register(PROP_TYPE_RECT,&rectprop_ops);
  prop_type_register(PROP_TYPE_ENDPOINTS,&endpointsprop_ops);
  prop_type_register(PROP_TYPE_CONNPOINT_LINE,&connpoint_lineprop_ops);
}
