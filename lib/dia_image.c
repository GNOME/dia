/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
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
#include <string.h> /* memmove */

#include "intl.h"
#include "geometry.h"
#include "dia_image.h"
#include "message.h"
#include <gdk-pixbuf/gdk-pixbuf.h>

#define SCALING_CACHE

GType dia_image_get_type (void);
#define DIA_TYPE_IMAGE (dia_image_get_type())
#define DIA_IMAGE(object) (G_TYPE_CHECK_INSTANCE_CAST((object), DIA_TYPE_IMAGE, DiaImage))
#define DIA_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_IMAGE, DiaImageClass))

typedef struct _DiaImageClass DiaImageClass;
struct _DiaImageClass {
  GObjectClass parent_class;
};

/*!
 * \brief DiaImage is a thin wrapper around GdkPixbuf
 *
 * _DiaImage provides some copying accessors as well as internal caching of
 * a downscaled variant of the underlying pixbuf. Also there is special handling
 * of 'broken' i.e. typically empty images.
 *
 * _DiaImage can be used to assemble _DiaObjects - it is also part of the
 * _DiaRenderer interface and thus provides interface to all of the exporters.
 *
 * \ingroup ObjectParts
 */
struct _DiaImage {
  GObject parent_instance;
  GdkPixbuf *image;
  gchar *filename;
  gchar *mime_type; /* optional */
#ifdef SCALING_CACHE
  GdkPixbuf *scaled; /* a cache of the last scaled version */
  int scaled_width, scaled_height;
#endif
};

static void dia_image_class_init(DiaImageClass* class);
static void dia_image_finalize(GObject* object);
static void dia_image_init_instance(DiaImage*);

GType
dia_image_get_type (void)
{
    static GType object_type = 0;

    if (!object_type) {
        static const GTypeInfo object_info =
            {
                sizeof (DiaImageClass),
                (GBaseInitFunc) NULL,
                (GBaseFinalizeFunc) NULL,
                (GClassInitFunc) dia_image_class_init, /* class_init */
                NULL,           /* class_finalize */
                NULL,           /* class_data */
                sizeof (DiaImage),
                0,              /* n_preallocs */
                (GInstanceInitFunc)dia_image_init_instance
            };
        object_type = g_type_register_static (G_TYPE_OBJECT,
                                              "DiaImage",
                                              &object_info, 0);
    }
    return object_type;
}

static gpointer parent_class;

static void
dia_image_class_init(DiaImageClass* klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  parent_class = g_type_class_peek_parent(klass);
  object_class->finalize = dia_image_finalize;
}

/*!
 * \brief Constructor
 * \memberof _DiaImage
 */
static void
dia_image_init_instance(DiaImage *image)
{
  /* GObject *gobject = G_OBJECT(image);  */
  /* zero intialization should be good for us */
}

/*!
 * \brief Destructor
 * \memberof _DiaImage
 */
static void
dia_image_finalize(GObject* object)
{
  DiaImage *image = DIA_IMAGE(object);
#ifdef SCALING_CACHE
  if (image->scaled)
    g_object_unref (image->scaled);
  image->scaled = NULL;
#endif
  if (image->image)
    g_object_unref (image->image);
  image->image = NULL;
  g_free (image->filename);
  image->filename = NULL;
  g_free (image->mime_type);
  image->mime_type = NULL;
}

/*!
 * \brief Constructor of a 'broken' image
 * Get the image to put in place of a image that cannot be read.
 * @return A statically allocated image.
 * \memberof _DiaImage
 */
DiaImage *
dia_image_get_broken(void)
{
  static GdkPixbuf *broken = NULL;
  DiaImage *image;

  image = DIA_IMAGE(g_object_new(DIA_TYPE_IMAGE, NULL));
  if (broken == NULL) {
    broken = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                       "image-missing",
                                       64,
                                       0, NULL);
  }
  image->image = g_object_ref (broken);
  /* Kinda hard to export :) */
  image->filename = g_strdup("<broken>");
#ifdef SCALING_CACHE
  image->scaled = NULL;
#endif
  return image;
}

/*!
 * \brief Load an image from file.
 * @param filename Name of the file to load.
 * @return An image loaded from file, or NULL if an error occurred.
 *          Error messages will be displayed to the user.
 * \memberof _DiaImage
 */
DiaImage *
dia_image_load(const gchar *filename)
{
  DiaImage *dia_img;
  GdkPixbuf *image;
  GError *error = NULL;

  image = gdk_pixbuf_new_from_file(filename, &error);
  if (image == NULL) {
    /* dia_image_load() function is also (mis)used to check file
     * existence. Don't warn if the file is simply not there but
     * only if there is something else wrong while loading it.
     */
    if (g_file_test(filename, G_FILE_TEST_EXISTS))
      message_warning ("%s\n", error->message);
    g_error_free (error);
    return NULL;
  }

  dia_img = DIA_IMAGE(g_object_new(DIA_TYPE_IMAGE, NULL));
  dia_img->image = image;
  dia_img->filename = g_strdup(filename);
  /* the pixbuf does not know anymore where it came from */
  {
    GdkPixbufFormat *format = gdk_pixbuf_get_file_info (filename, NULL, NULL);
    gchar **mime_types = gdk_pixbuf_format_get_mime_types (format);
    dia_img->mime_type = g_strdup (mime_types[0]);
    g_strfreev (mime_types);
  }
#ifdef SCALING_CACHE
  dia_img->scaled = NULL;
#endif
  return dia_img;
}

