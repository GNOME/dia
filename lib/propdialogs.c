/* Dia -- a diagram creation/manipulation program -*- c -*-
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 * Major restructuration done in August 2001 by C. Chepelov
 *
 * © 2023 Hubert Figuière <hub@figuiere.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Routines for the construction of a property dialog.
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
#include "object.h"

const gchar *prop_dialogdata_key = "object-props:dialogdata";

static void prop_dialog_signal_destroy(GtkWidget *dialog_widget);
static void prop_dialog_fill(PropDialog *dialog, GList *objects, gboolean is_default);

PropDialog *
prop_dialog_new(GList *objects, gboolean is_default)
{
  PropDialog *dialog = g_new0(PropDialog,1);
  dialog->props = NULL;
  dialog->widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
  dialog->prop_widgets = g_array_new(FALSE,TRUE,sizeof(PropWidgetAssoc));
  dialog->copies = NULL;
  dialog->curtable = NULL;
  dialog->containers = g_ptr_array_new();

  prop_dialog_container_push(dialog,dialog->widget);

  g_object_set_data (G_OBJECT (dialog->widget), prop_dialogdata_key, dialog);
  g_signal_connect (G_OBJECT (dialog->widget), "destroy",
                    G_CALLBACK (prop_dialog_signal_destroy), NULL);

  prop_dialog_fill(dialog,objects,is_default);

  return dialog;
}

void
prop_dialog_destroy(PropDialog *dialog)
{
  if (dialog->props) prop_list_free(dialog->props);
  g_array_free(dialog->prop_widgets,TRUE);
  g_ptr_array_free(dialog->containers,TRUE);
  if (dialog->copies) destroy_object_list(dialog->copies);
  g_free(dialog);
}

PropDialog *
prop_dialog_from_widget(GtkWidget *dialog_widget)
{
  return g_object_get_data (G_OBJECT (dialog_widget), prop_dialogdata_key);
}

static void
prop_dialog_signal_destroy(GtkWidget *dialog_widget)
{
  prop_dialog_destroy(prop_dialog_from_widget(dialog_widget));
}

WIDGET *
prop_dialog_get_widget(const PropDialog *dialog)
{
  return dialog->widget;
}

void
prop_dialog_add_raw(PropDialog *dialog, GtkWidget *widget)
{
  dialog->curtable = NULL;
  if (!widget) return;
  gtk_container_add(GTK_CONTAINER(dialog->lastcont),widget);
}

void
prop_dialog_add_raw_with_flags(PropDialog *dialog, GtkWidget *widget,
			       gboolean expand, gboolean fill)
{
  g_return_if_fail(GTK_IS_BOX(dialog->lastcont));

  dialog->curtable = NULL;
  if (!widget) return;
  gtk_box_pack_start(GTK_BOX(dialog->lastcont),widget, expand, fill, 0);
}

static void
prop_dialog_make_curtable(PropDialog *dialog)
{
  GtkWidget *table = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(table), 6);
  gtk_grid_set_column_spacing(GTK_GRID(table), 6);
  gtk_widget_show(table);
  prop_dialog_add_raw(dialog,table);

  dialog->currow = 0;
  dialog->curtable = table;
}

static void
prop_dialog_add_widget(PropDialog *dialog, GtkWidget *label, GtkWidget *widget)
{
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

  if (!dialog->curtable)
    prop_dialog_make_curtable(dialog);
  gtk_grid_attach(GTK_GRID(dialog->curtable), label,
                  0, dialog->currow, 1, 1);
  gtk_widget_set_vexpand(label, FALSE);

  if (GTK_IS_SWITCH (widget)) {
    GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX (box), widget, FALSE, TRUE, 0);
    gtk_widget_show (widget);
    widget = box;
  }

  gtk_widget_set_hexpand(widget, TRUE);

  gtk_grid_attach(GTK_GRID(dialog->curtable), widget,
                  1, dialog->currow, 1, 1);
  gtk_widget_show(label);
  gtk_widget_show(widget);
  dialog->currow++;
}

/* Register a new container widget (which won't be automatically added) */
void
prop_dialog_container_push(PropDialog *dialog, GtkWidget *container)
{
  g_ptr_array_add(dialog->containers,container);
  dialog->lastcont = container;
  dialog->curtable = NULL;
}

/* De-register the last container of the stack. */
GtkWidget *
prop_dialog_container_pop(PropDialog *dialog)
{
  GtkWidget *res = g_ptr_array_remove_index(dialog->containers,
                                            dialog->containers->len-1);
  dialog->lastcont = g_ptr_array_index(dialog->containers,
                                       dialog->containers->len-1);
  dialog->curtable = NULL;

  return res;
}

