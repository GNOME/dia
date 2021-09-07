/* Dia -- a diagram creation/manipulation program -*- c -*-
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 * Major restructuration done in August 2001 by C. Chepelov
 *
 * Property types for "widget" types (static, buttons, notebooks, frames, etc.)
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

#include <glib.h>
#include <gtk/gtk.h>
#define WIDGET GtkWidget
#include "properties.h"
#include "propinternals.h"
#include "dia-simple-list.h"

/******************************/
/* The STATIC property type.  */
/******************************/

static WIDGET *
staticprop_get_widget(StaticProperty *prop, PropDialog *dialog)
{
  GtkWidget *ret = NULL;
  if (!prop->common.descr) return NULL;

  ret = gtk_label_new(prop->common.descr->tooltip);
  gtk_label_set_justify(GTK_LABEL(ret),GTK_JUSTIFY_LEFT);
  return ret;
}

static const PropertyOps staticprop_ops = {
  (PropertyType_New) noopprop_new,
  (PropertyType_Free) noopprop_free,
  (PropertyType_Copy) noopprop_copy,
  (PropertyType_Load) invalidprop_load,
  (PropertyType_Save) invalidprop_save,
  (PropertyType_GetWidget) staticprop_get_widget,
  (PropertyType_ResetWidget) noopprop_reset_widget,
  (PropertyType_SetFromWidget) noopprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) noopprop_get_from_offset,
  (PropertyType_SetFromOffset) noopprop_set_from_offset
};


/******************************/
/* The BUTTON property type.  */
/******************************/

static WIDGET *
buttonprop_get_widget(ButtonProperty *prop, PropDialog *dialog)
{
  GtkWidget *ret = NULL;
  if (!prop->common.descr) return NULL;

  ret = gtk_button_new_with_label(_(prop->common.descr->tooltip));
  prophandler_connect(&prop->common, G_OBJECT(ret), "clicked");

  return ret;
}

static const PropertyOps buttonprop_ops = {
  (PropertyType_New) noopprop_new,
  (PropertyType_Free) noopprop_free,
  (PropertyType_Copy) noopprop_copy,
  (PropertyType_Load) invalidprop_load,
  (PropertyType_Save) invalidprop_save,
  (PropertyType_GetWidget) buttonprop_get_widget,
  (PropertyType_ResetWidget) noopprop_reset_widget,
  (PropertyType_SetFromWidget) noopprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) noopprop_get_from_offset,
  (PropertyType_SetFromOffset) noopprop_set_from_offset
};

/**************************************************/
/* The FRAME_BEGIN and FRAME_END property types.  */
/**************************************************/

static GtkWidget *
frame_beginprop_get_widget (FrameProperty *prop, PropDialog *dialog)
{
  GtkWidget *frame = gtk_expander_new (_(prop->common.descr->description));
  GtkWidget *vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

  gtk_expander_set_expanded (GTK_EXPANDER (frame), TRUE);
  gtk_widget_show (frame);

  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  prop_dialog_add_raw (dialog, frame);

  prop_dialog_container_push (dialog, vbox);

  return NULL; /* there is no single widget to add with a label next to it. */
}


static GtkWidget *
frame_endprop_get_widget (FrameProperty *prop, PropDialog *dialog)
{
  prop_dialog_container_pop (dialog);
  return NULL;
}


static const PropertyOps frame_beginprop_ops = {
  (PropertyType_New) noopprop_new,
  (PropertyType_Free) noopprop_free,
  (PropertyType_Copy) noopprop_copy,
  (PropertyType_Load) invalidprop_load,
  (PropertyType_Save) invalidprop_save,
  (PropertyType_GetWidget) frame_beginprop_get_widget,
  (PropertyType_ResetWidget) noopprop_reset_widget,
  (PropertyType_SetFromWidget) noopprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) noopprop_get_from_offset,
  (PropertyType_SetFromOffset) noopprop_set_from_offset
};