/*!
 * \brief Create a Dia Image from in memory GdkPixbuf
 *
 * It stores only a reference, so drop your own after calling this.
 * Different _DiaImage can share the same underlying pixbuf,
 * _DiaImage is considered immutable.
 * \memberof _DiaImage
 */
DiaImage *
dia_image_new_from_pixbuf (GdkPixbuf *pixbuf)
{
  DiaImage *dia_img;
  const gchar *mime_type;

  dia_img = DIA_IMAGE(g_object_new(DIA_TYPE_IMAGE, NULL));
  dia_img->image = g_object_ref (pixbuf);
  mime_type = g_object_get_data (G_OBJECT (pixbuf), "mime-type");
  if (mime_type)
    dia_img->mime_type = g_strdup (mime_type);

  return dia_img;
}

/** Reference an image.
 * @param image Image that we want a reference to.
 */
void
dia_image_add_ref(DiaImage *image)
{
  g_object_ref(image);
}

/** Release a reference to an image.
 * @param image Image to unreference.
 */
void
dia_image_unref(DiaImage *image)
{
  g_object_unref(image);
}

/*!
 * \brief Create a scaled variant of the underlying pixbuf.
 * @param image explicit this pointer
 * @param width Width in pixels of result.
 * @param height Height in pixels of result.
 * \memberof _DiaImage
 */
GdkPixbuf *
dia_image_get_scaled_pixbuf(DiaImage *image, int width, int height)
{
  GdkPixbuf *scaled;

  if (width < 1 || height < 1)
    return NULL;
  if (gdk_pixbuf_get_width(image->image) > width ||
      gdk_pixbuf_get_height(image->image) > height) {
    /* Using TILES to make it look more like PostScript */
#ifdef SCALING_CACHE
    if (image->scaled == NULL ||
	image->scaled_width != width || image->scaled_height != height) {
      if (image->scaled)
	g_object_unref(image->scaled);
      image->scaled = gdk_pixbuf_scale_simple(image->image, width, height,
	/* dont waste interpolation time if it wont be seen anyway */
	(width * height > 256) ? GDK_INTERP_TILES : GDK_INTERP_NEAREST);
      image->scaled_width = width;
      image->scaled_height = height;
    }
    scaled = image->scaled;
#else
    scaled = gdk_pixbuf_scale_simple(image->image, width, height,
				     GDK_INTERP_TILES);
#endif
  } else {
    scaled = image->image;
  }
  /* always adding a reference */
  return g_object_ref (scaled);
}

static gchar *
_guess_format (const gchar *filename)
{
  const char* test = strrchr (filename, '.');
  GSList* formats = gdk_pixbuf_get_formats ();
  GSList* sl;
  gchar *type = NULL;

  if (test)
    ++test;
  else
    test = "png";

  for (sl = formats; sl != NULL; sl = g_slist_next (sl)) {
    GdkPixbufFormat* format = (GdkPixbufFormat*)sl->data;

    if (gdk_pixbuf_format_is_writable (format)) {
      gchar* name = gdk_pixbuf_format_get_name (format);
      gchar **extensions = gdk_pixbuf_format_get_extensions (format);
      int i;

      for (i = 0; extensions[i] != NULL; ++i) {
        const gchar *ext = extensions[i];
        if (strcmp (test, ext) == 0) {
	  type = g_strdup (name);
	  break;
	}
      }
      g_strfreev (extensions);
    }
    if (type)
      break;
  }
  g_slist_free (formats);
  return type;
}

/*!
 * \brief Save an image under the given filename
 * If saving was successful the internal filename is updated to the
 * given one. Otherwise FALSE will be returned.
 * \memberof _DiaImage
 */
gboolean
dia_image_save(DiaImage *image, const gchar *filename)
{
  gboolean saved = FALSE;

  if (image->image) {
    GError *error = NULL;
    gchar *type = _guess_format (filename);

    if (type) /* XXX: consider image->mime_type */
      saved = gdk_pixbuf_save (image->image, filename, type, &error, NULL);
    if (saved) {
      g_free (image->filename);
      image->filename = g_strdup (filename);
    } else if (!type) {
      /* pathologic case - pixbuf not even supporting PNG? */
      message_error(_("Unsupported file format for saving:\n%s\n"),
                    dia_message_filename(filename));
    } else {
      message_warning(_("Could not save file:\n%s\n%s\n"),
		      dia_message_filename(filename),
                      error->message);
      g_error_free (error);
    }

    g_free (type);
  }
  return saved;
}

/*!
 * \brief Get the width of an image.
 * @param image An image object
 * @return The natural width of the object in pixels.
 * \memberof _DiaImage
 */
