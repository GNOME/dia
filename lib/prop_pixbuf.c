/* Dia -- a diagram creation/manipulation program -*- c -*-
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 *
 * Copyright (C) 2010 Hans Breuer
 * Property types for pixbuf.
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

#include <gtk/gtk.h>
#define WIDGET GtkWidget
#include "widgets.h"
#include "properties.h"
#include "propinternals.h"
#include "message.h"

static PixbufProperty *
pixbufprop_new(const PropDescription *pdesc, PropDescToPropPredicate reason)
{
  PixbufProperty *prop = g_new0(PixbufProperty,1);

  initialize_property(&prop->common, pdesc, reason);
  /* empty by default */
  prop->pixbuf = NULL;

  return prop;
}

static void
pixbufprop_free(PixbufProperty *prop) 
{
  if (prop->pixbuf)
    g_object_unref(prop->pixbuf);
  g_free(prop);
} 

static PixbufProperty *
pixbufprop_copy(PixbufProperty *src) 
{
  PixbufProperty *prop = 
    (PixbufProperty *)src->common.ops->new_prop(src->common.descr,
                                              src->common.reason);
  if (src->pixbuf) /* TODO: rething on edit - gdk_pixbuf_copy() ? */
    prop->pixbuf = g_object_ref (src->pixbuf);

  return prop;
}

GdkPixbuf *
data_pixbuf (DataNode data)
{
  GdkPixbuf *pixbuf = NULL;
  GdkPixbufLoader *loader;
  GError *error = NULL;
  AttributeNode attr = composite_find_attribute(data, "data");

  loader = gdk_pixbuf_loader_new ();
  if (loader) {
    xmlNode *node = attribute_first_data (attr);
    gint state = 0;
    guint save = 0;
#  define BUF_SIZE 4096
    guchar buf[BUF_SIZE];
    gchar *in = NULL; /* direct access, not involving another xmlStrDup/xmlFree */
    gssize len = 0;

    if (node->children && xmlStrcmp (node->children->name, (const xmlChar*)"text") == 0) {
      in = (gchar *)node->children->content;
      len = strlen ((char *)in);
    }

    do {
      gsize step = g_base64_decode_step (in,
					 len > BUF_SIZE ? BUF_SIZE : len,
					 buf, &state, &save);

      if (!gdk_pixbuf_loader_write (loader, buf, step, &error))
	break;

      in += BUF_SIZE;
      len -= BUF_SIZE;
    } while (len > 0);
    if (gdk_pixbuf_loader_close (loader, error ? NULL : &error)) {
      pixbuf = g_object_ref (gdk_pixbuf_loader_get_pixbuf (loader));
    } else {
      message_warning (_("Failed to load image form diagram:\n%s"), error->message);
      g_error_free (error);
    }

    g_object_unref (loader);
  }
#  undef BUF_SIZE
  return pixbuf;
}

static void 
pixbufprop_load(PixbufProperty *prop, AttributeNode attr, DataNode data)
{
  prop->pixbuf = data_pixbuf (data);
}

typedef struct _EncodeData {
  GByteArray *array;
  
  gsize size;

  gint state;
  gint save;
} EncodeData;
static gboolean
_pixbuf_encode (const gchar *buf,
		gsize count,
		GError **error,
		gpointer data)
{
  EncodeData *ed = data;
  guint old_len;
  gsize growth = (count / 3 + 1) * 4 + 4 + ((count / 3 + 1) * 4 + 4) / 72 + 1;
  gchar *out;

  old_len = ed->array->len;
  g_byte_array_set_size (ed->array, ed->size + growth);
  out = (gchar *)&ed->array->data[ed->size];
  ed->size += g_base64_encode_step ((guchar *)buf, count, TRUE, 
                                    out, &ed->state, &ed->save);

  return TRUE;
}
void
data_add_pixbuf (AttributeNode attr, GdkPixbuf *pixbuf)
{
  ObjectNode composite = data_add_composite(attr, "pixbuf");
  AttributeNode comp_attr = composite_add_attribute (composite, "data");
  GError *error = NULL;
  EncodeData ed = { 0, };

  ed.array = g_byte_array_new ();

  if (!gdk_pixbuf_save_to_callback (pixbuf, _pixbuf_encode, &ed, "png", &error, NULL)) {
    message_error (_("Saving inline pixbuf failed:\n%s"), error->message);
    g_error_free (error);
    return;
  }
  /* g_base64_encode_close ... [needs] up to 5 bytes if line-breaking is enabled */
  /* also make the array 0-terminated */
  g_byte_array_append (ed.array, "\0\0\0\0\0", 6);
  ed.size += g_base64_encode_close (TRUE, (gchar *)&ed.array->data[ed.size], 
				    &ed.state, &ed.save);

  (void)xmlNewChild (comp_attr, NULL, (const xmlChar *)"data", ed.array->data);

  g_byte_array_free (ed.array, TRUE);
}

