/* Dia -- a diagram creation/manipulation program -*- c -*-
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 * Major restructuration done in August 2001 by C. Chepelov
 *
 * Property types for "textual" types (strings, texts, whatever).
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

#include <gtk/gtk.h>
#define WIDGET GtkWidget
#include "widgets.h"
#include "properties.h"
#include "propinternals.h"
#include "text.h"
#include "charconv.h"

/*****************************************************/
/* The STRING, FILE and MULTISTRING property types.  */
/*****************************************************/

static StringProperty *
stringprop_new(const PropDescription *pdesc, PropDescToPropPredicate reason)
{
  StringProperty *prop = g_new0(StringProperty,1);
  initialize_property(&prop->common,pdesc,reason);
  prop->string_data = NULL;
  prop->num_lines = 1;
  return prop;
}

static StringProperty *
multistringprop_new(const PropDescription *pdesc, 
                    PropDescToPropPredicate reason)
{
  StringProperty *prop = g_new0(StringProperty,1);
  initialize_property(&prop->common,pdesc,reason);
  prop->string_data = NULL;
  prop->num_lines = GPOINTER_TO_INT(pdesc->extra_data);
  return prop;
}

static StringProperty *
stringprop_copy(StringProperty *src) 
{
  StringProperty *prop = 
    (StringProperty *)src->common.ops->new_prop(src->common.descr,
                                                 src->common.reason);
  copy_init_property(&prop->common,&src->common);
  if (src->string_data)
    prop->string_data = g_strdup(src->string_data);
  else
    prop->string_data = NULL;
  prop->num_lines = src->num_lines;
  return prop;
}

static void 
stringprop_free(StringProperty *prop) 
{
  g_free(prop->string_data);
  g_free(prop);
}

static WIDGET *
stringprop_get_widget(StringProperty *prop, PropDialog *dialog) 
{ 
  GtkWidget *ret = gtk_entry_new();
  prophandler_connect(&prop->common,GTK_OBJECT(ret),"changed");
  return ret;
}

static void 
stringprop_reset_widget(StringProperty *prop, WIDGET *widget)
{
#ifdef GTK_DOESNT_TALK_UTF8_WE_DO
  gchar *locbuf;

  if (prop->string_data) {
	  locbuf = charconv_utf8_to_local8 (prop->string_data);
	  gtk_entry_set_text (GTK_ENTRY (widget), locbuf);
	  g_free (locbuf);
  } else {
	  gtk_entry_set_text (GTK_ENTRY (widget), "");
  }
#elif  defined(GTK_TALKS_UTF8_WE_DONT)
  utfchar *utfbuf;

  if (prop->string_data) {
	  utfbuf = charconv_local8_to_utf8 (prop->string_data);
	  gtk_entry_set_text (GTK_ENTRY (widget), utfbuf);
	  g_free(utfbuf);
  } else {
	  gtk_entry_set_text (GTK_ENTRY (widget), "");
  }
#else
  gtk_entry_set_text(GTK_ENTRY(widget), prop->string_data);
#endif
}

static void 
stringprop_set_from_widget(StringProperty *prop, WIDGET *widget) 
{
  gchar *val = gtk_editable_get_chars(GTK_EDITABLE(widget),0,-1);
#ifdef GTK_DOESNT_TALK_UTF8_WE_DO
  utfchar *utfbuf = charconv_local8_to_utf8(val);
  g_free(val);
  val = utfbuf;
#elif defined(GTK_TALKS_UTF8_WE_DONT)
  gchar *locbuf = charconv_utf8_to_local8(val);
  g_free(val);
  val = locbuf;
#else
  /* general case */
#endif
  g_free(prop->string_data);
  prop->string_data = val;
}

static WIDGET *
multistringprop_get_widget(StringProperty *prop, PropDialog *dialog) 
{ 
  GtkWidget *ret = gtk_text_new(NULL,NULL);
  gtk_text_set_editable(GTK_TEXT(ret),TRUE);
  prophandler_connect(&prop->common,GTK_OBJECT(ret),"changed");
  return ret;
}

static void 
multistringprop_reset_widget(StringProperty *prop, WIDGET *widget)
{
#ifdef GTK_DOESNT_TALK_UTF8_WE_DO
  gchar *locbuf = charconv_utf8_to_local8(prop->string_data);
  gtk_text_set_point(GTK_TEXT(widget), 0);
  gtk_text_forward_delete(GTK_TEXT(widget),
                          gtk_text_get_length(GTK_TEXT(widget)));
  gtk_text_insert(GTK_TEXT(widget), NULL, NULL, NULL, locbuf,-1);
  g_free(locbuf);
#elif defined(GTK_TALKS_UTF8_WE_DONT)
  utfchar *utfbuf = charconv_local8_to_utf8(prop->string_data);
  gtk_text_set_point(GTK_TEXT(widget), 0);
  gtk_text_forward_delete(GTK_TEXT(widget),
                          gtk_text_get_length(GTK_TEXT(widget)));
  gtk_text_insert(GTK_TEXT(widget), NULL, NULL, NULL, utfbuf,-1);
  g_free(utfbuf);
#else
  gtk_text_insert(GTK_TEXT(widget), NULL, NULL, NULL, prop->string_data,-1);
#endif
}

static WIDGET *
fileprop_get_widget(StringProperty *prop, PropDialog *dialog) 
{ 
  GtkWidget *ret = dia_file_selector_new();
  prophandler_connect(&prop->common,GTK_OBJECT(ret),"FIXME");
  return ret;
}

static void 
fileprop_reset_widget(StringProperty *prop, WIDGET *widget)
{
  dia_file_selector_set_file(DIAFILESELECTOR(widget),prop->string_data);
}

