/* Dia -- a diagram creation/manipulation program -*- c -*-
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 * Major restructuration done in August 2001 by C. Chepelov
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#undef GTK_DISABLE_DEPRECATED /* gtk_signal_connect, ... */
#include <gtk/gtk.h>
#define WIDGET GtkWidget
#include "widgets.h"
#include "properties.h"
#include "propinternals.h"
#include "object.h"

const gchar *prop_dialogdata_key = "object-props:dialogdata";

static void prop_dialog_signal_destroy(GtkWidget *dialog_widget);
static void prop_dialog_fill(PropDialog *dialog, Object *obj, gboolean is_default);

PropDialog *
prop_dialog_new(Object *obj, gboolean is_default) 
{
  PropDialog *dialog = g_new0(PropDialog,1);
  dialog->props = NULL;
  dialog->widget = gtk_vbox_new(FALSE,1);
  dialog->prop_widgets = g_array_new(FALSE,TRUE,sizeof(PropWidgetAssoc));
  dialog->obj_copy = NULL;
  dialog->curtable = NULL;
  dialog->containers = g_ptr_array_new();

  prop_dialog_container_push(dialog,dialog->widget);

  gtk_object_set_data(GTK_OBJECT(dialog->widget), prop_dialogdata_key, dialog);
  gtk_signal_connect(GTK_OBJECT(dialog->widget), "destroy",
                     GTK_SIGNAL_FUNC(prop_dialog_signal_destroy), NULL);

  prop_dialog_fill(dialog,obj,is_default);

  return dialog;
}

void
prop_dialog_destroy(PropDialog *dialog)
{
  if (dialog->props) prop_list_free(dialog->props);
  g_array_free(dialog->prop_widgets,TRUE);
  g_ptr_array_free(dialog->containers,TRUE);
  if (dialog->obj_copy) dialog->obj_copy->ops->destroy(dialog->obj_copy);
  g_free(dialog);
}

PropDialog *
prop_dialog_from_widget(GtkWidget *dialog_widget)
{
  return gtk_object_get_data(GTK_OBJECT(dialog_widget),
                             prop_dialogdata_key);
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
  GtkWidget *table = gtk_table_new(1,2,FALSE);  
  gtk_table_set_row_spacings(GTK_TABLE(table), 2);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_widget_show(table);
  prop_dialog_add_raw(dialog,table);

  dialog->currow = 0;
  dialog->curtable = table;
}

static void
prop_dialog_add_widget(PropDialog *dialog, GtkWidget *label, GtkWidget *widget)
{
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

  if (!dialog->curtable) prop_dialog_make_curtable(dialog);
  gtk_table_attach(GTK_TABLE(dialog->curtable),label, 
                   0,1,
                   dialog->currow, dialog->currow+1,
                   GTK_FILL, GTK_FILL|GTK_EXPAND,
                   0,0);
  gtk_table_attach(GTK_TABLE(dialog->curtable),widget, 
                   1,2,
                   dialog->currow, dialog->currow+1,
                   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND,
                   0,0);
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
property_signal_handler(GtkObject *obj,
                        gpointer func_data)
{
  PropEventData *ped = (PropEventData *)func_data;
  if (ped) {
    PropDialog *dialog = ped->dialog;
    Property *prop = ped->self;
    Object *obj = dialog->obj_copy;
    int j;

    g_assert(prop->event_handler);
    g_assert(obj);
    g_assert(object_complies_with_stdprop(obj));
    g_assert(obj->ops->set_props);
    g_assert(obj->ops->get_props);

    /* g_message("Received a signal for object %s from property %s",
       obj->type->name,prop->name); */

    /* obj is a scratch object ; we can do what we want with it. */
    prop_get_data_from_widgets(dialog);

    obj->ops->set_props(obj,dialog->props);
    prop->event_handler(obj,prop);
    obj->ops->get_props(obj,dialog->props);

    for (j = 0; j < dialog->prop_widgets->len; j++) {
      PropWidgetAssoc *pwa = 
        &g_array_index(dialog->prop_widgets,PropWidgetAssoc,j);
      /*
      g_message("going to reset widget %p for prop %s/%s "
                "[in response to event on  %s/%s]",
                pwa->widget,pwa->prop->type,pwa->prop->name,
                prop->type,prop->name); */ 
      pwa->prop->ops->reset_widget(pwa->prop,pwa->widget);
    } 
  } else {
    g_assert_not_reached();
  }
}

void 
prophandler_connect(const Property *prop,
                    GtkObject *object,
                    const gchar *signal)
{
  Object *obj = prop->self.dialog->obj_copy;

  if (!prop->event_handler) return;
  if (0==strcmp(signal,"FIXME")) {
    g_warning("signal type unknown for this kind of property (name is %s), \n"
              "handler ignored.",prop->name);
    return;
  } 
  if ((!obj->ops->set_props) || (!obj->ops->get_props)) {
      g_warning("object has no [sg]et_props() routine(s).\n"
                "event handler for property %s ignored.",
                prop->name);
      return;
  }
  
  /* g_message("connected signal %s to property %s",signal,prop->name); */

  gtk_signal_connect(object,
                     signal,
                     GTK_SIGNAL_FUNC(property_signal_handler),
                     (gpointer)(&prop->self));
}

void 
prop_dialog_add_property(PropDialog *dialog, Property *prop)
{
  GtkWidget *widget = NULL;
  PropWidgetAssoc pwa;
  GtkWidget *label;

  if ((prop->event_handler) && (!dialog->obj_copy)) 
    dialog->obj_copy = dialog->orig_obj->ops->copy(dialog->orig_obj);

  prop->self.dialog = dialog;
  prop->self.self = prop;
  prop->self.my_index = dialog->prop_widgets->len;

  if (prop->ops->get_widget) widget = prop->ops->get_widget(prop,dialog);  
  if (!widget) return; /* either property has no widget or it's a container */

  prop->self.widget = widget;
  if (prop->ops->reset_widget) prop->ops->reset_widget(prop,widget);
  
  pwa.prop = prop;
  pwa.widget = widget;  
  g_array_append_val(dialog->prop_widgets,pwa);  

  label = gtk_label_new(_(prop->descr->description));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

  prop_dialog_add_widget(dialog, label, widget);
}

static void 
prop_dialog_add_properties(PropDialog *dialog, GPtrArray *props)
{
  guint i;

  for (i = 0; i < props->len; i++) {
    Property *prop = (Property*)g_ptr_array_index(props,i);
    prop_dialog_add_property(dialog, prop);
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
extern gboolean pdtpp_is_visible_no_standard(const PropDescription *pdesc);
static void 
prop_dialog_fill(PropDialog *dialog, Object *obj, gboolean is_default) {
  const PropDescription *pdesc;
  GPtrArray *props;

  g_return_if_fail(object_complies_with_stdprop(obj));

  dialog->orig_obj = obj;

  pdesc = object_get_prop_descriptions(obj);
  if (!pdesc) return;

  if (is_default)
      props = prop_list_from_descs(pdesc,pdtpp_is_visible_no_standard);
  else
      props = prop_list_from_descs(pdesc,pdtpp_is_visible);

  if (!props) return;
    
  dialog->props = props;
  obj->ops->get_props(obj,props);

  prop_dialog_add_properties(dialog, props);
}