int
dia_image_width(const DiaImage *image)
{
  g_return_val_if_fail (image != NULL, 0);
  return gdk_pixbuf_get_width(image->image);
}

/*!
 * \brief Get the height of an image.
 * @param image An image object
 * @return The natural height of the object in pixels.
 * \memberof _DiaImage
 */
int
dia_image_height(const DiaImage *image)
{
  g_return_val_if_fail (image != NULL, 0);
  return gdk_pixbuf_get_height(image->image);
}

/*!
 * \brief Get the rowstride number of bytes per row, see gdk_pixbuf_get_rowstride.
 * @param image An image object
 * @return The rowstride number of the image.
 * \memberof _DiaImage
 */
int
dia_image_rowstride(const DiaImage *image)
{
  g_return_val_if_fail (image != NULL, 0);
  return gdk_pixbuf_get_rowstride(image->image);
}
/*!
 * \brief Direct const access to the underlying GdkPixbuf
 * @param image An image object
 * @return The pixbuf
 * \memberof _DiaImage
 */
const GdkPixbuf*
dia_image_pixbuf (const DiaImage *image)
{
  if (!image)
    return NULL;
  return image->image;
}

/*!
 * \brief Get the mime-type of the image
 * @param image An image object
 * @return The mime type from creation or "image/png" as fallback
 * \memberof _DiaImage
 */
const gchar *
dia_image_get_mime_type (const DiaImage *image)
{
  if (image->mime_type)
    return image->mime_type;
  return "image/png";
}
/*!
 * \brief Set the mime-type for the image
 * @param image An image object
 * @param mime_type A string like "image/jpeg"
 * @return The mime type from creation or "image/png" as fallback
 * \memberof _DiaImage
 */
void
dia_image_set_mime_type (DiaImage *image, const gchar *mime_type)
{
  g_free (image->mime_type);
  image->mime_type = g_strdup (mime_type);
}

/*!
 * \brief Get the raw RGB data from an image.
 * @param image An image object.
 * @return An array of bytes (height*rowstride) containing the RGB data
 *          This array should be freed after use.
 * \memberof _DiaImage
 */
guint8 *
dia_image_rgb_data(const DiaImage *image)
{
  int width = dia_image_width(image);
  int height = dia_image_height(image);
  int rowstride = dia_image_rowstride(image);
  int size = height*rowstride;
  guint8 *rgb_pixels = g_try_malloc(size);

  if (!rgb_pixels)
    return NULL;

  g_return_val_if_fail (image != NULL, NULL);
  if (gdk_pixbuf_get_has_alpha(image->image)) {
    guint8 *pixels = gdk_pixbuf_get_pixels(image->image);
    int i, j;
    for (i = 0; i < height; i++) {
      for (j = 0; j < width; j++) {
	rgb_pixels[i*rowstride+j*3] = pixels[i*rowstride+j*4];
	rgb_pixels[i*rowstride+j*3+1] = pixels[i*rowstride+j*4+1];
	rgb_pixels[i*rowstride+j*3+2] = pixels[i*rowstride+j*4+2];
      }
    }
    return rgb_pixels;
  } else {
    guint8 *pixels = gdk_pixbuf_get_pixels(image->image);

    g_memmove(rgb_pixels, pixels, height*rowstride);
    return rgb_pixels;
  }
}

/*!
 * \brief Get the mask data for an image.
 * @param image An image object.
 * @return An array of bytes (width*height) with the alpha channel information
 *         from the image.  This array should be freed after use.
 * \memberof _DiaImage
 */
guint8 *
dia_image_mask_data(const DiaImage *image)
{
  guint8 *pixels;
  guint8 *mask;
  int i, size;

  if (!gdk_pixbuf_get_has_alpha(image->image)) {
    return NULL;
  }

  pixels = gdk_pixbuf_get_pixels(image->image);

  size = gdk_pixbuf_get_width(image->image)*
    gdk_pixbuf_get_height(image->image);

  mask = g_try_malloc(size);
  if (!mask)
    return NULL;

  /* Pick every fourth byte (the alpha channel) into mask */
  for (i = 0; i < size; i++)
    mask[i] = pixels[i*4+3];

  return mask;
}

/*!
 * \brief Get full RGBA data from an image.
 * @param image An image object.
 * @return An array of pixels as delivered by gdk_pixbuf_get_pixels, or
 *          NULL if the image has no alpha channel.
 * \memberof _DiaImage
 */
const guint8 *
dia_image_rgba_data(const DiaImage *image)
{
  g_return_val_if_fail (image != NULL, 0);
  if (gdk_pixbuf_get_has_alpha(image->image)) {
    const guint8 *pixels = gdk_pixbuf_get_pixels(image->image);

    return pixels;
  } else {
    return NULL;
  }
}

/*!
 * \brief Return the filename associated with an image.
 * @param image An image object
 * @return The filename associated with an image, or "(null)" if the image
 *         has no filename.  This string should *not* be freed by the caller.
 * \memberof _DiaImage
 */
const char *
dia_image_filename(const DiaImage *image)
{
  if (!image->filename)
    return "(null)";
  return image->filename;
}