static void 
fileprop_set_from_widget(StringProperty *prop, WIDGET *widget) 
{
  g_free(prop->string_data);
  prop->string_data = 
    g_strdup(dia_file_selector_get_file(DIAFILESELECTOR(widget)));
}

static void 
stringprop_load(StringProperty *prop, AttributeNode attr, DataNode data)
{
  g_free(prop->string_data);
  prop->string_data = data_string(data);
}

static void 
stringprop_save(StringProperty *prop, AttributeNode attr) 
{
  data_add_string(attr, prop->string_data);
}

static void 
stringprop_get_from_offset(StringProperty *prop,
                           void *base, guint offset, guint offset2) 
{
  g_free(prop->string_data);
  prop->string_data = g_strdup(struct_member(base,offset,gchar *));
}

static void 
stringprop_set_from_offset(StringProperty *prop,
                           void *base, guint offset, guint offset2)
{
  g_free(struct_member(base,offset,gchar *));
  struct_member(base,offset,gchar *) = g_strdup(prop->string_data);
}

static const PropertyOps stringprop_ops = {
  (PropertyType_New) stringprop_new,
  (PropertyType_Free) stringprop_free,
  (PropertyType_Copy) stringprop_copy,
  (PropertyType_Load) stringprop_load,
  (PropertyType_Save) stringprop_save,
  (PropertyType_GetWidget) stringprop_get_widget,
  (PropertyType_ResetWidget) stringprop_reset_widget,
  (PropertyType_SetFromWidget) stringprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) stringprop_get_from_offset,
  (PropertyType_SetFromOffset) stringprop_set_from_offset
};

static const PropertyOps multistringprop_ops = {
  (PropertyType_New) multistringprop_new,
  (PropertyType_Free) stringprop_free,
  (PropertyType_Copy) stringprop_copy,
  (PropertyType_Load) stringprop_load,
  (PropertyType_Save) stringprop_save,
  (PropertyType_GetWidget) multistringprop_get_widget,
  (PropertyType_ResetWidget) multistringprop_reset_widget,
  (PropertyType_SetFromWidget) stringprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) stringprop_get_from_offset,
  (PropertyType_SetFromOffset) stringprop_set_from_offset
};

static const PropertyOps fileprop_ops = {
  (PropertyType_New) stringprop_new,
  (PropertyType_Free) stringprop_free,
  (PropertyType_Copy) stringprop_copy,
  (PropertyType_Load) stringprop_load,
  (PropertyType_Save) stringprop_save,
  (PropertyType_GetWidget) fileprop_get_widget,
  (PropertyType_ResetWidget) fileprop_reset_widget,
  (PropertyType_SetFromWidget) fileprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) stringprop_get_from_offset,
  (PropertyType_SetFromOffset) stringprop_set_from_offset
};

/***************************/
/* The TEXT property type. */
/***************************/

static TextProperty *
textprop_new(const PropDescription *pdesc, PropDescToPropPredicate reason)
{
  TextProperty *prop = g_new0(TextProperty,1);
  initialize_property(&prop->common,pdesc,reason);
  prop->text_data = NULL;
  return prop;
}

static TextProperty *
textprop_copy(TextProperty *src) 
{
  TextProperty *prop = 
    (TextProperty *)src->common.ops->new_prop(src->common.descr,
                                               src->common.reason);
  copy_init_property(&prop->common,&src->common);
  if (src->text_data)
    prop->text_data = g_strdup(src->text_data);
  else
    prop->text_data = NULL;
  return prop;
}

static void 
textprop_free(TextProperty *prop) 
{
  g_free(prop->text_data);
  g_free(prop);
}

static void 
textprop_load(TextProperty *prop, AttributeNode attr, DataNode data)
{
  Text *text;
  g_free(prop->text_data);
  text = data_text(data);
  text_get_attributes(text,&prop->attr);
  prop->text_data = text_get_string_copy(text);
  text_destroy(text);
}

static void 
textprop_save(TextProperty *prop, AttributeNode attr) 
{
  Text *text = new_text(prop->text_data,
                        prop->attr.font,
                        prop->attr.height,
                        &prop->attr.position,
                        &prop->attr.color,
                        prop->attr.alignment);
  data_add_text(attr,text);
  text_destroy(text);
}

static void 
textprop_get_from_offset(TextProperty *prop,
                         void *base, guint offset, guint offset2) 
{
  Text *text = struct_member(base,offset,Text *);
  g_free(prop->text_data);
  prop->text_data = text_get_string_copy(text);
  text_get_attributes(text,&prop->attr);
}

static void 
textprop_set_from_offset(TextProperty *prop,
                         void *base, guint offset, guint offset2)
{
  Text *text = struct_member(base,offset,Text *);
  text_set_string(text,prop->text_data);
  text_set_attributes(text,&prop->attr);
}

static const PropertyOps textprop_ops = {
  (PropertyType_New) textprop_new,
  (PropertyType_Free) textprop_free,
  (PropertyType_Copy) textprop_copy,
  (PropertyType_Load) textprop_load,
  (PropertyType_Save) textprop_save,
  (PropertyType_GetWidget) noopprop_get_widget,
  (PropertyType_ResetWidget) noopprop_reset_widget,
  (PropertyType_SetFromWidget) noopprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) textprop_get_from_offset,
  (PropertyType_SetFromOffset) textprop_set_from_offset
};

/* ************************************************************** */ 

void 
prop_text_register(void)
{
  prop_type_register(PROP_TYPE_STRING,&stringprop_ops);
  prop_type_register(PROP_TYPE_MULTISTRING,&multistringprop_ops);
  prop_type_register(PROP_TYPE_FILE,&fileprop_ops);
  prop_type_register(PROP_TYPE_TEXT,&textprop_ops);
}