static const PropertyOps frame_endprop_ops = {
  (PropertyType_New) noopprop_new,
  (PropertyType_Free) noopprop_free,
  (PropertyType_Copy) noopprop_copy,
  (PropertyType_Load) invalidprop_load,
  (PropertyType_Save) invalidprop_save,
  (PropertyType_GetWidget) frame_endprop_get_widget,
  (PropertyType_ResetWidget) noopprop_reset_widget,
  (PropertyType_SetFromWidget) noopprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) noopprop_get_from_offset,
  (PropertyType_SetFromOffset) noopprop_set_from_offset
};

/*********************************************************/
/* The MULTICOL_BEGIN, _COLUMN and _END property types.  */
/*********************************************************/

static WIDGET *
multicol_beginprop_get_widget(MulticolProperty *prop, PropDialog *dialog)
{
  GtkWidget *multicol = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);

  gtk_container_set_border_width (GTK_CONTAINER(multicol), 2);
  gtk_widget_show(multicol);

  prop_dialog_add_raw(dialog,multicol);

  prop_dialog_container_push(dialog,multicol);
  prop_dialog_container_push(dialog,NULL); /* there must be a _COLUMN soon */

  return NULL; /* there is no single widget to add with a label next to it. */
}

static WIDGET *
multicol_columnprop_get_widget(MulticolProperty *prop, PropDialog *dialog)
{
  GtkWidget *col = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);

  gtk_container_set_border_width (GTK_CONTAINER(col), 2);
  gtk_widget_show(col);

  prop_dialog_container_pop(dialog); /* NULL or the previous column */

  gtk_box_pack_start(GTK_BOX(dialog->lastcont),col,TRUE,TRUE,0);

  prop_dialog_add_raw(dialog,NULL); /* to reset the internal table system */
  prop_dialog_container_push(dialog,col);

  return NULL;
}

static WIDGET *
multicol_endprop_get_widget(MulticolProperty *prop, PropDialog *dialog)
{
  prop_dialog_container_pop(dialog); /* the column */
  prop_dialog_container_pop(dialog); /* the multicol */
  return NULL;
}

static const PropertyOps multicol_beginprop_ops = {
  (PropertyType_New) noopprop_new,
  (PropertyType_Free) noopprop_free,
  (PropertyType_Copy) noopprop_copy,
  (PropertyType_Load) invalidprop_load,
  (PropertyType_Save) invalidprop_save,
  (PropertyType_GetWidget) multicol_beginprop_get_widget,
  (PropertyType_ResetWidget) noopprop_reset_widget,
  (PropertyType_SetFromWidget) noopprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) noopprop_get_from_offset,
  (PropertyType_SetFromOffset) noopprop_set_from_offset
};

static const PropertyOps multicol_columnprop_ops = {
  (PropertyType_New) noopprop_new,
  (PropertyType_Free) noopprop_free,
  (PropertyType_Copy) noopprop_copy,
  (PropertyType_Load) invalidprop_load,
  (PropertyType_Save) invalidprop_save,
  (PropertyType_GetWidget) multicol_columnprop_get_widget,
  (PropertyType_ResetWidget) noopprop_reset_widget,
  (PropertyType_SetFromWidget) noopprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) noopprop_get_from_offset,
  (PropertyType_SetFromOffset) noopprop_set_from_offset
};

static const PropertyOps multicol_endprop_ops = {
  (PropertyType_New) noopprop_new,
  (PropertyType_Free) noopprop_free,
  (PropertyType_Copy) noopprop_copy,
  (PropertyType_Load) invalidprop_load,
  (PropertyType_Save) invalidprop_save,
  (PropertyType_GetWidget) multicol_endprop_get_widget,
  (PropertyType_ResetWidget) noopprop_reset_widget,
  (PropertyType_SetFromWidget) noopprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) noopprop_get_from_offset,
  (PropertyType_SetFromOffset) noopprop_set_from_offset
};

/*********************************************************/
/* The NOTEBOOK_BEGIN, _PAGE and _END property types.  */
/*********************************************************/