static void
property_signal_handler(GObject *obj,
                        gpointer func_data)
{
  PropEventData *ped = (PropEventData *)func_data;
  if (ped) {
    PropDialog *dialog = ped->dialog;
    Property *prop = ped->self;
    GList *list, *tmp;
    guint j;

    list = dialog->copies;
    g_return_if_fail(list);

    prop->experience &= ~PXP_NOTSET;

    if (!prop->event_handler)
	return;

    /* g_message("Received a signal for object %s from property %s",
       obj->type->name,prop->descr->name); */

    /* obj is a scratch object ; we can do what we want with it. */
    prop_get_data_from_widgets(dialog);

    for (tmp = list; tmp != NULL; tmp = tmp->next) {
      DiaObject *item = (DiaObject*) tmp->data;
      dia_object_set_properties (item, dialog->props);
      prop->event_handler (item, prop);
      dia_object_get_properties (item, dialog->props);
    }

    for (j = 0; j < dialog->prop_widgets->len; j++) {
      PropWidgetAssoc *pwa =
        &g_array_index(dialog->prop_widgets,PropWidgetAssoc,j);
      /* The event handler above might have changed every property
       * so we would have to mark them all as set (aka. to be applied).
       * But doing so would not work well with multiple/grouped objects
       * with unchanged and unequal object properties. Idea: keep as
       * set, what was before calling reset (but it's too late after get_props).
       *
       * See also commonprop_reset_widget() for more information
       */
      gboolean was_set = ((pwa->prop->experience & PXP_NOTSET) == 0);
      pwa->prop->ops->reset_widget(pwa->prop,pwa->widget);
      if (was_set)
	pwa->prop->experience &= ~PXP_NOTSET;
    }
    /* once more at least for _this_ property otherwise a property with
     * signal handler attached would not be changed at all ...
     */
    prop->experience &= ~PXP_NOTSET;
  } else {
    g_assert_not_reached();
  }
}

static void
property_notify_forward(GObject *obj,
               GParamSpec *pspec,
               gpointer func_data)
{
  property_signal_handler(obj, func_data);
}

static void
_prophandler_connect(const Property *prop,
                     GObject *object,
                     const gchar *signal,
                     GCallback handler)
{
  if (0==strcmp(signal,"FIXME")) {
    g_warning("signal type unknown for this kind of property (name is %s), \n"
              "handler ignored.",prop->descr->name);
    return;
  }

  g_signal_connect (G_OBJECT (object),
                    signal,
                    handler,
                    (gpointer)(&prop->self_event_data));
}

void
prophandler_connect_notify(const Property *prop,
                    GObject *object,
                    const gchar *signal)
{
  _prophandler_connect(prop, object, signal, G_CALLBACK (property_notify_forward));
}

void
prophandler_connect(const Property *prop,
                    GObject *object,
                    const gchar *signal)
{
  _prophandler_connect(prop, object, signal, G_CALLBACK (property_signal_handler));
}

void
prop_dialog_add_property(PropDialog *dialog, Property *prop)
{
  GtkWidget *widget = NULL;
  PropWidgetAssoc pwa;
  GtkWidget *label;

  prop->self_event_data.dialog = dialog;
  prop->self_event_data.self = prop;

  if (prop->ops->get_widget)
    widget = prop->ops->get_widget(prop,dialog);
  if (!widget)
    return; /* either property has no widget or it's a container */

  prop->self_event_data.widget = widget;
  if (prop->ops->reset_widget) prop->ops->reset_widget(prop,widget);
  prop->experience |= PXP_NOTSET;

  pwa.prop = prop;
  pwa.widget = widget;
  g_array_append_val(dialog->prop_widgets,pwa);

  /* Don't let gettext translate the empty string (it would give po-info).
   * Admitted it is a hack the string is empty for meta info, but the
   * gettext behavior looks suspicious, too.
   */
  if (strlen (prop->descr->description) == 0)
    label = gtk_label_new("");
  else
    label = gtk_label_new(_(prop->descr->description));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

  prop_dialog_add_widget(dialog, label, widget);
}

