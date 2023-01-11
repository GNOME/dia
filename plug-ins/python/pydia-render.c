/* Python plug-in for dia
 * Copyright (C) 1999  James Henstridge
 * Copyright (C) 2000,2002  Hans Breuer
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*!
 * \file pydia-render.c Wrapper to implement _DiaRenderer in Python
 *
 * The PyDiaRenderer is currently defined in Python only. The
 * DiaPyRenderer is using it's interface to map the gobject
 * DiaRenderer to it. A next step could be to let Python code
 * directly inherit from DiaPyRenderer.
 * To do that probably some code from pygtk.gobject needs to
 * be borrowed/shared
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <Python.h>
#include <glib.h>
#include <glib/gstdio.h>

#include <locale.h>

#include "geometry.h"

#include "pydia-object.h" /* for PyObject_HEAD_INIT */
#include "pydia-export.h"
#include "pydia-diagramdata.h"
#include "pydia-geometry.h"
#include "pydia-color.h"
#include "pydia-font.h"
#include "pydia-image.h"
#include "pydia-error.h"
#include "pydia-render.h"
#include "pydia-layer.h"

#include "diarenderer.h"

#define DIA_TYPE_PY_RENDERER           (dia_py_renderer_get_type ())
#define DIA_PY_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_PY_RENDERER, DiaPyRenderer))
#define DIA_PY_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_PY_RENDERER, DiaPyRendererClass))
#define DIA_IS_PY_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_PY_RENDERER))
#define DIA_PY_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_PY_RENDERER, DiaPyRendererClass))

typedef struct _DiaPyRenderer DiaPyRenderer;
typedef struct _DiaPyRendererClass DiaPyRendererClass;

/*!
 * \brief Wrapper class to allow renderer implementation in Python
 *
 * The DiaPyRenderer class serves basically two use cases.
 * - the assumed to be obvious one is to implement a drawing exporter
 *   in Python. See diasvg.SvgRenderer for an example.
 * - the other use case is implemented with codegen.ObjRenderer
 *   which does not deal with graphical information at all, but instead
 *   takes the _DiagramData object given in begin_render() and iterates
 *   over layers and objects to extract textual information.
 *
 * \extends _DiaRenderer
 * \ingroup PyDia
 */
struct _DiaPyRenderer
{
  DiaRenderer parent_instance;

  char*     filename;
  PyObject* self;
  PyObject* diagram_data;
  char*     old_locale;
};

struct _DiaPyRendererClass
{
  DiaRendererClass parent_class;
};

#define PYDIA_RENDERER(renderer) \
	(DIA_PY_RENDERER(renderer)->self)

/* Moved here to avoid Doxygen picking up the wrong definitions */
GType dia_py_renderer_get_type (void) G_GNUC_CONST;


/**
 * begin_render:
 * @renderer: Explicit this pointer
 * @update: The rectangle to update or %NULL for everything
 *
 * Begin rendering with Python
 *
 * The Python side of the begin_render() method has a different signature.
 * It gets passed in a PyDia wrapped #DiagramData object and a filename
 * to store to.
 */
static void
begin_render (DiaRenderer *renderer, const DiaRectangle *update)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  DIA_PY_RENDERER (renderer)->old_locale = setlocale (LC_NUMERIC, "C");

  func = PyObject_GetAttrString (self, "begin_render");
  if (func && PyCallable_Check (func)) {
    Py_INCREF (self);
    Py_INCREF (func);
    arg = Py_BuildValue ("(Os)",
                         DIA_PY_RENDERER (renderer)->diagram_data,
                         DIA_PY_RENDERER (renderer)->filename);
    if (arg) {
      res = PyObject_CallObject (func, arg);
      ON_RES (res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF (func);
    Py_DECREF (self);
  }
}


/**
 * end_render:
 * @renderer: the #DiaRenderer
 *
 * Finalize drawing/exporting
 */
static void
end_render (DiaRenderer *renderer)
{
  PyObject *func, *res, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "end_render");
  if (func && PyCallable_Check (func)) {
    Py_INCREF (self);
    Py_INCREF (func);
    res = PyObject_CallObject (func, (PyObject *) NULL);
    ON_RES (res, FALSE);
    Py_DECREF (func);
    Py_DECREF (self);
  }

  Py_DECREF (DIA_PY_RENDERER (renderer)->diagram_data);
  g_clear_pointer (&(DIA_PY_RENDERER (renderer)->filename), g_free);

  setlocale (LC_NUMERIC, DIA_PY_RENDERER (renderer)->old_locale);
}


