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

#include <glib/gi18n-lib.h>

#include "diatransformrenderer.h"
#include "text.h"
#include "object.h"
#include "diapathrenderer.h"

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
  g_clear_object (&self->worker);
}


/*!
 * \brief Starting a new rendering run
 * Should not be called because this renderer is intermediate.
 * \memberof _DiaTransformRenderer
 */
static void
begin_render (DiaRenderer *self, const DiaRectangle *update)
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

  if (RENDER_AFFINE == cap) {
    return TRUE; /* reason for existence */
  }

  g_return_val_if_fail (renderer->worker != NULL, FALSE);

  return dia_renderer_is_capable_of (renderer->worker, cap);
}

/*!
 * \brief Transform line-width and pass through
 * \memberof _DiaTransformRenderer
 */
static void
set_linewidth (DiaRenderer *self, real linewidth)
{  /* 0 == hairline **/
  DiaTransformRenderer *renderer = DIA_TRANSFORM_RENDERER (self);
  real lw = linewidth;
  DiaMatrix *m = g_queue_peek_tail (renderer->matrices);
  g_return_if_fail (renderer->worker != NULL);
  if (m) {
    transform_length (&lw, m);
  }
  dia_renderer_set_linewidth (renderer->worker, lw);
}


/*
 * Pass through line caps
 */
static void
set_linecaps (DiaRenderer *self, DiaLineCaps mode)
{
  DiaTransformRenderer *renderer = DIA_TRANSFORM_RENDERER (self);

  g_return_if_fail (renderer->worker != NULL);

  dia_renderer_set_linecaps (renderer->worker, mode);
}


/*
 * Pass through line join
 */
static void
set_linejoin (DiaRenderer *self, DiaLineJoin mode)
{
  DiaTransformRenderer *renderer = DIA_TRANSFORM_RENDERER (self);

  g_return_if_fail (renderer->worker != NULL);

  dia_renderer_set_linejoin (renderer->worker, mode);
}


/*
 * Pass through line style, transform dash length and pass through
 */
static void
set_linestyle (DiaRenderer *self, DiaLineStyle mode, double dash_length)
{
  DiaTransformRenderer *renderer = DIA_TRANSFORM_RENDERER (self);
  DiaMatrix *m = g_queue_peek_tail (renderer->matrices);

  g_return_if_fail (renderer->worker != NULL);

  if (m) {
    transform_length (&dash_length, m);
  }

  dia_renderer_set_linestyle (renderer->worker, mode, dash_length);
}


/*
 * Pass through fill style
 */
static void
set_fillstyle (DiaRenderer *self, DiaFillStyle mode)
{
  DiaTransformRenderer *renderer = DIA_TRANSFORM_RENDERER (self);

  g_return_if_fail (renderer->worker != NULL);

  dia_renderer_set_fillstyle (renderer->worker, mode);
}

/*!
 * \brief Transform line and delegate draw
 * \memberof _DiaTransformRenderer
 */
static void
draw_line (DiaRenderer *self,
           Point       *start,
           Point       *end,
           Color       *line_colour)
{
  Point p1 = *start;
  Point p2 = *end;
  DiaTransformRenderer *renderer = DIA_TRANSFORM_RENDERER (self);
  DiaMatrix *m = g_queue_peek_tail (renderer->matrices);
  g_return_if_fail (renderer->worker != NULL);
  if (m) {
    transform_point (&p1, m);
    transform_point (&p2, m);
  }
  dia_renderer_draw_line (renderer->worker, &p1, &p2, line_colour);
}

static void
_polyline (DiaRenderer *self,
           Point       *points,
           int          num_points,
           Color       *fill,
           Color       *stroke,
           gboolean     closed)
{
  Point *a_pts = g_newa (Point, num_points);
  DiaTransformRenderer *renderer = DIA_TRANSFORM_RENDERER (self);
  DiaMatrix *m = g_queue_peek_tail (renderer->matrices);
  g_return_if_fail (renderer->worker != NULL);
  memcpy (a_pts, points, sizeof(Point)*num_points);
  if (m) {
    int i;
    for (i = 0; i < num_points; ++i) {
      transform_point (&a_pts[i], m);
    }
  }
  if (closed) {
    dia_renderer_draw_polygon (renderer->worker, a_pts, num_points, fill, stroke);
  } else {
    dia_renderer_draw_polyline (renderer->worker, a_pts, num_points, stroke);
  }
}

