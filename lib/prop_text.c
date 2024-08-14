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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include "properties.h"
#include "propinternals.h"
#include "text.h"
#include "group.h"
#include "attributes.h"

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
  g_clear_pointer (&prop->string_data, g_free);
  g_clear_pointer (&prop, g_free);
}

static GtkWidget *
stringprop_get_widget(StringProperty *prop, PropDialog *dialog)
{
  GtkWidget *ret = gtk_entry_new();
  gtk_entry_set_activates_default(GTK_ENTRY(ret), TRUE);
  prophandler_connect(&prop->common, G_OBJECT(ret), "changed");
  return ret;
}

static void
stringprop_reset_widget(StringProperty *prop, GtkWidget *widget)
{
  gtk_entry_set_text(GTK_ENTRY(widget),
                     prop->string_data ? prop->string_data : "");
}

static void
stringprop_set_from_widget(StringProperty *prop, GtkWidget *widget)
{
  g_clear_pointer (&prop->string_data, g_free);
  prop->string_data =
    g_strdup (gtk_entry_get_text (GTK_ENTRY(widget)));
}

static gboolean
multistringprop_handle_key(GtkWidget *wid, GdkEventKey *event)
{
  /* Normal textview doesn't grab return, so to avoid closing the dialog...*/
  /* Actually, this doesn't seem to work -- I guess the dialog closes
   * becore this is called :(
   */
  if (event->keyval == GDK_KEY_Return)
    return TRUE;
  return FALSE;
}

static GtkWidget *
multistringprop_get_widget(StringProperty *prop, PropDialog *dialog)
{
  GtkWidget *ret = gtk_text_view_new();
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ret));
  GtkWidget *frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_container_add(GTK_CONTAINER(frame), ret);
  g_signal_connect(G_OBJECT(ret), "key-release-event",
		   G_CALLBACK(multistringprop_handle_key), NULL);
  gtk_widget_show(ret);
  prophandler_connect(&prop->common, G_OBJECT(buffer), "changed");
  return frame;
}

static void
multistringprop_reset_widget(StringProperty *prop, GtkWidget *widget)
{
  GtkWidget *textview = gtk_bin_get_child(GTK_BIN(widget));
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
  gtk_text_buffer_set_text(buffer,
			   prop->string_data ? prop->string_data : "", -1);
}

static void
multistringprop_set_from_widget(StringProperty *prop, GtkWidget *widget) {
  GtkWidget *textview = gtk_bin_get_child(GTK_BIN(widget));
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);
  g_clear_pointer (&prop->string_data, g_free);
  prop->string_data =
    g_strdup (gtk_text_buffer_get_text (buffer, &start, &end, TRUE));
}


static GtkWidget *
fileprop_get_widget (StringProperty *prop, PropDialog *dialog)
{
  GtkFileFilter *filter = gtk_file_filter_new ();
  GtkWidget *ret = gtk_file_chooser_button_new (_("Choose a file..."),
                                                GTK_FILE_CHOOSER_ACTION_OPEN);

  if (prop->common.descr->extra_data) {
    const char **exts = prop->common.descr->extra_data;

    for (int i = 0; exts[i] != NULL; i++) {
      char *globbed = g_strdup_printf ("*.%s", exts[i]);

      gtk_file_filter_add_pattern (filter, globbed);

      g_free (globbed);
    }
  } else {
    gtk_file_filter_add_pixbuf_formats (filter);
  }

  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (ret), filter);
  prophandler_connect (&prop->common, G_OBJECT (ret), "file-set");

  return ret;
}


static void
fileprop_reset_widget (StringProperty *prop, GtkWidget *widget)
{
  if (prop->string_data) {
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (widget), prop->string_data);
  }
}


static void
fileprop_set_from_widget (StringProperty *prop, GtkWidget *widget)
{
  g_clear_pointer (&prop->string_data, g_free);
  prop->string_data = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));
}