/**
 * set_linewidth:
 * @renderer: the #DiaRenderer
 * @linewidth: the linewidth
 *
 * Set linewidth for later use
 *
 * Optional on the PyDia side.
 */
static void
set_linewidth (DiaRenderer *renderer, double linewidth)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "set_linewidth");
  if (func && PyCallable_Check (func)) {
    Py_INCREF (self);
    Py_INCREF (func);
    arg = Py_BuildValue ("(d)", linewidth);
    if (arg) {
      res = PyObject_CallObject (func, arg);
      ON_RES (res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF (func);
    Py_DECREF (self);
  } else {
    /* member optional */
    PyErr_Clear ();
  }
}


/**
 * set_linecaps:
 * @renderer: the #DiaRenderer
 * @mode: #DiaLineCaps
 *
 * Set linecaps for later use
 *
 * Optional on the PyDia side.
 */
static void
set_linecaps (DiaRenderer *renderer, DiaLineCaps mode)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  switch (mode) {
    case DIA_LINE_CAPS_BUTT:
      break;
    case DIA_LINE_CAPS_ROUND:
      break;
    case DIA_LINE_CAPS_PROJECTING:
      break;
    case DIA_LINE_CAPS_DEFAULT:
    default:
      PyErr_Warn (PyExc_RuntimeWarning,
                  "DiaPyRenderer : Unsupported fill mode specified!\n");
  }

  func = PyObject_GetAttrString (self, "set_linecaps");
  if (func && PyCallable_Check (func)) {
    Py_INCREF (self);
    Py_INCREF (func);
    arg = Py_BuildValue ("(i)", mode);
    if (arg) {
      res = PyObject_CallObject (func, arg);
      ON_RES (res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF (func);
    Py_DECREF (self);
  } else {
    /* member optional */
    PyErr_Clear ();
  }
}


/**
 * set_linejoin:
 * @renderer: the #DiaRenderer
 * @mode: the #DiaLineJoin type
 *
 * Set linejoin for later use
 *
 * Optional on the PyDia side.
 */
static void
set_linejoin (DiaRenderer *renderer, DiaLineJoin mode)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  switch (mode) {
    case DIA_LINE_JOIN_MITER:
      break;
    case DIA_LINE_JOIN_ROUND:
      break;
    case DIA_LINE_JOIN_BEVEL:
      break;
    case DIA_LINE_JOIN_DEFAULT:
    default:
      PyErr_Warn (PyExc_RuntimeWarning,
                  "DiaPyRenderer : Unsupported fill mode specified!\n");
  }

  func = PyObject_GetAttrString (self, "set_linejoin");
  if (func && PyCallable_Check (func)) {
    Py_INCREF (self);
    Py_INCREF (func);
    arg = Py_BuildValue ("(i)", mode);
    if (arg) {
      res = PyObject_CallObject (func, arg);
      ON_RES (res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF (func);
    Py_DECREF (self);
  } else {
    /* member optional */
    PyErr_Clear ();
  }
}


/**
 * set_linestyle:
 * @renderer: the #DiaRenderer
 * @mode: the new #DiaLineStyle
 * @dash_length: how often to dash
 *
 * Set linestyle for later use
 *
 * Optional on the PyDia side.
 */
static void
set_linestyle (DiaRenderer *renderer, DiaLineStyle mode, double dash_length)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  /* line type */
  switch (mode) {
    case DIA_LINE_STYLE_SOLID:
      break;
    case DIA_LINE_STYLE_DASHED:
      break;
    case DIA_LINE_STYLE_DASH_DOT:
      break;
    case DIA_LINE_STYLE_DASH_DOT_DOT:
      break;
    case DIA_LINE_STYLE_DOTTED:
      break;
    case DIA_LINE_STYLE_DEFAULT:
    default:
      PyErr_Warn (PyExc_RuntimeWarning,
                  "DiaPyRenderer : Unsupported fill mode specified!\n");
  }

  func = PyObject_GetAttrString (self, "set_linestyle");
  if (func && PyCallable_Check (func)) {
    Py_INCREF (self);
    Py_INCREF (func);
    arg = Py_BuildValue ("(id)", mode, dash_length);
    if (arg) {
      res = PyObject_CallObject (func, arg);
      ON_RES (res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF (func);
    Py_DECREF (self);
  } else {
    /* member optional */
    PyErr_Clear ();
  }
}


/**
 * set_fillstyle:
 * @renderer: the #DiaRenderer
 * @mode: the #DiaFillStyle to fill with
 *
 * Set fillstyle for later use
 *
 * Optional on the PyDia side.
 */
static void
set_fillstyle (DiaRenderer *renderer, DiaFillStyle mode)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  switch (mode) {
    case DIA_FILL_STYLE_SOLID:
      break;
    default:
      PyErr_Warn (PyExc_RuntimeWarning,
                  "DiaPyRenderer : Unsupported fill mode specified!\n");
  }

  func = PyObject_GetAttrString (self, "set_fillstyle");
  if (func && PyCallable_Check (func)) {
    Py_INCREF (self);
    Py_INCREF (func);
    arg = Py_BuildValue ("(i)", mode);
    if (arg) {
      res = PyObject_CallObject (func, arg);
      ON_RES (res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF (func);
    Py_DECREF (self);
  } else {
    /* member optional */
    PyErr_Clear ();
  }
}


static gpointer parent_class = NULL;

/*!
 * \brief Advertise the renderer's capabilities
 * \memberof _DiaTransformRenderer
 */
static gboolean
is_capable_to (DiaRenderer *renderer, RenderCapability cap)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);
  gboolean bRet = FALSE;

  func = PyObject_GetAttrString (self, "is_capable_to");
  if (func && PyCallable_Check (func)) {
    Py_INCREF (self);
    Py_INCREF (func);
    arg = Py_BuildValue ("(i)", cap);
    if (arg) {
      res = PyObject_CallObject (func, arg);
      if (res && PyLong_Check (res)) {
        bRet = (PyLong_AsLong (res) != 0);
        Py_DECREF (res);
      } else {
        ON_RES (res, FALSE);
      }
    }
    Py_XDECREF (arg);
    Py_DECREF (func);
    Py_DECREF (self);
  } else {
    PyErr_Clear (); /* member optional */
    return DIA_RENDERER_CLASS (parent_class)->is_capable_to (renderer, cap);
  }
  return bRet;
}


/**
 * draw_layer:
 * @renderer: explicit this pointer
 * @layer: layer to draw
 * @active: %TRUE if it is the currently active layer
 * @update: the update rectangle, NULL for unlimited
 *
 * Render all the visible object in the layer
 */
static void
draw_layer (DiaRenderer  *renderer,
            DiaLayer     *layer,
            gboolean      active,
            DiaRectangle *update)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "draw_layer");
  if (func && PyCallable_Check (func)) {
    PyObject *olayer = PyDiaLayer_New (layer);
    PyObject *orect;

    Py_INCREF (self);
    Py_INCREF (func);
    if (update) {
      orect = PyDiaRectangle_New (update);
    } else {
      Py_INCREF (Py_None);
      orect = Py_None;
    }
    arg = Py_BuildValue ("(OiO)", olayer, active, orect);
    if (arg) {
      res = PyObject_CallObject (func, arg);
      ON_RES (res, FALSE);
    }
    Py_XDECREF (olayer);
    Py_XDECREF (orect);
    Py_DECREF (func);
    Py_DECREF (self);
  } else {
    /* member optional */
    PyErr_Clear ();
    /* have to call the base class */
    DIA_RENDERER_CLASS (parent_class)->draw_layer (renderer, layer, active, update);
  }
}


/**
 * draw_object:
 * @renderer: Self
 * @object: The object to draw
 * @matrix: The transformation matrix to use or NULL for no transformation
 *
 * Draw object
 *
 * Optional on the PyDia side. If not implemented the base class method
 * will be called.
 *
 * Intercepting this method on the Python side allows to create per
 * object information in the drawing. It is also necessary if the PyDia
 * renderer should support transformations.
 *
 * If implementing a drawing export filter and overwriting draw_object()
 * the following code shall be used. Otherwise no draw/fill method will
 * be called at all.
 *
 * |[<!-- language="Python" -->
 * # don't forget to render the object
 * object.draw (self)
 * ]|
 *
 * Not calling the object draw method is only useful when a non-drawing
 * export - e.g. code generation \sa codegen.py - is implemented.
 */
static void
draw_object (DiaRenderer *renderer, DiaObject *object, DiaMatrix *matrix)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "draw_object");
  if (func && PyCallable_Check (func)) {
    PyObject *oobj = PyDiaObject_New (object);
    PyObject *mat = NULL;

    Py_INCREF (self);
    Py_INCREF (func);
    if (matrix)
      mat = PyDiaMatrix_New (matrix);
    else {
      Py_INCREF (Py_None);
      mat = Py_None;
    }
    arg = Py_BuildValue ("(OO)", oobj, mat);
    if (arg) {
      res = PyObject_CallObject (func, arg);
      ON_RES (res, FALSE);
    }
    Py_XDECREF (arg);
    Py_XDECREF (oobj);
    Py_XDECREF (mat);
    Py_DECREF (func);
    Py_DECREF (self);
  } else { /* member optional */
    PyErr_Clear();
    /* but should still call the base class */
    DIA_RENDERER_CLASS (parent_class)->draw_object (renderer, object, matrix);
  }
}


