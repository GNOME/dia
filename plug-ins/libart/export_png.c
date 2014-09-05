/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * export_png.c: export a diagram to a PNG file.
 * Copyright (C) 2000 James Henstridge
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

#if defined(HAVE_LIBPNG) && defined(HAVE_LIBART)

#include <stdio.h>
#include <png.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "intl.h"
#include "filter.h"
#include "render_libart.h"
#include "dialibartrenderer.h"
#include "message.h"
#include "dialogs.h"

/* ugly, but better tahn crashin on non-interactive use */
#include "../../app/app_procs.h"

/* parses a string of the form "[0-9]*x[0-9]*" and transforms it into
   two long values width and height. */
static void
parse_size(gchar *size, long *width, long *height)
{
  if (size) {
    gchar** array = g_strsplit(size, "x", 3);
    *width  = (array[0])? strtol(array[0], NULL, 10): 0;
    *height = (array[1])? strtol(array[1], NULL, 10): 0;
    g_strfreev(array);
  }
  else {
    *width  = 0;
    *height = 0;
  }
}


/* the dots per centimetre to render this diagram at */
/* this matches the setting `100%' setting in dia. */
#define DPCM 20

/* the height of the band to use when rendering.  Smaller bands mean
 * rendering is slower, but less memory is used.  Setting this to G_MAXINT
 * should get the renderer to use one pass. */
#define BAND_HEIGHT 50

struct png_callback_data {
  DiagramData *data;
  gchar *filename;
  gchar *size;			/* for command line option --size */
};

/* Static data.  When the dialog is not reentrant, you could have all data
   be static.  I don't like it that way, though:)  I only hold static that
   which pertains to the dialog itself (including the aspect ratio, as that 
   is used to connect the two entries). */
static GtkWidget *export_png_dialog;
static GtkSpinButton *export_png_width_entry, *export_png_height_entry;
static GtkWidget *export_png_okay_button, *export_png_cancel_button;
static real export_png_aspect_ratio;