static WIDGET *
notebook_beginprop_get_widget(NotebookProperty *prop, PropDialog *dialog)
{
  GtkWidget *notebook = gtk_notebook_new();

  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook),GTK_POS_TOP);
  gtk_container_set_border_width (GTK_CONTAINER(notebook), 1);
  gtk_widget_show(notebook);

  prop_dialog_add_raw(dialog,notebook);

  prop_dialog_container_push(dialog,notebook);
  prop_dialog_container_push(dialog,NULL); /* there must be a _PAGE soon */

  return NULL; /* there is no single widget to add with a label next to it. */
}

static WIDGET *
notebook_pageprop_get_widget(NotebookProperty *prop, PropDialog *dialog)
{
  GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
  GtkWidget *label = gtk_label_new(_(prop->common.descr->description));

  gtk_container_set_border_width (GTK_CONTAINER(page), 2);
  gtk_widget_show(page);
  gtk_widget_show(label);

  prop_dialog_container_pop(dialog); /* NULL or the previous page */

  gtk_notebook_append_page(GTK_NOTEBOOK(dialog->lastcont),page,label);

  prop_dialog_add_raw(dialog,NULL); /* to reset the internal table system */
  prop_dialog_container_push(dialog,page);

  return NULL;
}

static WIDGET *
notebook_endprop_get_widget(NotebookProperty *prop, PropDialog *dialog)
{
  prop_dialog_container_pop(dialog); /* the page */
  prop_dialog_container_pop(dialog); /* the notebook */
  return NULL;
}

static const PropertyOps notebook_beginprop_ops = {
  (PropertyType_New) noopprop_new,
  (PropertyType_Free) noopprop_free,
  (PropertyType_Copy) noopprop_copy,
  (PropertyType_Load) invalidprop_load,
  (PropertyType_Save) invalidprop_save,
  (PropertyType_GetWidget) notebook_beginprop_get_widget,
  (PropertyType_ResetWidget) noopprop_reset_widget,
  (PropertyType_SetFromWidget) noopprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) noopprop_get_from_offset,
  (PropertyType_SetFromOffset) noopprop_set_from_offset
};

static const PropertyOps notebook_pageprop_ops = {
  (PropertyType_New) noopprop_new,
  (PropertyType_Free) noopprop_free,
  (PropertyType_Copy) noopprop_copy,
  (PropertyType_Load) invalidprop_load,
  (PropertyType_Save) invalidprop_save,
  (PropertyType_GetWidget) notebook_pageprop_get_widget,
  (PropertyType_ResetWidget) noopprop_reset_widget,
  (PropertyType_SetFromWidget) noopprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) noopprop_get_from_offset,
  (PropertyType_SetFromOffset) noopprop_set_from_offset
};

static const PropertyOps notebook_endprop_ops = {
  (PropertyType_New) noopprop_new,
  (PropertyType_Free) noopprop_free,
  (PropertyType_Copy) noopprop_copy,
  (PropertyType_Load) invalidprop_load,
  (PropertyType_Save) invalidprop_save,
  (PropertyType_GetWidget) notebook_endprop_get_widget,
  (PropertyType_ResetWidget) noopprop_reset_widget,
  (PropertyType_SetFromWidget) noopprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) noopprop_get_from_offset,
  (PropertyType_SetFromOffset) noopprop_set_from_offset
};

/****************************/
/* The LIST property type.  */
/****************************/

static void
listprop_emptylines_realloc (ListProperty *prop, guint new_size) {
  guint i;

  for (i = 0; i < prop->lines->len; i++) {
    g_clear_pointer (&g_ptr_array_index (prop->lines, i), g_free);
  }
  g_ptr_array_set_size (prop->lines,new_size);
}


static void
listprop_copylines (ListProperty *prop, GPtrArray *src) {
  guint i;

  listprop_emptylines_realloc (prop, src->len);

  for (i = 0; i < src->len; i++) {
    g_ptr_array_index (prop->lines, i) = g_strdup (g_ptr_array_index (src, i));
  }
}