/**
 * draw_line:
 * @renderer: the #DiaRenderer
 * @start: the #Point the line starts
 * @end: the end #Point
 * @line_colour: what #Color to use
 *
 * Draw line
 *
 * Not optional on the PyDia side. If not implemented a runtime warning
 * will be generated when called.
 */
static void
draw_line (DiaRenderer *renderer,
           Point       *start,
           Point       *end,
           Color       *line_colour)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "draw_line");
  if (func && PyCallable_Check (func)) {
    PyObject *ostart = PyDiaPoint_New (start);
    PyObject *oend = PyDiaPoint_New (end);
    PyObject *ocolor = PyDiaColor_New (line_colour);

    Py_INCREF (self);
    Py_INCREF (func);
    arg = Py_BuildValue ("(OOO)", ostart, oend, ocolor);
    if (arg) {
      res = PyObject_CallObject (func, arg);
      ON_RES (res, FALSE);
    }
    Py_XDECREF (arg);
    Py_XDECREF (ostart);
    Py_XDECREF (oend);
    Py_XDECREF (ocolor);
    Py_DECREF (func);
    Py_DECREF (self);
  } else { /* member not optional */
    char *msg = g_strdup_printf ("%s.draw_line() implementation missing.",
                                 G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (renderer)));
    PyErr_Clear ();
    PyErr_Warn (PyExc_RuntimeWarning, msg);
    g_clear_pointer (&msg, g_free);
  }
}


