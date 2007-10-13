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

/*! \file diarenderer.h -- the basic renderer interface definition */
#ifndef DIA_RENDERER_H
#define DIA_RENDERER_H

#include "diatypes.h"
#include <glib-object.h>

#include "dia-enums.h"
#include "geometry.h"
#include "font.h" /* not strictly needed by this header, but needed in almost any plug-in/ */

G_BEGIN_DECLS

/*! GObject boiler plate, create runtime information */
#define DIA_TYPE_RENDERER           (dia_renderer_get_type ())
/*! GObject boiler plate, a safe type cast */
#define DIA_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_RENDERER, DiaRenderer))
/*! GObject boiler plate, in C++ this would be the vtable */
#define DIA_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_RENDERER, DiaRendererClass))
/*! GObject boiler plate, type check */
#define DIA_IS_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_RENDERER))
/*! GObject boiler plate, get from object to class (vtable) */
#define DIA_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_RENDERER, DiaRendererClass))

GType dia_renderer_get_type (void) G_GNUC_CONST;

/*! \brief The member variables part of _DiaRenderer */
struct _DiaRenderer
{
  GObject parent_instance; /*!< inheritance in object oriented C */
  gboolean is_interactive; /*!< if the user can interact */
  /*< private >*/
  DiaFont *font;
  real font_height; /* IMO It should be possible use the font's size to keep
                     * this info, but currently _not_ : multiline text is 
                     * growing on every line when zoomed: BUG in font.c  --hb
                     */
  BezierApprox *bezier;
};

/*!
 * \class _DiaRendererClass
 *
 * \brief Base class for all of Dia's render facilities
 *
 * Renderers work in close cooperation with DiaObject. They provide the way to
 * make all the object drawing independent of concrete drawing backends
 */
struct _DiaRendererClass
{
  GObjectClass parent_class; /*!< the base class */

  /*! return width in pixels, only for interactive renderers */
  int (*get_width_pixels) (DiaRenderer*);
  /*! return width in pixels, only for interactive renderers */
  int (*get_height_pixels) (DiaRenderer*);
  /*! simply calls the objects draw function, which calls this again */
  void (*draw_object) (DiaRenderer*, DiaObject*);
  /*! Returns the EXACT width of text in cm, using the current font.
     There has been some confusion as to the definition of this.
     It used to say the width was in pixels, but actual width returned
     was cm.  You shouldn't know about pixels anyway.
   */
  real (*get_text_width) (DiaRenderer *renderer,
                          const gchar *text, int length);


  /* 
   * Function which MUST be implemented by any DiaRenderer 
   */
  /*! Called before rendering begins.
     Can be used to do various pre-rendering setup. */
  void (*begin_render) (DiaRenderer *);
  /*! Called after all rendering is done.
     Used to do various clean-ups.*/
  void (*end_render) (DiaRenderer *);

  /*! Set the current line width
     if linewidth==0, the line will be an 'hairline' */
  void (*set_linewidth) (DiaRenderer *renderer, real linewidth);
  /*! Set the current linecap (the way lines are ended) */
  void (*set_linecaps) (DiaRenderer *renderer, LineCaps mode);
  /*! Set the current linejoin (the way two lines are joined together) */
  void (*set_linejoin) (DiaRenderer *renderer, LineJoin mode);
  /*! Set the current line style */
  void (*set_linestyle) (DiaRenderer *renderer, LineStyle mode);
  /*! Set the dash length, when the style is not SOLID
     A dot will be 10% of length */
  void (*set_dashlength) (DiaRenderer *renderer, real length);
  /*! Set the fill style */
  void (*set_fillstyle) (DiaRenderer *renderer, FillStyle mode);
  /*! Set the current font */
  void (*set_font) (DiaRenderer *renderer, DiaFont *font, real height);

  /*! Draw a line from start to end, using color and the current line style */
  void (*draw_line) (DiaRenderer *renderer,
                     Point *start, Point *end,
                     Color *color);
  /*! Fill a rectangle, given its upper-left and lower-right corners */
  void (*fill_rect) (DiaRenderer *renderer,
                     Point *ul_corner, Point *lr_corner,
                     Color *color);
  /*! the polygon is filled using the current fill type, no border is drawn */
  void (*fill_polygon) (DiaRenderer *renderer,
                        Point *points, int num_points,
                        Color *color);
  /*! Draw an arc, given its center, the bounding box (widget, height),
     the start angle and the end angle */
  void (*draw_arc) (DiaRenderer *renderer,
                    Point *center,
                    real width, real height,
                    real angle1, real angle2,
                    Color *color);
  /*! Same a DrawArcFunc except the arc is filled (a pie-chart) */
  void (*fill_arc) (DiaRenderer *renderer,
                    Point *center,
                    real width, real height,
                    real angle1, real angle2,
                    Color *color);
  /*! Draw an ellipse, given its center and the bounding box */
  void (*draw_ellipse) (DiaRenderer *renderer,
                        Point *center,
                        real width, real height,
                        Color *color);
  /*! Same a DrawEllipse, except the ellips is filled */
  void (*fill_ellipse) (DiaRenderer *renderer,
                        Point *center,
                        real width, real height,
                        Color *color);
  /*! Print a string at pos, using the current font */
  void (*draw_string) (DiaRenderer *renderer,
                       const gchar *text,
                       Point *pos,
                       Alignment alignment,
                       Color *color);
  /*! Draw an image, given its bounding box */
  void (*draw_image) (DiaRenderer *renderer,
                      Point *point,
                      real width, real height,
                      DiaImage image);