static void 
pixbufprop_save(PixbufProperty *prop, AttributeNode attr) 
{
  if (prop->pixbuf) {
    data_add_pixbuf (attr, prop->pixbuf);
  }
}

static void 
pixbufprop_get_from_offset(PixbufProperty *prop,
                         void *base, guint offset, guint offset2) 
{
  /* before we start editing a simple refernce should be enough */
  GdkPixbuf *pixbuf = struct_member(base,offset,GdkPixbuf *);

  if (pixbuf)
    prop->pixbuf = g_object_ref (pixbuf);
  else
    prop->pixbuf = NULL;
}

static void 
pixbufprop_set_from_offset(PixbufProperty *prop,
                           void *base, guint offset, guint offset2)
{
  GdkPixbuf *dest = struct_member(base,offset,GdkPixbuf *);
  if (dest)
    g_object_unref (dest);
  if (prop->pixbuf)
    struct_member(base,offset, GdkPixbuf *) = g_object_ref (prop->pixbuf);
  else
    struct_member(base,offset, GdkPixbuf *) = NULL;
}

/* GUI stuff - not yet 
   - allow to crop
   - maybe scale
 */
static void
_pixbuf_toggled(GtkWidget *wid)
{
  if (GTK_TOGGLE_BUTTON(wid)->active)
    gtk_label_set_text(GTK_LABEL(GTK_BIN(wid)->child), _("Yes"));
  else
    gtk_label_set_text(GTK_LABEL(GTK_BIN(wid)->child), _("No"));
}

static GtkWidget *
pixbufprop_get_widget (PixbufProperty *prop, PropDialog *dialog) 
{ 
  GtkWidget *ret = gtk_toggle_button_new_with_label(_("No"));
  g_signal_connect(G_OBJECT(ret), "toggled",
                   G_CALLBACK (_pixbuf_toggled), NULL);
  prophandler_connect(&prop->common, G_OBJECT(ret), "toggled");
  return ret;
}

static void 
pixbufprop_reset_widget(PixbufProperty *prop, GtkWidget *widget)
{
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),prop->pixbuf != NULL);
}

static void 
pixbufprop_set_from_widget(PixbufProperty *prop, GtkWidget *widget) 
{
  if (GTK_TOGGLE_BUTTON(widget)->active) {
    if (!prop->pixbuf)
      message_warning (_("Cant create image data from scratch!"));
  } else {
    if (prop->pixbuf)
      g_object_unref (prop->pixbuf);
    prop->pixbuf = NULL;
  }
}

static const PropertyOps pixbufprop_ops = {
  (PropertyType_New) pixbufprop_new,
  (PropertyType_Free) pixbufprop_free,
  (PropertyType_Copy) pixbufprop_copy,
  (PropertyType_Load) pixbufprop_load,
  (PropertyType_Save) pixbufprop_save,
  (PropertyType_GetWidget) pixbufprop_get_widget,
  (PropertyType_ResetWidget) pixbufprop_reset_widget,
  (PropertyType_SetFromWidget) pixbufprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) pixbufprop_get_from_offset,
  (PropertyType_SetFromOffset) pixbufprop_set_from_offset
};

void 
prop_pixbuftypes_register(void)
{
  prop_type_register(PROP_TYPE_PIXBUF, &pixbufprop_ops);
}