/**
 * draw_polyline:
 * @renderer: the #DiaRenderer
 * @points: the #Points of the line
 * @num_points: the length of @points
 * @line_colour: the #Color of the line
 *
 * Draw polyline
 *
 * Optional on the PyDia side. If not implemented fallback to base class member.
 */
static void
draw_polyline (DiaRenderer *renderer,
               Point       *points,
               int          num_points,
               Color       *line_colour)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "draw_polyline");
  if (func && PyCallable_Check (func)) {
    PyObject *optt = PyDiaPointTuple_New (points, num_points);
    PyObject *ocolor = PyDiaColor_New (line_colour);

    Py_INCREF (self);
    Py_INCREF (func);
    arg = Py_BuildValue ("(OO)", optt, ocolor);
    if (arg) {
      res = PyObject_CallObject (func, arg);
      ON_RES (res, FALSE);
    }
    Py_XDECREF (arg);
    Py_XDECREF (optt);
    Py_XDECREF (ocolor);
    Py_DECREF (func);
    Py_DECREF (self);
  } else {
    /* member optional */
    PyErr_Clear ();
    /* XXX: implementing the same fallback as DiaRenderer */
    DIA_RENDERER_CLASS (parent_class)->draw_polyline (renderer, points, num_points, line_colour);
  }
}


/**
 * draw_polygon:
 * @renderer: the #DiaRenderer
 * @points: the polygon #Points
 * @num_points: the length of @points
 * @fill: the polygon file #Color
 * @stroke: outline the polyon in #Color
 *
 * Draw polygon
 *
 * Optional on the PyDia side. If not implemented fallback to base class member.
 */
