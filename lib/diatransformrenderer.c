/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diatransformrenderer.c -- facade renderer to support affine transformation
 *
 * Copyright (C) 2013 Hans Breuer <hans@breuer.org>
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

#include "diatransformrenderer.h"
#include "text.h"
#include "object.h"
#include "diapathrenderer.h"

typedef struct _DiaTransformRenderer DiaTransformRenderer;
typedef struct _DiaTransformRendererClass DiaTransformRendererClass;

/*! GObject boiler plate, create runtime information */
#define DIA_TYPE_TRANSFORM_RENDERER           (dia_transform_renderer_get_type ())
/*! GObject boiler plate, a safe type cast */
#define DIA_TRANSFORM_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_TRANSFORM_RENDERER, DiaTransformRenderer))
/*! GObject boiler plate, in C++ this would be the vtable */
#define DIA_TRANSFORM_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_TRANSFORM_RENDERER, DiaTransformRendererClass))
/*! GObject boiler plate, type check */
#define DIA_IS_TRANSFORM_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_TRANSFORM_RENDERER))
/*! GObject boiler plate, get from object to class (vtable) */
#define DIA_TRANSFORM_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_TRANSFORM_RENDERER, DiaTransformRendererClass))

GType dia_transform_renderer_get_type (void) G_GNUC_CONST;

/*!
 * \brief Renderer which does affine transform rendering
 *
 * The transform renderer does not produce any external output by itself. It
 * directly delegates it's results to the worker renderer. 
 *
 * \extends _DiaRenderer
 */
struct _DiaTransformRenderer
{
  DiaRenderer parent_instance; /*!< inheritance in object oriented C */

  DiaRenderer *worker; /*!< the renderer with real output */

  GQueue *matrices; 
};

struct _DiaTransformRendererClass
{
  DiaRendererClass parent_class; /*!< the base class */
};


G_DEFINE_TYPE (DiaTransformRenderer, dia_transform_renderer, DIA_TYPE_RENDERER)

/*!
 * \brief Constructor
 * Initialize everything which needs something else than 0.
 * \memberof _DiaTransformRenderer
 */
static void
dia_transform_renderer_init (DiaTransformRenderer *self)
{
  self->matrices = g_queue_new ();
}
/*!
 * \brief Destructor
 * If there are still matrices left, deallocate them
 * \memberof _DiaTransformRenderer
 */
static void
dia_path_renderer_finalize (GObject *object)
{
  DiaTransformRenderer *self = DIA_TRANSFORM_RENDERER (object);

  g_queue_free (self->matrices);
  /* drop our reference */
  g_object_unref (self->worker);
}

/*!
 * \brief Starting a new rendering run
 * Should not be called because this renderer is intermediate.
 * \memberof _DiaTransformRenderer
 */
static void
begin_render (DiaRenderer *self, const Rectangle *update)
{
}
/*!
 * \brief End of current rendering run
 * Should not be called because this renderer is intermediate.
 * \memberof _DiaTransformRenderer
 */
static void
end_render(DiaRenderer *self)
{
}
/*!
 * \brief Advertise the renderers capabilities
 * The DiaTransformRenderer can extend every DiaRenderer with affine
 * transformations. Other capabilities are just pass through of the
 * wrapped renderer.
 * \memberof _DiaTransformRenderer
 */
static gboolean
is_capable_to (DiaRenderer *self, RenderCapability cap)
{
  DiaTransformRenderer *renderer = DIA_TRANSFORM_RENDERER (self);

  if (RENDER_AFFINE == cap)
    return TRUE; /* reason for existance */
  g_return_val_if_fail (renderer->worker != NULL, FALSE);
  return DIA_RENDERER_GET_CLASS (renderer->worker)->is_capable_to (renderer->worker, cap);
}
/*!
 * \brief Transform line-width and pass through
 * \memberof _DiaTransformRenderer
 */