static void
stringprop_load(StringProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
{
  g_clear_pointer (&prop->string_data, g_free);
  prop->string_data = data_string(data, ctx);
  if (prop->string_data == NULL) {
    prop->string_data = g_strdup("");
  }
}

static void
stringprop_save(StringProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  data_add_string(attr, prop->string_data, ctx);
}

static void
stringprop_get_from_offset (StringProperty *prop,
                            void           *base,
                            guint           offset,
                            guint           offset2)
{
  g_clear_pointer (&prop->string_data, g_free);
  prop->string_data = g_strdup (struct_member (base, offset, char *));
}


static void
stringprop_set_from_offset (StringProperty *prop,
                            void           *base,
                            guint           offset,
                            guint           offset2)
{
  g_clear_pointer (&struct_member(base, offset, char *), g_free);
  struct_member (base, offset, char *) = g_strdup (prop->string_data);
}


static int
stringprop_get_data_size(void)
{
  StringProperty prop;
  return sizeof (prop.string_data); /* only the pointer */
}

static StringListProperty *
stringlistprop_new(const PropDescription *pdesc, PropDescToPropPredicate reason)
{
  StringListProperty *prop = g_new0(StringListProperty,1);
  initialize_property(&prop->common,pdesc,reason);
  prop->string_list = NULL;
  return prop;
}

static void
stringlistprop_free(StringListProperty *prop)
{
  g_list_foreach(prop->string_list, (GFunc)g_free, NULL);
  g_list_free(prop->string_list);
  g_clear_pointer (&prop, g_free);
}

static StringListProperty *
stringlistprop_copy(StringListProperty *src)
{
  StringListProperty *prop =
    (StringListProperty *)src->common.ops->new_prop(src->common.descr,
                                                 src->common.reason);
  copy_init_property(&prop->common,&src->common);
  if (src->string_list) {
    GList *tmp;

    for (tmp = src->string_list; tmp != NULL; tmp = tmp->next)
      prop->string_list = g_list_append(prop->string_list, g_strdup(tmp->data));
  }
  else
    prop->string_list = NULL;
  return prop;
}

static void
stringlistprop_load(StringListProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
{
  g_warning("stringlistprop_load not implemented");
}

static void
stringlistprop_save(StringProperty *prop, AttributeNode attr)
{
  g_warning("stringlistprop_save not implemented");
}

static void
stringlistprop_get_from_offset(StringListProperty *prop,
                           void *base, guint offset, guint offset2)
{
  GList *tmp, *lst = prop->string_list;

  g_list_foreach(lst, (GFunc)g_free, NULL);
  g_list_free(lst);
  for (tmp = struct_member(base,offset,GList *); tmp != NULL; tmp = tmp->next)
      lst = g_list_append(lst, g_strdup(tmp->data));
  prop->string_list = lst;
}

static void
stringlistprop_set_from_offset(StringListProperty *prop,
                           void *base, guint offset, guint offset2)
{
  GList *tmp, *lst = struct_member(base,offset,GList *);

  g_list_foreach(lst, (GFunc)g_free, NULL);
  g_list_free(lst);
  for (tmp = prop->string_list; tmp != NULL; tmp = tmp->next)
      lst = g_list_append(lst, g_strdup(tmp->data));
  struct_member(base,offset,GList *) = lst;
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
  (PropertyType_SetFromOffset) stringprop_set_from_offset,
  (PropertyType_GetDataSize) stringprop_get_data_size
};

static const PropertyOps stringlistprop_ops = {
  (PropertyType_New) stringlistprop_new,
  (PropertyType_Free) stringlistprop_free,
  (PropertyType_Copy) stringlistprop_copy,
  (PropertyType_Load) stringlistprop_load,
  (PropertyType_Save) stringlistprop_save,
  (PropertyType_GetWidget) noopprop_get_widget,
  (PropertyType_ResetWidget) noopprop_reset_widget,
  (PropertyType_SetFromWidget) noopprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) stringlistprop_get_from_offset,
  (PropertyType_SetFromOffset) stringlistprop_set_from_offset
};

static const PropertyOps multistringprop_ops = {
  (PropertyType_New) multistringprop_new,
  (PropertyType_Free) stringprop_free,
  (PropertyType_Copy) stringprop_copy,
  (PropertyType_Load) stringprop_load,
  (PropertyType_Save) stringprop_save,
  (PropertyType_GetWidget) multistringprop_get_widget,
  (PropertyType_ResetWidget) multistringprop_reset_widget,
  (PropertyType_SetFromWidget) multistringprop_set_from_widget,

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
  DiaFont *font;
  real height;
  TextProperty *prop = g_new0(TextProperty,1);
  initialize_property(&prop->common,pdesc,reason);
  attributes_get_default_font (&font, &height);
  /* minimum atribute intialization */
  prop->attr.font = g_object_ref (font);
  prop->attr.height = height;
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
textprop_free (TextProperty *prop)
{
  g_clear_object (&prop->attr.font);
  g_clear_pointer (&prop->text_data, g_free);
  g_clear_pointer (&prop, g_free);
}

static void
textprop_load(TextProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
{
  Text *text;
  g_clear_pointer (&prop->text_data, g_free);
  text = data_text(data, ctx);
  text_get_attributes(text,&prop->attr);
  prop->text_data = text_get_string_copy(text);
  text_destroy(text);
}

static void
textprop_save(TextProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  Text *text = new_text(prop->text_data,
                        prop->attr.font,
                        prop->attr.height,
                        &prop->attr.position,
                        &prop->attr.color,
                        prop->attr.alignment);
  data_add_text(attr, text, ctx);
  text_destroy(text);
}

static void
textprop_get_from_offset(TextProperty *prop,
                         void *base, guint offset, guint offset2)
{
  Text *text = struct_member(base,offset,Text *);
  g_clear_pointer (&prop->text_data, g_free);
  prop->text_data = text_get_string_copy(text);
  text_get_attributes(text,&prop->attr);
}

static void
textprop_set_from_offset(TextProperty *prop,
                         void *base, guint offset, guint offset2)
{
  Text *text = struct_member(base,offset,Text *);
  text_set_string(text,prop->text_data);
  if (prop->attr.color.alpha != 0.0) /* HACK */
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
  prop_type_register(PROP_TYPE_STRINGLIST,&stringlistprop_ops);
  prop_type_register(PROP_TYPE_MULTISTRING,&multistringprop_ops);
  prop_type_register(PROP_TYPE_FILE,&fileprop_ops);
  prop_type_register(PROP_TYPE_TEXT,&textprop_ops);
}

/*!
 * Return a diplayable name for the object
 */
gchar *
object_get_displayname (DiaObject* object)
{
  gchar *name = NULL;
  Property *prop = NULL;

  if (!object)
    return g_strdup ("<null>"); /* should not happen */

  if (IS_GROUP(object)) {
    guint num = g_list_length(group_objects(object));
    name = g_strdup_printf (g_dngettext (GETTEXT_PACKAGE,
				"Group with %d object",
				"Group with %d objects", num), num);
  } else if ((prop = object_prop_by_name(object, "name")) != NULL)
    name = g_strdup (((StringProperty *)prop)->string_data);
  else if ((prop = object_prop_by_name(object, "text")) != NULL)
    name = g_strdup (((TextProperty *)prop)->text_data);

  if (!name || (0 == strcmp(name, "")))
    name = g_strdup (object->type->name);

  if (prop)
    prop->ops->free(prop);

  name = g_strdelimit (name, "\n", ' ');

  return name;
}