static void
draw_polygon (DiaRenderer *renderer,
              Point       *points,
              int          num_points,
              Color       *fill,
              Color       *stroke)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "draw_polygon");
  if (func && PyCallable_Check (func)) {
    PyObject *optt = PyDiaPointTuple_New (points, num_points);
    PyObject *fill_po, *stroke_po;

    if (fill) {
      fill_po = PyDiaColor_New (fill);
    } else {
      Py_INCREF (Py_None), fill_po = Py_None;
    }

    if (stroke) {
      stroke_po = PyDiaColor_New (stroke);
    } else {
      Py_INCREF (Py_None), stroke_po = Py_None;
    }

    Py_INCREF (self);
    Py_INCREF (func);
    arg = Py_BuildValue ("(OOO)", optt, fill_po, stroke_po);
    if (arg) {
      res = PyObject_CallObject (func, arg);
      ON_RES (res, FALSE);
    }
    Py_XDECREF (arg);
    Py_XDECREF (optt);
    Py_XDECREF (fill_po);
    Py_XDECREF (stroke_po);
    Py_DECREF (func);
    Py_DECREF (self);
  } else {
    /* fill_polygon was not an optional member */
    PyErr_Warn (PyExc_RuntimeWarning,
                "DiaPyRenderer : draw_polygon() method missing!\n");
  }
}


static void
draw_rect (DiaRenderer *renderer,
           Point       *ul_corner,
           Point       *lr_corner,
           Color       *fill,
           Color       *stroke)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "draw_rect");
  if (func && PyCallable_Check (func)) {
    PyObject *orect = PyDiaRectangle_New_FromPoints (ul_corner, lr_corner);
    PyObject *fill_po, *stroke_po;
    Py_INCREF (self);
    Py_INCREF (func);

    if (fill) {
      fill_po = PyDiaColor_New (fill);
    } else {
      Py_INCREF (Py_None), fill_po = Py_None;
    }

    if (stroke) {
      stroke_po = PyDiaColor_New (stroke);
    } else {
      Py_INCREF (Py_None), stroke_po = Py_None;
    }

    arg = Py_BuildValue ("(OOO)", orect, fill_po, stroke_po);
    if (arg) {
      res = PyObject_CallObject (func, arg);
      ON_RES (res, FALSE);
    }
    Py_XDECREF (arg);
    Py_XDECREF (orect);
    Py_XDECREF (fill_po);
    Py_XDECREF (stroke_po);
    Py_DECREF (func);
    Py_DECREF (self);
  } else {
    /* member optional */
    PyErr_Clear();
    /* XXX: implementing the same fallback as DiaRenderer would do */
    DIA_RENDERER_CLASS (parent_class)->draw_rect(renderer, ul_corner, lr_corner, fill, stroke);
  }
}


static void
draw_rounded_rect (DiaRenderer *renderer,
                   Point       *ul_corner,
                   Point       *lr_corner,
                   Color       *fill,
                   Color       *stroke,
                   double       rounding)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "draw_rounded_rect");
  if (func && PyCallable_Check (func)) {
    PyObject *orect = PyDiaRectangle_New_FromPoints (ul_corner, lr_corner);
    PyObject *fill_po, *stroke_po;

    Py_INCREF (self);
    Py_INCREF (func);
    if (fill) {
      fill_po = PyDiaColor_New (fill);
    } else {
      Py_INCREF (Py_None), fill_po = Py_None;
    }

    if (stroke) {
      stroke_po = PyDiaColor_New (stroke);
    } else {
      Py_INCREF (Py_None), stroke_po = Py_None;
    }

    arg = Py_BuildValue ("(OOOd)", orect, fill_po, stroke_po, rounding);
    if (arg) {
      res = PyObject_CallObject (func, arg);
      ON_RES (res, FALSE);
    }
    Py_XDECREF (arg);
    Py_XDECREF (fill_po);
    Py_XDECREF (stroke_po);
    Py_XDECREF (orect);
    Py_DECREF (func);
    Py_DECREF (self);
  } else {
    /* member optional */
    PyErr_Clear ();
    /* implementing the same fallback as DiaRenderer would do */
    DIA_RENDERER_CLASS (parent_class)->draw_rounded_rect (renderer,
                                                          ul_corner,
                                                          lr_corner,
                                                          fill,
                                                          stroke,
                                                          rounding);
  }
}