static void
set_linewidth(DiaRenderer *self, real linewidth)
{  /* 0 == hairline **/
  DiaTransformRenderer *renderer = DIA_TRANSFORM_RENDERER (self);
  real lw = linewidth;
  DiaMatrix *m = g_queue_peek_tail (renderer->matrices);
  g_return_if_fail (renderer->worker != NULL);
  if (m)
    transform_length (&lw, m);
  DIA_RENDERER_GET_CLASS (renderer->worker)->set_linewidth (renderer->worker, lw);
}
/*!
 * \brief Pass through line caps
 * \memberof _DiaTransformRenderer
 */
static void
set_linecaps(DiaRenderer *self, LineCaps mode)
{
  DiaTransformRenderer *renderer = DIA_TRANSFORM_RENDERER (self);
  g_return_if_fail (renderer->worker != NULL);
  DIA_RENDERER_GET_CLASS (renderer->worker)->set_linecaps (renderer->worker, mode);
}
/*!
 * \brief Pass through line join
 * \memberof _DiaTransformRenderer
 */
static void
set_linejoin(DiaRenderer *self, LineJoin mode)
{
  DiaTransformRenderer *renderer = DIA_TRANSFORM_RENDERER (self);
  g_return_if_fail (renderer->worker != NULL);
  DIA_RENDERER_GET_CLASS (renderer->worker)->set_linejoin (renderer->worker, mode);
}
/*!
 * \brief Pass through line style
 * \memberof _DiaTransformRenderer
 */
static void
set_linestyle(DiaRenderer *self, LineStyle mode)
{
  DiaTransformRenderer *renderer = DIA_TRANSFORM_RENDERER (self);
  g_return_if_fail (renderer->worker != NULL);
  DIA_RENDERER_GET_CLASS (renderer->worker)->set_linestyle (renderer->worker, mode);
}
/*!
 * \brief Transform dash length and pass through
 * \memberof _DiaTransformRenderer
 */
static void
set_dashlength(DiaRenderer *self, real length)
{  /* dot = 20% of len */
  DiaTransformRenderer *renderer = DIA_TRANSFORM_RENDERER (self);
  real dl= length;
  DiaMatrix *m = g_queue_peek_tail (renderer->matrices);
  g_return_if_fail (renderer->worker != NULL);
  if (m)
    transform_length (&dl, m);
  DIA_RENDERER_GET_CLASS (renderer->worker)->set_dashlength (renderer->worker, dl);
}
/*!
 * \brief Pass through fill style
 * \memberof _DiaTransformRenderer
 */
static void
set_fillstyle(DiaRenderer *self, FillStyle mode)
{
  DiaTransformRenderer *renderer = DIA_TRANSFORM_RENDERER (self);
  g_return_if_fail (renderer->worker != NULL);
  DIA_RENDERER_GET_CLASS (renderer->worker)->set_fillstyle (renderer->worker, mode);
}
/*!
 * \brief Transform line and delegate draw
 * \memberof _DiaTransformRenderer
 */
static void
draw_line(DiaRenderer *self, 
	  Point *start, Point *end, 
	  Color *line_colour)
{
  Point p1 = *start;
  Point p2 = *end;
  DiaTransformRenderer *renderer = DIA_TRANSFORM_RENDERER (self);
  DiaMatrix *m = g_queue_peek_tail (renderer->matrices);
  g_return_if_fail (renderer->worker != NULL);
  if (m) {
    transform_point(&p1, m);
    transform_point(&p2, m);
  }
  DIA_RENDERER_GET_CLASS (renderer->worker)->draw_line (renderer->worker, &p1, &p2, line_colour);
}
static void
_polyline(DiaRenderer *self, 
	  Point *points, int num_points, 
	  Color *stroke, Color *fill,
	  gboolean closed)
{
  Point *a_pts = g_newa (Point, num_points);
  DiaTransformRenderer *renderer = DIA_TRANSFORM_RENDERER (self);
  DiaMatrix *m = g_queue_peek_tail (renderer->matrices);
  g_return_if_fail (renderer->worker != NULL);
  memcpy (a_pts, points, sizeof(Point)*num_points);
  if (m) {
    int i;
    for (i = 0; i < num_points; ++i)
      transform_point (&a_pts[i], m);
  }
  if (fill)
    DIA_RENDERER_GET_CLASS (renderer->worker)->fill_polygon (renderer->worker, a_pts, num_points, fill);
  else if (closed)
    DIA_RENDERER_GET_CLASS (renderer->worker)->draw_polygon (renderer->worker, a_pts, num_points, stroke);
  else
    DIA_RENDERER_GET_CLASS (renderer->worker)->draw_polyline (renderer->worker, a_pts, num_points, stroke);
}
/*!
 * \brief Transform polyline and delegate draw
 * \memberof _DiaTransformRenderer
 */