  /* 
   * Functions which SHOULD be implemented by specific renderer, but
   * have a default implementation based on the above functions 
   */
  /*! Draw a bezier curve, given it's control points. The first BezPoint must 
     be of type MOVE_TO, and no other ones may be MOVE_TO's. */
  void (*draw_bezier) (DiaRenderer *renderer,
                       BezPoint *points,
                       int numpoints,
                       Color *color);
  /*! Same as DrawBezierFunc, except the last point must be the same as the
     first point, and the resulting shape is filled */
  void (*fill_bezier) (DiaRenderer *renderer,
                       BezPoint *points,
                       int numpoints,
                       Color *color);
  /*! Draw a line joining multiple points, using color and the current
     line style */
  void (*draw_polyline) (DiaRenderer *renderer,
                         Point *points, int num_points,
                         Color *color);
  /*! Draw a line joining multiple points, using color and the current
     line style with rounded corners between segments */
  void (*draw_rounded_polyline) (DiaRenderer *renderer,
                         Point *points, int num_points,
                         Color *color, real radius );
  /*! Draw a polygone, using the current line style
     The polygon is closed even if the first point is not the same as the
     last point */
  void (*draw_polygon) (DiaRenderer *renderer,
                        Point *points, int num_points,
                        Color *color);
  /*! Print a Text.  It holds its own information. */
  void (*draw_text) (DiaRenderer *renderer,
                     Text *text);
  /*! Print a TextLine.  It holds its own font/size information. */
  void (*draw_text_line) (DiaRenderer *renderer,
			  TextLine *text_line, Point *pos, Alignment alignment, Color *color);
  /*! Draw a rectangle, given its upper-left and lower-right corners */
  void (*draw_rect) (DiaRenderer *renderer,
                     Point *ul_corner, Point *lr_corner,
                     Color *color);

  /*
   * Highest level functions, probably only to be implemented by 
   * special 'high level' renderers
   */
  /*! Draw a rounded rectangle, given its upper-left and lower-right corners */
  void (*draw_rounded_rect) (DiaRenderer *renderer,
                             Point *ul_corner, Point *lr_corner,
                             Color *color, real radius);
  /*! Same a DrawRoundedRectangleFunc, except the rectangle is filled using the
     current fill style */
  void (*fill_rounded_rect) (DiaRenderer *renderer,
                             Point *ul_corner, Point *lr_corner,
                             Color *color, real radius);

  /*! Highest level function doing specific arrow positioning */
  void (*draw_line_with_arrows)  (DiaRenderer *renderer, 
                                  Point *start, Point *end, 
                                  real line_width,
                                  Color *line_color,
                                  Arrow *start_arrow,
                                  Arrow *end_arrow);
  /*! Highest level function doing specific arrow positioning */
  void (*draw_arc_with_arrows)  (DiaRenderer *renderer, 
                                 Point *start, Point *end,
                                 Point *midpoint,
                                 real line_width,
                                 Color *color,
                                 Arrow *start_arrow,
                                 Arrow *end_arrow);
  /*! Highest level function doing specific arrow positioning */
  void (*draw_polyline_with_arrows) (DiaRenderer *renderer, 
                                     Point *points, int num_points,
                                     real line_width,
                                     Color *color,
                                     Arrow *start_arrow,
                                     Arrow *end_arrow);
  void (*draw_rounded_polyline_with_arrows) (DiaRenderer *renderer, 
                                     Point *points, int num_points,
                                     real line_width,
                                     Color *color,
                                     Arrow *start_arrow,
                                     Arrow *end_arrow,
                                     real radius);

  void (*draw_bezier_with_arrows) (DiaRenderer *renderer, 
                                   BezPoint *points,
                                   int num_points,
                                   real line_width,
                                   Color *color,
                                   Arrow *start_arrow,
                                   Arrow *end_arrow);
};

/*
 * Declare the Interactive Renderer Interface, which get's added
 * to some renderer classes by app/
 */
#define DIA_TYPE_INTERACTIVE_RENDERER_INTERFACE     (dia_interactive_renderer_interface_get_type ())
#define DIA_GET_INTERACTIVE_RENDERER_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), DIA_TYPE_INTERACTIVE_RENDERER_INTERFACE, DiaInteractiveRendererInterface))

struct _DiaInteractiveRendererInterface
{
  GTypeInterface base_iface;

  /* Clear the current clipping region. */
  void (*set_size)            (DiaRenderer *renderer, gpointer, int, int);

  /* Clear the current clipping region. */
  void (*clip_region_clear)    (DiaRenderer *renderer);

  /* Add a rectangle to the current clipping region. */
  void (*clip_region_add_rect) (DiaRenderer *renderer, Rectangle *rect);

  /* Draw a line from start to end, using color and the current line style */
  void (*draw_pixel_line)      (DiaRenderer *renderer,
                                int x1, int y1, int x2, int y2,
                                Color *color);
  /* Draw a rectangle, given its upper-left and lower-right corners in pixels. */
  void (*draw_pixel_rect)      (DiaRenderer *renderer,
                                int x, int y, int width, int height,
                                Color *color);
  /* Fill a rectangle, given its upper-left and lower-right corners in pixels. */
  void (*fill_pixel_rect)      (DiaRenderer *renderer,
                                int x, int y, int width, int height,
                                Color *color);

  void (*copy_to_window)      (DiaRenderer *renderer,
                               gpointer     window, 
                               int x, int y, int width, int height);
};

GType dia_interactive_renderer_interface_get_type (void) G_GNUC_CONST;

void dia_renderer_set_size          (DiaRenderer*, gpointer window, int, int);
int  dia_renderer_get_width_pixels  (DiaRenderer*);
int  dia_renderer_get_height_pixels (DiaRenderer*);

G_END_DECLS

#endif /* DIA_RENDERER_H */