static void
draw_arc (DiaRenderer *renderer,
          Point       *center,
          double       width,
          double       height,
          double       angle1,
          double       angle2,
          Color       *colour)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "draw_arc");
  if (func && PyCallable_Check (func)) {
    PyObject *opoint = PyDiaPoint_New (center);
    PyObject *ocolor = PyDiaColor_New (colour);

    Py_INCREF (self);
    Py_INCREF (func);
    arg = Py_BuildValue ("(OddddO)",
                         opoint,
                         width,
                         height,
                         angle1,
                         angle2,
                         ocolor);
    if (arg) {
      res = PyObject_CallObject (func, arg);
      ON_RES (res, FALSE);
    }
    Py_XDECREF (arg);
    Py_XDECREF (opoint);
    Py_XDECREF (ocolor);
    Py_DECREF (func);
    Py_DECREF (self);
  } else { /* member not optional */
    char *msg = g_strdup_printf ("%s.draw_arc() implementation missing.",
                                 G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (renderer)));
    PyErr_Clear();
    PyErr_Warn (PyExc_RuntimeWarning, msg);
    g_clear_pointer (&msg, g_free);
  }
}


/**
 * fill_arc:
 * @renderer: the #DiaRenderer
 *
 *
 * Fill arc
 *
 * Not optional on the PyDia side. If not implemented a runtime warning
 * will be generated when called.
 */
static void
fill_arc (DiaRenderer *renderer,
          Point       *center,
          double       width,
          double       height,
          double       angle1,
          double       angle2,
          Color       *colour)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "fill_arc");
  if (func && PyCallable_Check (func)) {
    PyObject *opoint = PyDiaPoint_New (center);
    PyObject *ocolor = PyDiaColor_New (colour);

    Py_INCREF (self);
    Py_INCREF (func);
    arg = Py_BuildValue ("(OddddO)",
                         opoint,
                         width,
                         height,
                         angle1,
                         angle2,
                         ocolor);
    if (arg) {
      res = PyObject_CallObject (func, arg);
      ON_RES (res, FALSE);
    }
    Py_XDECREF (arg);
    Py_XDECREF (opoint);
    Py_XDECREF (ocolor);
    Py_DECREF (func);
    Py_DECREF (self);
  } else { /* member not optional */
    char *msg = g_strdup_printf ("%s.fill_arc() implementation missing.",
                                 G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (renderer)));
    PyErr_Clear ();
    PyErr_Warn (PyExc_RuntimeWarning, msg);
    g_clear_pointer (&msg, g_free);
  }
}


/**
 * draw_ellipse:
 *
 * Draw ellipse
 *
 * Not optional on the PyDia side. If not implemented a runtime warning
 * will be generated when called.
 */
static void
draw_ellipse (DiaRenderer *renderer,
              Point       *center,
              double       width,
              double       height,
              Color       *fill,
              Color       *stroke)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "draw_ellipse");
  if (func && PyCallable_Check (func)) {
    PyObject *opoint = PyDiaPoint_New (center);
    PyObject *fill_po;
    PyObject *stroke_po;

    Py_INCREF (self);
    Py_INCREF (func);

    /* we have to provide a Python object even if there is no color */
    if (fill) {
      fill_po = PyDiaColor_New (fill);
    } else {
      Py_INCREF (Py_None), fill_po = Py_None;
    }

    if (stroke) {
      stroke_po = PyDiaColor_New (stroke);
    } else {
      Py_INCREF (Py_None), stroke_po = Py_None;
    }

    arg = Py_BuildValue ("(OddOO)", opoint, width, height, fill_po, stroke_po);
    if (arg) {
      res = PyObject_CallObject (func, arg);
      ON_RES (res, FALSE);
    }
    Py_XDECREF (arg);
    Py_XDECREF (opoint);
    Py_XDECREF (fill_po);
    Py_XDECREF (stroke_po);
    Py_DECREF (func);
    Py_DECREF (self);
  } else { /* member not optional */
    char *msg = g_strdup_printf ("%s.draw_ellipse() implementation missing.",
                                 G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (renderer)));
    PyErr_Clear ();
    PyErr_Warn (PyExc_RuntimeWarning, msg);
    g_clear_pointer (&msg, g_free);
  }
}


static void
draw_bezier (DiaRenderer *renderer,
             BezPoint    *points,
             int          num_points,
             Color       *colour)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "draw_bezier");
  if (func && PyCallable_Check (func)) {
    PyObject *obt = PyDiaBezPointTuple_New (points, num_points);
    PyObject *ocolor = PyDiaColor_New (colour);
    Py_INCREF (self);
    Py_INCREF (func);
    arg = Py_BuildValue ("(OO)", obt, ocolor);
    if (arg) {
      res = PyObject_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);
    Py_XDECREF (obt);
    Py_XDECREF (ocolor);
    Py_DECREF (func);
    Py_DECREF (self);
  } else {
    /* member optional */
    PyErr_Clear ();
    /* XXX: implementing the same fallback as DiaRenderer would do */
    DIA_RENDERER_CLASS (parent_class)->draw_bezier (renderer, points, num_points, colour);
  }
}