static void
draw_polyline(DiaRenderer *self, 
	      Point *points, int num_points, 
	      Color *line_colour)
{
  _polyline (self, points, num_points, line_colour, NULL, FALSE);
}
/*!
 * \brief Transform polygon and delegate draw
 * \memberof _DiaTransformRenderer
 */
static void
draw_polygon(DiaRenderer *self, 
	      Point *points, int num_points, 
	      Color *line_colour)
{
  _polyline (self, points, num_points, line_colour, NULL, TRUE);
}
/*!
 * \brief Transform polygon and delegate fill
 * \memberof _DiaTransformRenderer
 */
static void
fill_polygon(DiaRenderer *self, 
	     Point *points, int num_points, 
	     Color *color)
{
  _polyline (self, points, num_points, NULL, color, TRUE);
}
static void
_rect (DiaRenderer *self, 
       Point *ul_corner, Point *lr_corner,
       Color *stroke, Color *fill)
{
  Point corner[4];
  /* translate to polygon */
  corner[0] = *ul_corner;
  corner[1].x = lr_corner->x;
  corner[1].y = ul_corner->y;
  corner[2] = *lr_corner;
  corner[3].x = ul_corner->x;
  corner[3].y = lr_corner->y;
  /* delegate transformation and drawing */
  _polyline (self, corner, 4, stroke, fill, TRUE);
}
/*!
 * \brief Transform rectangle and delegate draw
 * \memberof _DiaTransformRenderer
 */
static void
draw_rect (DiaRenderer *self, 
	   Point *ul_corner, Point *lr_corner,
	   Color *color)
{
  _rect (self, ul_corner, lr_corner, color, NULL);
}
/*!
 * \brief Transform rectangle and delegate fill
 * \memberof _DiaTransformRenderer
 */
static void
fill_rect (DiaRenderer *self, 
	   Point *ul_corner, Point *lr_corner,
	   Color *color)
{
  _rect (self, ul_corner, lr_corner, NULL, color);
}
/* ToDo: arc and ellipse to be emulated by bezier - in base class? */
static void
_bezier (DiaRenderer *self, 
	 BezPoint *points, int num_points,
	 Color *fill, Color *stroke,
	 gboolean closed)
{
  BezPoint *a_pts = g_newa (BezPoint, num_points);
  DiaTransformRenderer *renderer = DIA_TRANSFORM_RENDERER (self);
  DiaMatrix *m = g_queue_peek_tail (renderer->matrices);
  g_return_if_fail (renderer->worker != NULL);
  memcpy (a_pts, points, sizeof(BezPoint)*num_points);
  if (m) {
    int i;
    for (i = 0; i < num_points; ++i)
      transform_bezpoint (&a_pts[i], m);
  }
  if (closed)
    DIA_RENDERER_GET_CLASS (renderer->worker)->draw_beziergon (renderer->worker, a_pts, num_points, fill, stroke);
  else
    DIA_RENDERER_GET_CLASS (renderer->worker)->draw_bezier (renderer->worker, a_pts, num_points, stroke);
  if (!closed)
    g_return_if_fail (fill == NULL && "fill for stroke?");
}
static void
_arc (DiaRenderer *self, 
      Point *center,
      real width, real height,
      real angle1, real angle2,
      Color *stroke, Color *fill)
{
  GArray *path = g_array_new (FALSE, FALSE, sizeof(BezPoint));
  path_build_arc (path, center, width, height, angle1, angle2, stroke == NULL);
  _bezier (self, &g_array_index (path, BezPoint, 0), path->len, fill, stroke, fill!=NULL);
  g_array_free (path, TRUE);
}
/*!
 * \brief Transform arc and delegate draw
 * \memberof _DiaTransformRenderer
 */