static ListProperty *
listprop_new (const PropDescription *pdesc, PropDescToPropPredicate reason)
{
  ListProperty *prop = g_new0 (ListProperty,1);
  initialize_property (&prop->common, pdesc, reason);
  prop->selected = -1;
  prop->lines = g_ptr_array_new ();
  return prop;
}


static void
listprop_free (ListProperty *prop)
{
  listprop_emptylines_realloc (prop,-1);
  g_ptr_array_free (prop->lines,TRUE);
}


static ListProperty *
listprop_copy (ListProperty *src)
{
  ListProperty *prop =
    (ListProperty *) src->common.ops->new_prop (src->common.descr,
                                                src->common.reason);

  copy_init_property (&prop->common,&src->common);
  prop->selected = src->selected;
  listprop_copylines (prop,src->lines);

  return prop;
}


static GtkWidget *
listprop_get_widget (ListProperty *prop, PropDialog *dialog)
{
  GtkWidget *ret = dia_simple_list_new ();

  prophandler_connect (&prop->common, G_OBJECT(ret), "selection-changed");

  return ret;
}


static void
listprop_reset_widget (ListProperty *prop, GtkWidget *widget)
{
  guint i;

  g_return_if_fail (DIA_IS_SIMPLE_LIST (widget));

  dia_simple_list_empty (DIA_SIMPLE_LIST (widget));

  for (i = 0; i < prop->lines->len; i++) {
    dia_simple_list_append (DIA_SIMPLE_LIST (widget),
                            g_ptr_array_index (prop->lines, i));
  }

  dia_simple_list_select (DIA_SIMPLE_LIST (widget), prop->selected);
}


static void
listprop_set_from_widget (ListProperty *prop, WIDGET *widget)
{
  prop->selected = dia_simple_list_get_selected (DIA_SIMPLE_LIST (widget));
}


static void
listprop_get_from_offset (ListProperty *prop,
                          void         *base,
                          guint         offset,
                          guint         offset2)
{
  listprop_copylines (prop, struct_member (base,offset, GPtrArray *));
  prop->selected = struct_member (base,offset2,gint);
}


static void
listprop_set_from_offset (ListProperty *prop,
                          void         *base,
                          guint         offset,
                          guint         offset2)
{
  struct_member (base, offset2, gint) = prop->selected;
}


static const PropertyOps listprop_ops = {
  (PropertyType_New) listprop_new,
  (PropertyType_Free) listprop_free,
  (PropertyType_Copy) listprop_copy,

  (PropertyType_Load) invalidprop_load,
  (PropertyType_Save) invalidprop_save,

  (PropertyType_GetWidget) listprop_get_widget,
  (PropertyType_ResetWidget) listprop_reset_widget,
  (PropertyType_SetFromWidget) listprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) listprop_get_from_offset,
  (PropertyType_SetFromOffset) listprop_set_from_offset
};

/* ************************************************************** */

void
prop_widgets_register(void)
{
  prop_type_register(PROP_TYPE_STATIC,&staticprop_ops);
  prop_type_register(PROP_TYPE_BUTTON,&buttonprop_ops);
  prop_type_register(PROP_TYPE_FRAME_BEGIN,&frame_beginprop_ops);
  prop_type_register(PROP_TYPE_FRAME_END,&frame_endprop_ops);
  prop_type_register(PROP_TYPE_MULTICOL_BEGIN,&multicol_beginprop_ops);
  prop_type_register(PROP_TYPE_MULTICOL_END,&multicol_endprop_ops);
  prop_type_register(PROP_TYPE_MULTICOL_COLUMN,&multicol_columnprop_ops);
  prop_type_register(PROP_TYPE_NOTEBOOK_BEGIN,&notebook_beginprop_ops);
  prop_type_register(PROP_TYPE_NOTEBOOK_END,&notebook_endprop_ops);
  prop_type_register(PROP_TYPE_NOTEBOOK_PAGE,&notebook_pageprop_ops);
  prop_type_register(PROP_TYPE_LIST,&listprop_ops);
}