/* kept for backward compatibility, not any longer in the renderer interface */
static void
fill_bezier (DiaRenderer *renderer,
             BezPoint    *points, /* Last point must be same as first point */
             int          num_points,
             Color       *colour)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "fill_bezier");
  if (func && PyCallable_Check (func)) {
    PyObject *obt = PyDiaBezPointTuple_New (points, num_points);
    PyObject *ocolor = PyDiaColor_New (colour);
    Py_INCREF (self);
    Py_INCREF (func);
    arg = Py_BuildValue ("(OO)", obt, ocolor);
    if (arg) {
      res = PyObject_CallObject (func, arg);
      ON_RES (res, FALSE);
    }
    Py_XDECREF (arg);
    Py_XDECREF (obt);
    Py_XDECREF (ocolor);
    Py_DECREF (func);
    Py_DECREF (self);
  } else {
    /* member optional */
    PyErr_Clear ();
    /* XXX: implementing the same fallback as DiaRenderer would do */
    DIA_RENDERER_CLASS (parent_class)->draw_beziergon (renderer, points, num_points, colour, NULL);
  }
}


/**
 * draw_beziergon:
 *
 * Fill and/or stroke a closed path
 */
static void
draw_beziergon (DiaRenderer *renderer,
                BezPoint    *points,
                int          num_points,
                Color       *fill,
                Color       *stroke)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "draw_beziergon");
  if (func && PyCallable_Check (func)) {
    PyObject *obt = PyDiaBezPointTuple_New (points, num_points);
    PyObject *fill_po;
    PyObject *stroke_po;
    Py_INCREF (self);
    Py_INCREF (func);

    /* we have to provide a Python object even if there is no color */
    if (fill) {
      fill_po = PyDiaColor_New (fill);
    } else {
      Py_INCREF (Py_None), fill_po = Py_None;
    }

    if (stroke) {
      stroke_po = PyDiaColor_New (stroke);
    } else {
      Py_INCREF(Py_None), stroke_po = Py_None;
    }

    arg = Py_BuildValue ("(OOO)", obt, fill_po, stroke_po);
    if (arg) {
      res = PyObject_CallObject (func, arg);
      ON_RES (res, FALSE);
    }
    Py_XDECREF (arg);
    Py_XDECREF (obt);
    Py_XDECREF (fill_po);
    Py_XDECREF (stroke_po);
    Py_DECREF (func);
    Py_DECREF (self);
  } else { /* member optional */
    PyErr_Clear ();
    /* PyDia only backward compatibility */
    if (fill) {
      fill_bezier (renderer, points, num_points, fill);
    }
    if (stroke) {
      /* XXX: still not closing */
      draw_bezier (renderer, points, num_points, stroke);
    }
  }
}


/**
 * draw_string:
 * @renderer: the #DiaRenderer
 * @text: the string to draw
 * @pos: #Point where to draw @text
 * @alignment: the @text #DiaAlignment
 * @colour: the #Color of @text
 *
 * Draw string
 *
 * Not optional on the PyDia side. If not implemented a runtime warning
 * will be generated when called.
 */
static void
draw_string (DiaRenderer  *renderer,
             const char   *text,
             Point        *pos,
             DiaAlignment  alignment,
             Color        *colour)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  switch (alignment) {
    case DIA_ALIGN_LEFT:
      break;
    case DIA_ALIGN_CENTRE:
      break;
    case DIA_ALIGN_RIGHT:
      break;
    default:
      break;
  }

  func = PyObject_GetAttrString (self, "draw_string");
  if (func && PyCallable_Check (func)) {
    PyObject *opoint = PyDiaPoint_New (pos);
    PyObject *ocolor = PyDiaColor_New (colour);

    Py_INCREF (self);
    Py_INCREF (func);
    arg = Py_BuildValue ("(sOiO)", text, opoint, alignment, ocolor);
    if (arg) {
      res = PyObject_CallObject (func, arg);
      ON_RES (res, FALSE);
    }
    Py_XDECREF (arg);
    Py_XDECREF (opoint);
    Py_XDECREF (ocolor);
    Py_DECREF (func);
    Py_DECREF (self);
  } else { /* member not optional */
    char *msg = g_strdup_printf ("%s.draw_string() implementation missing.",
                                 G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (renderer)));
    PyErr_Clear ();
    PyErr_Warn (PyExc_RuntimeWarning, msg);
    g_clear_pointer (&msg, g_free);
  }
}