static void
draw_arc (DiaRenderer *self, 
	  Point *center,
	  real width, real height,
	  real angle1, real angle2,
	  Color *color)
{
  _arc (self, center, width, height, angle1, angle2, color, NULL);
}
/*!
 * \brief Transform arc and delegate fill
 * \memberof _DiaTransformRenderer
 */
static void
fill_arc (DiaRenderer *self, 
	  Point *center,
	  real width, real height,
	  real angle1, real angle2,
	  Color *color)
{
  _arc (self, center, width, height, angle1, angle2, NULL, color);
}
static void
_ellipse (DiaRenderer *self,
	  Point *center,
	  real width, real height,
	  Color *stroke, Color *fill)
{
  GArray *path = g_array_new (FALSE, FALSE, sizeof(BezPoint));
  path_build_ellipse (path, center, width, height);
  _bezier (self, &g_array_index (path, BezPoint, 0), path->len, fill, stroke, fill!=NULL);
  g_array_free (path, TRUE);
}
/*!
 * \brief Transform ellipse and delegate draw
 * \memberof _DiaTransformRenderer
 */
static void
draw_ellipse (DiaRenderer *self, 
	      Point *center,
	      real width, real height,
	      Color *color)
{
  _ellipse (self, center, width, height, color, NULL);
}
/*!
 * \brief Transform ellipse and delegate fill
 * \memberof _DiaTransformRenderer
 */
static void
fill_ellipse (DiaRenderer *self, 
	      Point *center,
	      real width, real height,
	      Color *color)
{
  _ellipse (self, center, width, height, NULL, color);
}
/*!
 * \brief Transform bezier and delegate draw
 * \memberof _DiaTransformRenderer
 */
static void
draw_bezier (DiaRenderer *self, 
	     BezPoint *points,
	     int numpoints,
	     Color *color)
{
  _bezier(self, points, numpoints, NULL, color, FALSE);
}
/*!
 * \brief Transform bezier and delegate fill
 * \memberof _DiaTransformRenderer
 */
static void
draw_beziergon (DiaRenderer *self, 
		BezPoint *points, /* Last point must be same as first point */
		int numpoints,
		Color *fill,
		Color *stroke)
{
  _bezier(self, points, numpoints, fill, stroke, TRUE);
}
/*!
 * \brief Transform the text object while drawing
 * \memberof _DiaTransformRenderer
 */
static void 
draw_text (DiaRenderer *self,
	   Text        *text) 
{
  DiaTransformRenderer *renderer = DIA_TRANSFORM_RENDERER (self);
  DiaMatrix *m = g_queue_peek_tail (renderer->matrices);

  /* ToDo: see DiaPathRenderer? */
  Point pos;
  int i;

  pos = text->position;

  for (i=0;i<text->numlines;i++) {
    TextLine *text_line = text->lines[i];
    Point pt;

    pt = pos;
    if (m) {
      transform_point (&pt, m);
      /* ToDo: fon-size and angle */
    }
    DIA_RENDERER_GET_CLASS(renderer->worker)->draw_text_line(renderer->worker, text_line,
						             &pt, text->alignment,
						             &text->color);
    pos.y += text->height;
  }

}
/*!
 * \brief Convert the string back to a _Text object and render that
 * \memberof _DiaTransformRenderer
 */
static void
draw_string(DiaRenderer *self,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *color)
{
  if (text && strlen(text)) {
    Text *text_obj;
    /* it could have been so easy without the context switch */
    text_obj = new_text (text,
			 self->font, self->font_height,
			 pos, color, alignment);
    draw_text (self, text_obj);
    text_destroy (text_obj);
  }
}
/*!
 * \brief Draw a potentially transformed image
 * \memberof _DiaTransformRenderer
 */
