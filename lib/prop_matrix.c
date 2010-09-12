/* Dia -- a diagram creation/manipulation program -*- c -*-
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 *
 * Copyright (C) 2010 Hans Breuer
 * Property type for affine transformation.
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
#include <config.h>

#include <math.h>
#include <gtk/gtk.h>
#define WIDGET GtkWidget
#include "widgets.h"
#include "properties.h"
#include "propinternals.h"
#include "message.h"

static MatrixProperty *
matrixprop_new(const PropDescription *pdesc, PropDescToPropPredicate reason)
{
  MatrixProperty *prop = g_new0(MatrixProperty,1);

  initialize_property(&prop->common, pdesc, reason);
  /* empty by default (=identity) */
  prop->matrix = NULL;

  return prop;
}

static void
matrixprop_free(MatrixProperty *prop) 
{
  g_free (prop->matrix);
  g_free(prop);
} 

static MatrixProperty *
matrixprop_copy(MatrixProperty *src) 
{
  MatrixProperty *prop = 
    (MatrixProperty *)src->common.ops->new_prop(src->common.descr,
                                              src->common.reason);

  prop->matrix = g_memdup (src->matrix, sizeof(DiaMatrix));
					      
  return prop;
}

static real
_matrix_xml_get_value (DataNode data, const char* name, real defval)
{
  xmlChar *val;
  real retval;

  val = xmlGetProp(data, (const xmlChar *)name);
  if (!val)
    return defval;
  retval = g_ascii_strtod((char *)val, NULL);
  xmlFree(val);
  return retval;
}
DiaMatrix *
data_matrix (DataNode data)
{
  DiaMatrix *matrix;
  gchar *str;

  matrix = g_new (DiaMatrix, 1);
  matrix->xx = _matrix_xml_get_value (data, "xx", 1.0);
  matrix->xy = _matrix_xml_get_value (data, "xy", 0.0);
  matrix->yx = _matrix_xml_get_value (data, "yx", 0.0);
  matrix->yy = _matrix_xml_get_value (data, "yy", 1.0);
  matrix->x0 = _matrix_xml_get_value (data, "x0", 0.0);
  matrix->y0 = _matrix_xml_get_value (data, "y0", 0.0);

  /* identity? */
  if (matrix->xx == 1.0 && matrix->yx == 0.0 &&
      matrix->xy == 0.0 && matrix->yy == 1.0 &&
      matrix->x0 == 0.0 && matrix->y0 == 0.0) {
    g_free (matrix);
    return NULL;
  }
  /* TODO: check it's invertible? */

  return matrix;
}

static void 
matrixprop_load(MatrixProperty *prop, AttributeNode attr, DataNode data)
{
  prop->matrix = data_matrix (data);
}

static void
_matrix_xml_add_val (DataNode data_node, const char* name, real val)
{
  gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

  g_ascii_formatd(buf, sizeof(buf), "%g", val);
  xmlSetProp(data_node, (const xmlChar *)name, (xmlChar *)buf);
}
void
data_add_matrix (AttributeNode attr, DiaMatrix *matrix)
{
  DataNode data_node;
  
  data_node = xmlNewChild(attr, NULL, (const xmlChar *)"matrix", NULL);

  if (matrix) {
    _matrix_xml_add_val (data_node, "xx", matrix->xx);
    _matrix_xml_add_val (data_node, "xy", matrix->xy);
    _matrix_xml_add_val (data_node, "yx", matrix->yx);
    _matrix_xml_add_val (data_node, "yy", matrix->yy);
    _matrix_xml_add_val (data_node, "x0", matrix->x0);
    _matrix_xml_add_val (data_node, "y0", matrix->y0);
  }
}

static void 
matrixprop_save(MatrixProperty *prop, AttributeNode attr) 
{
  if (prop->matrix) {
    data_add_matrix (attr, prop->matrix);
  }
}

static void 
matrixprop_get_from_offset(MatrixProperty *prop,
                         void *base, guint offset, guint offset2) 
{
  DiaMatrix *matrix = struct_member(base,offset,DiaMatrix *);

  prop->matrix = g_memdup (matrix, sizeof (DiaMatrix));
}

static void 
matrixprop_set_from_offset(MatrixProperty *prop,
                           void *base, guint offset, guint offset2)
{
  DiaMatrix *dest = struct_member(base,offset,DiaMatrix *);
  if (dest)
    g_free (dest);

  struct_member(base,offset, DiaMatrix *) = g_memdup (prop->matrix, sizeof (DiaMatrix));
}

/* GUI stuff - just the angle for now
 */
static GtkWidget *
matrixprop_get_widget (MatrixProperty *prop, PropDialog *dialog) 
{ 
  GtkObject *adj;
  GtkWidget *ret;

  adj = gtk_adjustment_new(0.0, -180.0, 180.0, 1.0, 15.0, 0);
  ret = gtk_spin_button_new(GTK_ADJUSTMENT (adj), 1.0, 2);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ret),TRUE);
  prophandler_connect(&prop->common, G_OBJECT(ret), "value_changed");
  
  return ret;
}

static void 
matrixprop_reset_widget(MatrixProperty *prop, GtkWidget *widget)
{
  GtkObject *adj;
  real angle;

  if (!prop->matrix)
    angle = 0;
  else
    angle = atan2 (prop->matrix->xy, prop->matrix->xx)*180/G_PI;

  adj = gtk_adjustment_new(angle, -180.0, 180.0, 1.0, 15.0, 0);
  gtk_spin_button_set_adjustment(GTK_SPIN_BUTTON(widget), GTK_ADJUSTMENT (adj));
}

static void 
matrixprop_set_from_widget(MatrixProperty *prop, GtkWidget *widget) 
{
  real angle = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
  real old_angle = 0.0;
  if (angle != 0.0) {
    if (!prop->matrix) {
      prop->matrix = g_new0 (DiaMatrix, 1);

      prop->matrix->xx = 1.0;
      prop->matrix->yy = 1.0;
    } else {
      old_angle = atan2 (prop->matrix->xy, prop->matrix->xx);
      old_angle = 180*old_angle/G_PI;
    }
    cairo_matrix_rotate ((cairo_matrix_t *)prop->matrix, G_PI*(old_angle-angle)/180);
  } else {
    g_free (prop->matrix);
    prop->matrix = NULL;
  }
}

static const PropertyOps matrixprop_ops = {
  (PropertyType_New) matrixprop_new,
  (PropertyType_Free) matrixprop_free,
  (PropertyType_Copy) matrixprop_copy,
  (PropertyType_Load) matrixprop_load,
  (PropertyType_Save) matrixprop_save,
  (PropertyType_GetWidget) matrixprop_get_widget,
  (PropertyType_ResetWidget) matrixprop_reset_widget,
  (PropertyType_SetFromWidget) matrixprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) matrixprop_get_from_offset,
  (PropertyType_SetFromOffset) matrixprop_set_from_offset
};

void 
prop_matrix_register(void)
{
  prop_type_register(PROP_TYPE_MATRIX, &matrixprop_ops);
}