/**
 * draw_image:
 * @renderer: the #DiaRenderer
 * @point: the #Point to draw @image
 * @width: @image width
 * @height: height of @image
 * @image: the #DiaImage to draw
 *
 * Draw image
 *
 * Not optional on the PyDia side. If not implemented a runtime warning
 * will be generated when called.
 */
static void
draw_image (DiaRenderer *renderer,
            Point       *point,
            double       width,
            double       height,
            DiaImage    *image)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "draw_image");
  if (func && PyCallable_Check (func)) {
    PyObject *opoint = PyDiaPoint_New (point);
    PyObject *oimage = PyDiaImage_New (image);

    Py_INCREF (self);
    Py_INCREF (func);
    arg = Py_BuildValue ("(OddO)", opoint, width, height, oimage);
    if (arg) {
      res = PyObject_CallObject (func, arg);
      ON_RES (res, FALSE);
    }
    Py_XDECREF (arg);
    Py_XDECREF (opoint);
    Py_XDECREF (oimage);
    Py_DECREF (func);
    Py_DECREF (self);
  } else { /* member not optional */
    char *msg = g_strdup_printf ("%s.draw_image() implementation missing.",
                                 G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (renderer)));
    PyErr_Clear ();
    PyErr_Warn (PyExc_RuntimeWarning, msg);
    g_clear_pointer (&msg, g_free);
  }
}


gboolean
PyDia_export_data (DiagramData *data,
                   DiaContext  *ctx,
                   const char  *filename,
                   const char  *diafilename,
                   void        *user_data)
{
  DiaPyRenderer *renderer;

  {
    FILE *file;
    file = g_fopen(filename, "w"); /* "wb" for binary! */

    if (file == NULL) {
      dia_context_add_message_with_errno(ctx, errno, _("Couldn't open '%s' for writing.\n"),
				         dia_context_get_filename(ctx));
      return FALSE;
    }
    else
      fclose (file);
  }

  renderer = g_object_new (DIA_TYPE_PY_RENDERER, NULL);

  renderer->filename = g_strdup (filename);
  renderer->diagram_data = PyDiaDiagramData_New (data);

  /* The Python Renderer object was created at PyDia_Register */
  renderer->self = (PyObject*) user_data;

  /* this will call the required callback functions above */
  data_render (data, DIA_RENDERER (renderer), NULL, NULL, NULL);

  g_clear_object (&renderer);

  return TRUE;
}


DiaRenderer *
PyDia_new_renderer_wrapper (PyObject *self)
{
  DiaPyRenderer *wrapper;

  wrapper = g_object_new (DIA_TYPE_PY_RENDERER, NULL);
  wrapper->self = self;

  return DIA_RENDERER (wrapper);
}

/*
 * GObject boiler plate
 */
static void dia_py_renderer_class_init (DiaPyRendererClass *klass);

GType
dia_py_renderer_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (DiaPyRendererClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) dia_py_renderer_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (DiaPyRenderer),
        0,              /* n_preallocs */
	NULL            /* init */
      };

      object_type = g_type_register_static (DIA_TYPE_RENDERER,
                                            "DiaPyRenderer",
                                            &object_info, 0);
    }

  return object_type;
}

static void
dia_py_renderer_finalize (GObject *object)
{
  DiaPyRenderer *renderer = DIA_PY_RENDERER (object);

  g_clear_pointer (&renderer->filename, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
dia_py_renderer_class_init (DiaPyRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = dia_py_renderer_finalize;

  /* all defined members from above */
  /* renderer members */
  renderer_class->begin_render = begin_render;
  renderer_class->end_render   = end_render;

  renderer_class->draw_layer = draw_layer;
  renderer_class->draw_object  = draw_object;

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
  renderer_class->draw_rect      = draw_rect;
  renderer_class->draw_polyline  = draw_polyline;

  renderer_class->draw_bezier   = draw_bezier;
  renderer_class->draw_beziergon = draw_beziergon;

  /* highest level functions */
  renderer_class->draw_rounded_rect = draw_rounded_rect;
  /* other */
  renderer_class->is_capable_to = is_capable_to;
}