/* The heart of the png exporter.
   Deals with a bit of dialog handling and all the rendering and writing.
   The dialog is not used when dia is non-interactive (export mode)
*/
static void
export_png_ok(GtkButton *button, gpointer userdata) 
{
  struct png_callback_data *cbdata = (struct png_callback_data *)userdata;
  DiagramData *data = cbdata->data;
  DiaRenderer *renderer;
  DiaLibartRenderer *la_renderer;
  Rectangle *ext = &data->extents;
  Rectangle visible;
  guint32 width, height, band, row, i;
  real band_height;
  guint32 imagewidth = 0, imageheight = 0;
  long req_width, req_height;
  real imagezoom;

  FILE *fp;
  png_structp png;
  png_infop info;
  png_color_8 sig_bit;
  png_bytep *row_ptr;

  width  = (guint32) ((ext->right - ext->left) * DPCM * data->paper.scaling + 0.5);
  height = (guint32) ((ext->bottom - ext->top) * DPCM * data->paper.scaling + 0.5);

  if (button != NULL) {
    /* We don't want multiple clicks:) */
    gtk_widget_hide(export_png_dialog);

    imagewidth = gtk_spin_button_get_value_as_int(export_png_width_entry);
    imageheight = gtk_spin_button_get_value_as_int(export_png_height_entry);
  } else {
    if (cbdata && cbdata->size) {
      float ratio = (float) width/(float) height;

      parse_size(cbdata->size, &req_width, &req_height);
      if (req_width && !req_height) {
	imagewidth  = req_width;
	imageheight = req_width / ratio;
      } else if (req_height && !req_width) {
	imagewidth  = req_height * ratio;
	imageheight = req_height;
      } else if (req_width && req_height) {
	imagewidth  = req_width;
	imageheight = req_height;
      }
    } else {
      imagewidth  = width;
      imageheight = height;
    }
  }

  /* Subtract one to ensure all pixels are inside bitmap (see bug #413275 */
  imagezoom = ((real)(imageheight - 1)/height) * DPCM * data->paper.scaling;

  /* we render in bands to try to keep memory consumption down ... */
  band = MIN(imageheight, BAND_HEIGHT);
  band_height = (real)band / imagezoom;

  visible = *ext;
  visible.bottom = MIN(visible.bottom,
		       visible.top + band_height);

  renderer = new_libart_renderer(dia_transform_new (&visible, &imagezoom), 0);
  la_renderer = DIA_LIBART_RENDERER (renderer);
  dia_renderer_set_size(renderer, NULL, imagewidth, band);

  fp = g_fopen(cbdata->filename, "wb");
  if (fp == NULL) {
    message_error(_("Can't open output file %s: %s\n"), cbdata->filename, strerror(errno));
    goto error;
  }

  png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
				NULL, NULL, NULL);
  if (!png) {
    fclose(fp);
    message_error(_("Could not create PNG write structure"));
    goto error;
  }

  /* allocate/initialise the image information data */
  info = png_create_info_struct(png);
  if (!info) {
    fclose(fp);
    png_destroy_write_struct(&png, (png_infopp)NULL);
    message_error(_("Could not create PNG header info structure"));
    goto error;
  }

  /* set error handling ... */
  if (setjmp(png_jmpbuf(png))) {
    fclose(fp);
    png_destroy_write_struct(&png, &info);
    message_error(_("Error occurred while writing PNG"));
    goto error;
  }
  /* the compiler said these may be clobbered by setjmp, so we set it again
   * here. */
  if (button != NULL) {
    imagewidth = gtk_spin_button_get_value_as_int(export_png_width_entry);
    imageheight = gtk_spin_button_get_value_as_int(export_png_height_entry);
  } else {
    if (cbdata && cbdata->size) {
      float ratio = (float) width/(float) height;

      parse_size(cbdata->size, &req_width, &req_height);
      if (req_width && !req_height) {
	imagewidth  = req_width;
	imageheight = req_width / ratio;
      } else if (req_height && !req_width) {
	imagewidth  = req_height * ratio;
	imageheight = req_height;
      } else if (req_width && req_height) {
	imagewidth  = req_width;
	imageheight = req_height;
      }
    } else {
      imagewidth  = width;
      imageheight = height;
    }
  }
  band = MIN(imageheight, BAND_HEIGHT);

  png_init_io(png, fp);

  /* header fields */
  png_set_IHDR(png, info, imagewidth, imageheight, 8,
	       PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
	       PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  sig_bit.red = 8;
  sig_bit.green = 8;
  sig_bit.blue = 8;
  png_set_sBIT(png, info, &sig_bit);

  png_set_pHYs(png, info,
	       imagewidth/width*DPCM*100, 
	       imageheight/height*DPCM*100,
	       PNG_RESOLUTION_METER);

  png_write_info(png, info);
  png_set_shift(png, &sig_bit);
  png_set_packing(png);

  row_ptr = g_new(png_bytep, band);

  for (row = 0; row < imageheight; row += band) {
    /* render band */
    for (i = 0; i < imagewidth*band; i++) {
      la_renderer->rgb_buffer[3*i]   = 0xff * data->bg_color.red;
      la_renderer->rgb_buffer[3*i+1] = 0xff * data->bg_color.green;
      la_renderer->rgb_buffer[3*i+2] = 0xff * data->bg_color.blue;
    }
    data_render(data, renderer, &visible, NULL,NULL);
    /* write rows to png file */
    for (i = 0; i < band; i++)
      row_ptr[i] = la_renderer->rgb_buffer + 3 * i * imagewidth;
    png_write_rows(png, row_ptr, MIN(band, imageheight - row));

    visible.top    += band_height;
    visible.bottom += band_height;
  }
  g_free(row_ptr);
  png_write_end(png, info);
  png_destroy_write_struct(&png, &info);
  fclose(fp);

 error:
  g_object_unref(renderer);
  if (button != NULL) {
    g_signal_handlers_disconnect_matched (G_OBJECT(export_png_okay_button), 
					  G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, userdata);
    g_signal_handlers_disconnect_matched (G_OBJECT(export_png_cancel_button), 
					  G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, userdata);
  }
  g_free(cbdata->filename);
  g_free(cbdata);
  return;
}

/* Stuff to do when cancelling:
   disconnect signals (since the dialog persists)
   hide dialog
   free callback data
*/
static void
export_png_cancel(GtkButton *button, gpointer userdata) 
{
  struct png_callback_data *cbdata = (struct png_callback_data *)userdata;

  g_signal_handlers_disconnect_matched (G_OBJECT(export_png_okay_button), 
					G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, userdata);
  g_signal_handlers_disconnect_matched (G_OBJECT(export_png_cancel_button), 
					G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, userdata);

  gtk_widget_hide(export_png_dialog);
  g_free(cbdata->filename);
  g_free(cbdata);
}