static void
prop_dialog_add_properties(PropDialog *dialog, GPtrArray *props)
{
  guint i;
  gboolean scrollable = (props->len > 16);

  if (scrollable) {
    GtkWidget *swin = gtk_scrolled_window_new (NULL, NULL);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_box_pack_start(GTK_BOX (dialog->widget), swin, TRUE, TRUE, 0);
    gtk_widget_show (swin);
    gtk_container_add (GTK_CONTAINER (swin), vbox);
    gtk_viewport_set_shadow_type(GTK_VIEWPORT(gtk_bin_get_child(GTK_BIN(swin))), GTK_SHADOW_NONE);
    gtk_widget_show (vbox);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swin), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    prop_dialog_container_push (dialog, swin);
    prop_dialog_container_push (dialog, vbox);
  }


  for (i = 0; i < props->len; i++) {
    Property *prop = (Property*)g_ptr_array_index(props,i);
    prop_dialog_add_property(dialog, prop);
  }

  if (scrollable) {
    GtkRequisition requisition;
    GtkWidget *vbox = prop_dialog_container_pop(dialog);
    GtkWidget *swin = prop_dialog_container_pop(dialog);
    GdkScreen *screen = gtk_widget_get_screen(swin);
    gint sheight = screen ? (2 * gdk_screen_get_height(screen)) / 3 : 400;

    gtk_widget_get_preferred_size (vbox, NULL, &requisition);
    /* I'd say default size calculation for scrollable is quite broken */
    gtk_widget_set_size_request (swin, -1, requisition.height > sheight ? sheight : requisition.height);
  }
}

void
prop_get_data_from_widgets(PropDialog *dialog)
{
  guint i;
  for (i = 0; i < dialog->prop_widgets->len; i++) {
    PropWidgetAssoc *pwa = &g_array_index(dialog->prop_widgets,
                                          PropWidgetAssoc,i);
    pwa->prop->ops->set_from_widget(pwa->prop,pwa->widget);

  }
}

static gboolean
pdtpp_is_visible_default (const PropDescription *pdesc)
{
  return pdtpp_defaults (pdesc) && pdtpp_is_visible_no_standard(pdesc);
}

static void
_prop_list_extend_for_meta (GPtrArray *props)
{
  static PropDescription extras[] = {
    PROP_STD_NOTEBOOK_BEGIN,
    PROP_NOTEBOOK_PAGE("general_page",PROP_FLAG_DONT_MERGE,N_("General")),
    PROP_NOTEBOOK_PAGE("meta_page",0,N_("Meta")),
    { "meta", PROP_TYPE_DICT, PROP_FLAG_VISIBLE|PROP_FLAG_SELF_ONLY, "", ""},
    PROP_STD_NOTEBOOK_END,
    {NULL}
  };

  /* Some objects have no properties in case of defaults */
  Property *p = props->len > 0 ? g_ptr_array_index(props,0) : NULL;
  GPtrArray *pex = prop_list_from_descs(extras,pdtpp_is_visible);

  if (!p || strcmp (p->descr->type, PROP_TYPE_NOTEBOOK_BEGIN) != 0) {
    int i, olen = props->len;
    /* wrap everything into a first notebook page */
    g_ptr_array_set_size (props, olen + 2);
    /* make room for 2 at the beginning */
    for (i = olen - 1; i >=  0; --i)
      g_ptr_array_index (props, i + 2) = g_ptr_array_index (props, i);
    g_ptr_array_index (props, 0) = g_ptr_array_index (pex, 0);
    g_ptr_array_index (props, 1) = g_ptr_array_index (pex, 1);
  } else {
    p = g_ptr_array_index (props, props->len - 1);
    g_assert (strcmp (p->descr->type, PROP_TYPE_NOTEBOOK_END) == 0);
    /* drop the end, we'll add it again below */
    g_ptr_array_set_size (props, props->len - 1);
  }
  g_ptr_array_add (props, g_ptr_array_index (pex, 2));
  g_ptr_array_add (props, g_ptr_array_index (pex, 3));
  g_ptr_array_add (props, g_ptr_array_index (pex, 4));
  /* free the array, but not the reused segments */
  g_ptr_array_free (pex, FALSE);
}

static void
prop_dialog_fill(PropDialog *dialog, GList *objects, gboolean is_default)
{
  const PropDescription *pdesc;
  GPtrArray *props;

  g_return_if_fail(objects_comply_with_stdprop(objects));

  dialog->objects = g_list_copy(objects);
  dialog->copies = object_copy_list(objects);

  pdesc = object_list_get_prop_descriptions(objects, PROP_UNION);
  if (!pdesc) return;

  if (is_default)
    props = prop_list_from_descs(pdesc,pdtpp_is_visible_default);
  else
    props = prop_list_from_descs(pdesc,pdtpp_is_visible);

  if (!props) return;

  _prop_list_extend_for_meta (props);

  dialog->props = props;
  object_list_get_props(objects, props);

  prop_dialog_add_properties(dialog, props);
}



