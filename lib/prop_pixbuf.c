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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>
#define WIDGET GtkWidget
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
  g_clear_object (&prop->pixbuf);
  g_clear_pointer (&prop, g_free);
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


/**
 * pixbuf_decode_base64:
 * @b64: Base64 encoded data
 *
 * Convert Base64 to pixbuf
 */
GdkPixbuf *
pixbuf_decode_base64 (const char *b64)
{
  /* see lib/prop_pixbuf.c(data_pixbuf) for a very similar implementation */
  GdkPixbuf *pixbuf = NULL;
  GdkPixbufLoader *loader;
  GError *error = NULL;

  loader = gdk_pixbuf_loader_new ();
  if (loader) {
    int state = 0;
    guint save = 0;
#   define BUF_SIZE 4096
    guchar buf[BUF_SIZE];
    char *in = (gchar *)b64; /* direct access, not involving another xmlStrDup/xmlFree */
    gssize len = strlen (b64);

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
      GdkPixbufFormat *format = gdk_pixbuf_loader_get_format (loader);
      char  *format_name = gdk_pixbuf_format_get_name (format);
      char **mime_types = gdk_pixbuf_format_get_mime_types (format);

      dia_log_message ("Loaded pixbuf from '%s' with '%s'", format_name, mime_types[0]);
      pixbuf = g_object_ref (gdk_pixbuf_loader_get_pixbuf (loader));
      /* attach the mime-type to the pixbuf */
      g_object_set_data_full (G_OBJECT (pixbuf), "mime-type",
                              g_strdup (mime_types[0]),
                              (GDestroyNotify) g_free);
      g_strfreev (mime_types);
      g_clear_pointer (&format_name, g_free);
    } else {
      message_warning (_("Failed to load image form diagram:\n%s"), error->message);
      g_clear_error (&error);
    }

    g_clear_object (&loader);
  }
  return pixbuf;
# undef BUF_SIZE
}


GdkPixbuf *
data_pixbuf (DataNode data, DiaContext *ctx)
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
      g_clear_error (&error);
    }

    g_clear_object (&loader);
  }
#  undef BUF_SIZE
  return pixbuf;
}

static void
pixbufprop_load(PixbufProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
{
  prop->pixbuf = data_pixbuf (data, ctx);
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
  gsize growth = (count / 3 + 1) * 4 + 4 + ((count / 3 + 1) * 4 + 4) / 72 + 1;
  gchar *out;

  g_byte_array_set_size (ed->array, ed->size + growth);
  out = (gchar *)&ed->array->data[ed->size];
  ed->size += g_base64_encode_step ((guchar *)buf, count, TRUE,
                                    out, &ed->state, &ed->save);

  return TRUE;
}


static const char *
_make_pixbuf_type_name (const char *p)
{
  if (p && strstr (p, "image/jpeg")) {
    return "jpeg";
  }
  if (p && strstr (p, "image/jp2")) {
    return "jpeg2000";
  }
  return "png";
}


/**
 * pixbuf_encode_base64:
 * @pixbuf: the #GdkPixbuf to encode
 * @prefix: mime type
 *
 * Reusable variant of pixbuf to base64 string conversion
 */
char *
pixbuf_encode_base64 (const GdkPixbuf *pixbuf, const char *prefix)
{
  GError *error = NULL;
  EncodeData ed = { 0, };
  const gchar *type = _make_pixbuf_type_name (prefix);
  ed.array = g_byte_array_new ();

  if (prefix) {
    ed.size = strlen (prefix);
    g_byte_array_append (ed.array, (guint8 *)prefix, ed.size);
  }

  if (!gdk_pixbuf_save_to_callback ((GdkPixbuf *)pixbuf, _pixbuf_encode, &ed, type, &error, NULL)) {
    message_error (_("Saving inline pixbuf failed:\n%s"), error->message);
    g_clear_error (&error);
    return NULL;
  }

  /* g_base64_encode_close ... [needs] up to 5 bytes if line-breaking is enabled */
  /* also make the array 0-terminated */
  g_byte_array_append (ed.array, (guint8 *)"\0\0\0\0\0", 6);
  ed.size += g_base64_encode_close (FALSE, (gchar *)&ed.array->data[ed.size],
				    &ed.state, &ed.save);
  ed.array->data[ed.size] = '\0';

  return (gchar *)g_byte_array_free (ed.array, FALSE);
}
void
data_add_pixbuf (AttributeNode attr, GdkPixbuf *pixbuf, DiaContext *ctx)
{
  ObjectNode composite = data_add_composite(attr, "pixbuf", ctx);
  AttributeNode comp_attr = composite_add_attribute (composite, "data");
  gchar *b64;

  b64 = pixbuf_encode_base64 (pixbuf, NULL);

  if (b64)
    (void)xmlNewChild (comp_attr, NULL, (const xmlChar *)"data", (xmlChar *)b64);

  g_clear_pointer (&b64, g_free);
}

static void
pixbufprop_save(PixbufProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  if (prop->pixbuf) {
    data_add_pixbuf (attr, prop->pixbuf, ctx);
  }
}

static void
pixbufprop_get_from_offset(PixbufProperty *prop,
                         void *base, guint offset, guint offset2)
{
  /* before we start editing a simple reference should be enough */
  GdkPixbuf *pixbuf = struct_member(base,offset,GdkPixbuf *);

  if (pixbuf)
    prop->pixbuf = g_object_ref (pixbuf);
  else
    prop->pixbuf = NULL;
}


static void
pixbufprop_set_from_offset (PixbufProperty *prop,
                            void           *base,
                            guint           offset,
                            guint           offset2)
{
  GdkPixbuf *dest = struct_member (base, offset, GdkPixbuf *);

  if (dest == prop->pixbuf) {
    return;
  }

  g_clear_object (&dest);

  if (prop->pixbuf) {
    struct_member (base, offset, GdkPixbuf *) = g_object_ref (prop->pixbuf);
  } else {
    struct_member (base, offset, GdkPixbuf *) = NULL;
  }
}


/* GUI stuff - not yet
   - allow to crop
   - maybe scale
 */
static void
_pixbuf_toggled(GtkWidget *wid)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(wid)))
    gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(wid))), _("Yes"));
  else
    gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(wid))), _("No"));
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
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(widget))) {
    if (!prop->pixbuf) {
      message_warning (_("Cant create image data from scratch!"));
    }
  } else {
    g_clear_object (&prop->pixbuf);
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