/* Adjust the aspect ratio */
static void
export_png_ratio(GtkAdjustment *limits, gpointer userdata) 
{
  /* This variable makes sure that we don't have a loopback effect. */
  static gboolean in_progress;
  if (in_progress) return;
  in_progress = TRUE;
  if (userdata == export_png_height_entry) {
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(userdata),
			      (int)((real)gtk_spin_button_get_value_as_int(export_png_width_entry))/export_png_aspect_ratio);
  } else {
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(userdata),
			      (int)((real)gtk_spin_button_get_value_as_int(export_png_height_entry))*export_png_aspect_ratio);
  }
  in_progress = FALSE;
}

static gboolean
export_png(DiagramData *data, DiaContext *ctx,
	   const gchar *filename, const gchar *diafilename,
	   void* user_data)
{
  /* Create the callback data.  Can't be stack allocated, as the function
     returns before the callback is called.  Must be freed by the
     final callbacks. */
  struct png_callback_data *cbdata = 
    (struct png_callback_data *) g_new0(struct png_callback_data, 1);
  Rectangle *ext = &data->extents;
  guint32 width, height;

  /* Note that this dialog, while not modal, is no reentrant, as it creates
     a single dialog and uses that every time.  Trying to do two exports at
     the same time will lead to confusion.
  */

  if (export_png_dialog == NULL && user_data == NULL && app_is_interactive()) {
    /* Create a dialog */
    export_png_dialog = dialog_make(_("PNG Export Options"),
				    _("Export"), NULL,
				    &export_png_okay_button,
				    &export_png_cancel_button);
    /* Add two integer entries */
    export_png_width_entry =
      dialog_add_spinbutton(export_png_dialog, _("Image width:"),
			    0.0, 10000.0, 0);
    export_png_height_entry =
      dialog_add_spinbutton(export_png_dialog, _("Image height:"),
			    0.0, 10000.0, 0);

    /* Make sure that the aspect ratio stays the same */
    g_signal_connect(G_OBJECT(gtk_spin_button_get_adjustment(export_png_width_entry)), 
		       "value_changed",
		     G_CALLBACK(export_png_ratio), (gpointer)export_png_height_entry);
    g_signal_connect(G_OBJECT(gtk_spin_button_get_adjustment(export_png_height_entry)), 
		       "value_changed",
		       G_CALLBACK(export_png_ratio), (gpointer)export_png_width_entry);

  }

  /* Store pertinent data in callback data structure */
  cbdata->data = data;
  cbdata->filename = g_strdup(filename);

  if (user_data == NULL && app_is_interactive()) {
    /* Find the default size */
    width  = (guint32) ((ext->right - ext->left) * DPCM * data->paper.scaling);
    height = (guint32) ((ext->bottom - ext->top) * DPCM * data->paper.scaling);

    /* Store aspect ratio */
    export_png_aspect_ratio = ((real)width)/height;
    
    /* Set the default size */
    gtk_spin_button_set_value(export_png_width_entry, (float)width);
    /* This is set from the aspect ratio */
    /*  gtk_spin_button_set_value(export_png_height_entry, (float)height);*/
    
    /* Set OK and Cancel buttons to call the relevant callbacks with cbdata */
    g_signal_connect(G_OBJECT(export_png_okay_button), "clicked",
		     G_CALLBACK(export_png_ok), (gpointer)cbdata);
    g_signal_connect(G_OBJECT(export_png_cancel_button), "clicked",
		     G_CALLBACK(export_png_cancel), (gpointer)cbdata);
    
    /* Show the whole thing */
    gtk_widget_show_all(export_png_dialog);
  } else {
    cbdata->size = (gchar *) user_data;
    export_png_ok(NULL, cbdata);
  }
  return TRUE;
}

static const gchar *extensions[] = { "png", NULL };
DiaExportFilter png_export_filter = {
  N_("PNG (antialiased)"),
  extensions,
  export_png,
  NULL,
  "png-libart"
};

#endif
