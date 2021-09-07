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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>
#include <gtk/gtk.h>
#define WIDGET GtkWidget
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
  g_clear_pointer (&prop->matrix, g_free);
  g_clear_pointer (&prop, g_free);
}

static MatrixProperty *
matrixprop_copy(MatrixProperty *src)
{
  MatrixProperty *prop =
    (MatrixProperty *)src->common.ops->new_prop(src->common.descr,
                                              src->common.reason);

  prop->matrix = g_memdup2 (src->matrix, sizeof(DiaMatrix));

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

  matrix = g_new0 (DiaMatrix, 1);
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
    g_clear_pointer (&matrix, g_free);
    return NULL;
  }
  /* TODO: check it's invertible? */

  return matrix;
}

static void
matrixprop_load(MatrixProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
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
data_add_matrix (AttributeNode attr, DiaMatrix *matrix, DiaContext *ctx)
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
matrixprop_save(MatrixProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  if (prop->matrix) {
    data_add_matrix (attr, prop->matrix, ctx);
  }
}

static void
matrixprop_get_from_offset(MatrixProperty *prop,
                         void *base, guint offset, guint offset2)
{
  DiaMatrix *matrix = struct_member(base,offset,DiaMatrix *);

  prop->matrix = g_memdup2 (matrix, sizeof (DiaMatrix));
}

static void
matrixprop_set_from_offset(MatrixProperty *prop,
                           void *base, guint offset, guint offset2)
{
  DiaMatrix *dest = struct_member(base,offset,DiaMatrix *);

  g_clear_pointer (&dest, g_free);

  struct_member (base, offset, DiaMatrix *) = g_memdup2 (prop->matrix, sizeof (DiaMatrix));
}

/* GUI stuff - just the angle for now
 */
static GtkWidget *
matrixprop_get_widget (MatrixProperty *prop, PropDialog *dialog)
{
  GtkAdjustment *adj;
  GtkWidget *ret, *sb;
  int i;

  ret = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  /* angle */
  adj = GTK_ADJUSTMENT (gtk_adjustment_new(0.0, -180.0, 180.0, 1.0, 15.0, 0));
  sb = gtk_spin_button_new(adj, 1.0, 2);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(sb),TRUE);
  prophandler_connect(&prop->common, G_OBJECT(sb), "changed");
  gtk_widget_show(sb);
  gtk_box_pack_start(GTK_BOX(ret), sb, TRUE, TRUE, 0);
  /* sx, sy */
  for (i = 0; i < 2; ++i) {
    adj = GTK_ADJUSTMENT (gtk_adjustment_new(0.0, 0.01, 100.0, 0.01, 1.0, 0));
    sb = gtk_spin_button_new(GTK_ADJUSTMENT (adj), 1.0, 2);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(sb),TRUE);
    prophandler_connect(&prop->common, G_OBJECT(sb), "changed");
    gtk_widget_show(sb);
    gtk_box_pack_start(GTK_BOX(ret), sb, TRUE, TRUE, 0);
  }

  return ret;
}

static void
matrixprop_reset_widget(MatrixProperty *prop, GtkWidget *widget)
{
  GList *children, *child;
  GtkWidget *sb;
  real angle, sx, sy;
  int i = 0;

  if (!prop->matrix) {
    angle = 0;
    sx = sy = 1.0;
  } else {
    real a;

    dia_matrix_get_angle_and_scales (prop->matrix, &a, &sx, &sy);

    angle = -a*180/G_PI;
  }

  children = gtk_container_get_children (GTK_CONTAINER (widget));
  for (child = children; child != NULL; child = g_list_next (child)) {
    GtkAdjustment *adj;
    sb = child->data;
    adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON(sb));
    if (i == 0)
      gtk_adjustment_configure (adj, angle, -180.0, 180.0, 1.0, 15.0, 0);
    else if (i == 1)
      gtk_adjustment_configure (adj, sx, 0.01, 100.0, 0.1, 1.0, 0);
    else if (i == 2)
      gtk_adjustment_configure (adj, sy, 0.01, 100.0, 0.1, 1.0, 0);
    else
      g_assert_not_reached ();
    ++i;
  }
}

static void
matrixprop_set_from_widget(MatrixProperty *prop, GtkWidget *widget)
{
  GList *children, *child;
  GtkWidget *sb;
  real angle = 0.0, sx = 1.0, sy = 1.0;
  int i = 0;

  children = gtk_container_get_children (GTK_CONTAINER (widget));
  for (child = children; child != NULL; child = g_list_next (child)) {
    sb = child->data;
    if (i == 0)
      angle = gtk_spin_button_get_value(GTK_SPIN_BUTTON(sb));
    else if (i == 1)
      sx = gtk_spin_button_get_value(GTK_SPIN_BUTTON(sb));
    else if (i == 2)
      sy = gtk_spin_button_get_value(GTK_SPIN_BUTTON(sb));
    else
      g_assert_not_reached ();
    ++i;
  }

  if (angle != 0.0 || sx != 1.0 || sy != 1.0) {
    if (!prop->matrix) {
      prop->matrix = g_new0 (DiaMatrix, 1);
    }
    dia_matrix_set_angle_and_scales (prop->matrix, -angle/180.0*G_PI, sx, sy);
  } else {
    g_clear_pointer (&prop->matrix, g_free);
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
