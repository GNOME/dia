/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
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

#pragma once

#include "diatypes.h"
#include <glib-object.h>
#include <glib.h>

#include "dia-enums.h"

#include "diagramdata.h"
#include "diarenderer.h"

G_BEGIN_DECLS

#define DIA_TYPE_INTERACTIVE_RENDERER dia_interactive_renderer_get_type ()
G_DECLARE_INTERFACE (DiaInteractiveRenderer, dia_interactive_renderer, DIA, INTERACTIVE_RENDERER, DiaRenderer)


/**
 * DiaInteractiveRendererInterface:
 * @get_width_pixels: return width in pixels
 * @get_height_pixels: return height in pixels
 * @set_size: set the output size
 * @clip_region_clear: Clear the current clipping region
 * @clip_region_add_rect: Add a rectangle to the current clipping region
 * @draw_pixel_line: Draw a line from start to end, using color and the
 *                   current line style
 * @draw_pixel_rect: Draw a rectangle, given its upper-left and lower-right
 *                   corners in pixels
 * @fill_pixel_rect: Fill a rectangle, given its upper-left and lower-right
 *                   corners in pixels
 * @paint: Copy already rendered content to the given context
 * @draw_object_highlighted: Support for drawing selected objects highlighted
 * @set_selection: Set the current selection box
 */
struct _DiaInteractiveRendererInterface
{
  /* < private > */
  GTypeInterface base_iface;

  /* < public > */
  int  (*get_width_pixels)        (DiaInteractiveRenderer *self);
  int  (*get_height_pixels)       (DiaInteractiveRenderer *self);
  void (*set_size)                (DiaInteractiveRenderer *self,
                                   gpointer                window,
                                   int                     width,
                                   int                     height);
  void (*clip_region_clear)       (DiaInteractiveRenderer *self);
  void (*clip_region_add_rect)    (DiaInteractiveRenderer *self,
                                   DiaRectangle           *rect);
  void (*draw_pixel_line)         (DiaInteractiveRenderer *self,
                                   int                     x1,
                                   int                     y1,
                                   int                     x2,
                                   int                     y2,
                                   Color                  *color);
  void (*draw_pixel_rect)         (DiaInteractiveRenderer *self,
                                   int                     x,
                                   int                     y,
                                   int                     width,
                                   int                     height,
                                   Color                  *color);
  void (*fill_pixel_rect)         (DiaInteractiveRenderer *self,
                                   int                     x,
                                   int                     y,
                                   int                     width,
                                   int                     height,
                                   Color                  *color);
  void (*paint)                   (DiaInteractiveRenderer *self,
                                   cairo_t                *ctx,
                                   int                     width,
                                   int                     height);
  void (*draw_object_highlighted) (DiaInteractiveRenderer *self,
                                   DiaObject              *object,
                                   DiaHighlightType        type);
  void (*set_selection)           (DiaInteractiveRenderer *self,
                                   gboolean                has_selection,
                                   double                  x,
                                   double                  y,
                                   double                  width,
                                   double                  height);
};


int  dia_interactive_renderer_get_width_pixels        (DiaInteractiveRenderer *self);
int  dia_interactive_renderer_get_height_pixels       (DiaInteractiveRenderer *self);
void dia_interactive_renderer_set_size                (DiaInteractiveRenderer *self,
                                                       gpointer                window,
                                                       int                     width,
                                                       int                     height);
void dia_interactive_renderer_clip_region_clear       (DiaInteractiveRenderer *self);
void dia_interactive_renderer_clip_region_add_rect    (DiaInteractiveRenderer *self,
                                                       DiaRectangle           *rect);
void dia_interactive_renderer_draw_pixel_line         (DiaInteractiveRenderer *self,
                                                       int                     x1,
                                                       int                     y1,
                                                       int                     x2,
                                                       int                     y2,
                                                       Color                  *color);
void dia_interactive_renderer_draw_pixel_rect         (DiaInteractiveRenderer *self,
                                                       int                     x,
                                                       int                     y,
                                                       int                     width,
                                                       int                     height,
                                                       Color                  *color);
void dia_interactive_renderer_fill_pixel_rect         (DiaInteractiveRenderer *self,
                                                       int                     x,
                                                       int                     y,
                                                       int                     width,
                                                       int                     height,
                                                       Color                  *color);
void dia_interactive_renderer_paint                   (DiaInteractiveRenderer *self,
                                                       cairo_t                *ctx,
                                                       int                     width,
                                                       int                     height);
void dia_interactive_renderer_draw_object_highlighted (DiaInteractiveRenderer *self,
                                                       DiaObject              *object,
                                                       DiaHighlightType        type);
void dia_interactive_renderer_set_selection           (DiaInteractiveRenderer *self,
                                                       gboolean                has_selection,
                                                       double                  x,
                                                       double                  y,
                                                       double                  width,
                                                       double                  height);


G_END_DECLS
