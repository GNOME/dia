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

#include "config.h"

#if defined(HAVE_LIBPNG) && defined(HAVE_LIBART)

#include <stdio.h>
#include <png.h>

#include "intl.h"
#include "filter.h"
#include "render_libart.h"
#include "display.h"

/* the dots per centimetre to render this diagram at */
/* this matches the setting `100%' setting in dia. */
#define DPCM 20

/* the height of the band to use when rendering.  Smaller bands mean
 * rendering is slower, but less memory is used.  Setting this to G_MAXINT
 * should get the renderer to use one pass. */
#define BAND_HEIGHT 50

static void
export_png(DiagramData *data, const gchar *filename, const gchar *diafilename)
{
  Rectangle *ext = &data->extents;
  DDisplay *ddisp;
  RendererLibart *renderer;
  guint32 width, height, band, row, i;
  real band_height;

  FILE *fp;
  png_structp png;
  png_infop info;
  png_color_8 sig_bit;
  png_bytep *row_ptr;

  width  = (guint32) ((ext->right - ext->left) * DPCM * data->paper.scaling);
  height = (guint32) ((ext->bottom - ext->top) * DPCM * data->paper.scaling);

  /* we render in bands to try to keep memory consumption down ... */
  band = MIN(height, BAND_HEIGHT);
  band_height = (real)band / (DPCM * data->paper.scaling);

  /* create a fake ddisp to keep the renderer happy */
  ddisp = g_new0(DDisplay, 1);
  ddisp->zoom_factor = DPCM * data->paper.scaling;
  ddisp->visible = *ext;
  ddisp->visible.bottom = MIN(ddisp->visible.bottom,
			      ddisp->visible.top + band_height);
  renderer = new_libart_renderer(ddisp, 0);
  libart_renderer_set_size(renderer, NULL, width, band);
  ddisp->renderer = (Renderer *)renderer;

  fp = fopen(filename, "wb");
  if (fp == NULL) {
    message_error(_("Couldn't open: '%s' for writing.\n"), filename);
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
  if (setjmp(png->jmpbuf)) {
    fclose(fp);
    png_destroy_write_struct(&png, (png_infopp)NULL);
    message_error(_("Error occurred while writing PNG"));
    goto error;
  }
  /* the compiler said band may be clobbered by setjmp, so we set it again
   * here. */
  band = MIN(height, BAND_HEIGHT);

  png_init_io(png, fp);

  /* header fields */
  png_set_IHDR(png, info, width, height, 8,
	       PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
	       PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  sig_bit.red = 8;
  sig_bit.green = 8;
  sig_bit.blue = 8;
  png_set_sBIT(png, info, &sig_bit);

  png_write_info(png, info);
  png_set_shift(png, &sig_bit);
  png_set_packing(png);

  row_ptr = g_new(png_bytep, band);

  for (row = 0; row < height; row += band) {
    /* render band */
    for (i = 0; i < width*band; i++) {
      renderer->rgb_buffer[3*i]   = 0xff * data->bg_color.red;
      renderer->rgb_buffer[3*i+1] = 0xff * data->bg_color.green;
      renderer->rgb_buffer[3*i+2] = 0xff * data->bg_color.blue;
    }
    data_render(data, (Renderer *)renderer, &ddisp->visible, NULL,NULL);
    /* write rows to png file */
    for (i = 0; i < band; i++)
      row_ptr[i] = renderer->rgb_buffer + 3 * i * width;
    png_write_rows(png, row_ptr, MIN(band, height - row));

    ddisp->visible.top    += band_height;
    ddisp->visible.bottom += band_height;
  }
  g_free(row_ptr);
  png_write_end(png, info);
  png_destroy_write_struct(&png, (png_infopp)NULL);
  fclose(fp);

 error:
  destroy_libart_renderer(renderer);
  g_free(ddisp);
  return;
}

static const gchar *extensions[] = { "png", NULL };
DiaExportFilter png_export_filter = {
  N_("Portable Network Graphics"),
  extensions,
  export_png
};

#endif