static void
draw_image(DiaRenderer *self,
	   Point *point,
	   real width, real height,
	   DiaImage *image)
{
  Point p1 = *point;
  Point p2 = p1;
  DiaTransformRenderer *renderer = DIA_TRANSFORM_RENDERER (self);
  DiaMatrix *m = g_queue_peek_tail (renderer->matrices);
  g_return_if_fail (renderer->worker != NULL);

  /* ToDo: some support on the library level? */
  p2.x += width;
  p2.y += height;
  if (m) {
    transform_point (&p1, m);
    transform_point (&p2, m);
  }
  /* FIXME: for now only the position is transformed */
  DIA_RENDERER_GET_CLASS (renderer->worker)->draw_image (renderer->worker, &p1, p2.x - p1.x, p2.y - p1.y, image);
}

/*!
 * \brief Main renderer function used by lib/app, matrix transport
 * \memberof _DiaTransformRenderer
 */
static void
draw_object (DiaRenderer *self,
	     DiaObject   *object,
	     DiaMatrix   *matrix) 
{
  DiaTransformRenderer *renderer = DIA_TRANSFORM_RENDERER (self);
  DiaMatrix *m = g_queue_peek_tail (renderer->matrices);
  g_return_if_fail (renderer->worker != NULL);
  
  if (matrix) {
    DiaMatrix *m2 = g_new (DiaMatrix, 1);
    if (m)
      dia_matrix_multiply (m2, matrix, m);
    else
      *m2 = *matrix;
    g_queue_push_tail (renderer->matrices, m2);
  }
  /* This will call us again */
  object->ops->draw(object, DIA_RENDERER (renderer));
  if (matrix)
    g_free (g_queue_pop_tail (renderer->matrices));
}

/*!
 * \brief _DiaTransformRenderer class initialization
 * Overwrite methods of the base classes here.
 * \memberof _DiaTransformRenderer
 */
static void
dia_transform_renderer_class_init (DiaTransformRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  object_class->finalize = dia_path_renderer_finalize;

  renderer_class->draw_object = draw_object;
  /* renderer members */
  renderer_class->begin_render = begin_render;
  renderer_class->end_render   = end_render;

  renderer_class->set_linewidth  = set_linewidth;
  renderer_class->set_linecaps   = set_linecaps;
  renderer_class->set_linejoin   = set_linejoin;
  renderer_class->set_linestyle  = set_linestyle;
  renderer_class->set_dashlength = set_dashlength;
  renderer_class->set_fillstyle  = set_fillstyle;

  renderer_class->draw_line    = draw_line;
  renderer_class->fill_polygon = fill_polygon;
  renderer_class->draw_rect    = draw_rect;
  renderer_class->fill_rect    = fill_rect;
  renderer_class->draw_arc     = draw_arc;
  renderer_class->fill_arc     = fill_arc;
  renderer_class->draw_ellipse = draw_ellipse;
  renderer_class->fill_ellipse = fill_ellipse;

  renderer_class->draw_string  = draw_string;
  renderer_class->draw_image   = draw_image;

  /* medium level functions */
  renderer_class->draw_rect = draw_rect;
  renderer_class->draw_polyline  = draw_polyline;
  renderer_class->draw_polygon   = draw_polygon;

  renderer_class->draw_bezier   = draw_bezier;
  renderer_class->draw_beziergon = draw_beziergon;
  renderer_class->draw_text     = draw_text;
  /* other */
  renderer_class->is_capable_to = is_capable_to;
}

/*!
 * \brief Factory function to construct a transform renderer
 */
DiaRenderer *
dia_transform_renderer_new (DiaRenderer *to_be_wrapped)
{
  DiaTransformRenderer *tr = g_object_new (DIA_TYPE_TRANSFORM_RENDERER, NULL);
  tr->worker = g_object_ref (to_be_wrapped);

  return DIA_RENDERER (tr);
}