/*!
 * \brief Transform polyline and delegate draw
 * \memberof _DiaTransformRenderer
 */
static void
draw_polyline (DiaRenderer *self,
               Point       *points,
               int          num_points,
               Color       *stroke)
{
  _polyline (self, points, num_points, NULL, stroke, FALSE);
}

/*!
 * \brief Transform polygon and delegate draw
 * \memberof _DiaTransformRenderer
 */
static void
draw_polygon (DiaRenderer *self,
              Point       *points,
              int          num_points,
              Color       *fill,
              Color       *stroke)
{
  _polyline (self, points, num_points, fill, stroke, TRUE);
}

/* ToDo: arc and ellipse to be emulated by bezier - in base class? */
static void
_bezier (DiaRenderer *self,
         BezPoint    *points,
         int          num_points,
         Color       *fill,
         Color       *stroke,
         gboolean     closed)
{
  BezPoint *a_pts = g_newa (BezPoint, num_points);
  DiaTransformRenderer *renderer = DIA_TRANSFORM_RENDERER (self);
  DiaMatrix *m = g_queue_peek_tail (renderer->matrices);

  g_return_if_fail (renderer->worker != NULL);

  memcpy (a_pts, points, sizeof(BezPoint)*num_points);
  if (m) {
    int i;
    for (i = 0; i < num_points; ++i) {
      transform_bezpoint (&a_pts[i], m);
    }
  }
  if (closed) {
    dia_renderer_draw_beziergon (renderer->worker, a_pts, num_points, fill, stroke);
  } else {
    dia_renderer_draw_bezier (renderer->worker, a_pts, num_points, stroke);
  }
  if (!closed) {
    g_return_if_fail (fill == NULL && "fill for stroke?");
  }
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
/*!
 * \brief Transform ellipse and delegate draw
 * \memberof _DiaTransformRenderer
 */
static void
draw_ellipse (DiaRenderer *self,
	      Point *center,
	      real width, real height,
	      Color *fill, Color *stroke)
{
  GArray *path = g_array_new (FALSE, FALSE, sizeof(BezPoint));
  path_build_ellipse (path, center, width, height);
  _bezier (self, &g_array_index (path, BezPoint, 0), path->len, fill, stroke, fill!=NULL);
  g_array_free (path, TRUE);
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
  Point pos = text->position;
  real angle, sx, sy;
  int i;

  pos = text->position;
  if (m && dia_matrix_get_angle_and_scales (m, &angle, &sx, &sy)) {
    Text *tc = text_copy (text);
    transform_point (&pos, m);
    text_set_position (tc, &pos);
    text_set_height (tc, text_get_height (text) * MIN(sx,sy));
    dia_renderer_draw_rotated_text (renderer->worker,
                                    tc,
                                    NULL,
                                    180.0 * angle / G_PI);
    text_destroy (tc);
  } else {
    for (i=0;i<text->numlines;i++) {
      TextLine *text_line = text->lines[i];
      Point pt;

      pt = pos;
      if (m) {
        transform_point (&pt, m);
        /* ToDo: font-size and angle */
      }
      dia_renderer_draw_text_line (renderer->worker,
                                   text_line,
                                   &pt,
                                   text->alignment,
                                   &text->color);
      pos.y += text->height;
    }
  }
}

/*!
 * \brief Transform the text object while drawing
 *
 * Join the transformation from the renderer with the rotation from the API.
 *
 * \memberof _DiaTransformRenderer
 */
static void
draw_rotated_text (DiaRenderer *self, Text *text, Point *center, real angle)
{
  DiaTransformRenderer *renderer = DIA_TRANSFORM_RENDERER (self);
  DiaMatrix *m = g_queue_peek_tail (renderer->matrices);
  Point pos = text->position;

  if (m) {
    real angle2, sx, sy;
    DiaMatrix m2 = { 1, 0, 0, 1, -pos.x, -pos.y };
    DiaMatrix t = { 1, 0, 0, 1, pos.x, pos.y };

    if (center) {
      m2.x0 = -center->x;
      m2.y0 = -center->y;
      t.x0 = center->x;
      t.y0 = center->y;
    }
    dia_matrix_set_angle_and_scales (&m2, G_PI * angle / 180.0, 1.0, 1.0);
    dia_matrix_multiply (&m2, &t, &m2);
    dia_matrix_multiply (&m2, m, &m2);
    if (dia_matrix_get_angle_and_scales (&m2, &angle2, &sx, &sy)) {
      Text *tc = text_copy (text);
      /* text position is independent of the rotation matrix */
      transform_point (&pos, m);
      text_set_position (tc, &pos);
      text_set_height (tc, text_get_height (text) * MIN(sx,sy));
      dia_renderer_draw_rotated_text (renderer->worker,
                                      tc,
                                      NULL,
                                      180.0 * angle2 / G_PI);
      text_destroy (tc);
    } else {
      g_warning ("DiaTransformRenderer::draw_rotated_text() bad matrix.");
    }
  } else {
    /* just pass through */
    dia_renderer_draw_rotated_text (renderer->worker,
                                    text,
                                    center,
                                    G_PI * angle / 180.0);
  }
}


/*
 * Convert the string back to a _Text object and render that
 */
static void
draw_string (DiaRenderer  *self,
             const char   *text,
             Point        *pos,
             DiaAlignment  alignment,
             Color        *color)
{
  if (text && strlen (text)) {
    Text *text_obj;
    DiaFont *font;
    double font_height;

    font = dia_renderer_get_font (self, &font_height);

    /* it could have been so easy without the context switch */
    text_obj = new_text (text,
                         font,
                         font_height,
                         pos,
                         color,
                         alignment);
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
  Point pc = p1;
  DiaTransformRenderer *renderer = DIA_TRANSFORM_RENDERER (self);
  DiaMatrix *m = g_queue_peek_tail (renderer->matrices);
  g_return_if_fail (renderer->worker != NULL);

  /* ToDo: some support on the library level? */
  pc.x += width/2.0;
  pc.y += height/2.0;
  if (m) {
    transform_point (&pc, m);
    p1.x = pc.x - width/2.0;
    p1.y = pc.y - height/2.0;
  }
  /* FIXME: for now only the position is transformed */
  dia_renderer_draw_image (renderer->worker, &p1, width, height, image);
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
    DiaMatrix *m2 = g_new0 (DiaMatrix, 1);
    if (m)
      dia_matrix_multiply (m2, matrix, m);
    else
      *m2 = *matrix;
    g_queue_push_tail (renderer->matrices, m2);
  }
  /* This will call us again */
  dia_object_draw (object, DIA_RENDERER (renderer));
  if (matrix) {
    g_free (g_queue_pop_tail (renderer->matrices));
  }
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
  renderer_class->set_fillstyle  = set_fillstyle;

  renderer_class->draw_line    = draw_line;
  renderer_class->draw_polygon = draw_polygon;
  renderer_class->draw_arc     = draw_arc;
  renderer_class->fill_arc     = fill_arc;
  renderer_class->draw_ellipse = draw_ellipse;

  renderer_class->draw_string  = draw_string;
  renderer_class->draw_image   = draw_image;

  /* medium level functions */
  renderer_class->draw_polyline  = draw_polyline;

  renderer_class->draw_bezier   = draw_bezier;
  renderer_class->draw_beziergon = draw_beziergon;
  renderer_class->draw_text     = draw_text;
  renderer_class->draw_rotated_text = draw_rotated_text;
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
