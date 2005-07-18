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

#include <config.h>

#include <Python.h>
#include <glib.h>

#include <locale.h>

#include "intl.h"
#include "message.h"
#include "geometry.h"

#include "pydia-object.h" /* for PyObject_HEAD_INIT */
#include "pydia-export.h"
#include "pydia-diagramdata.h"
#include "pydia-geometry.h"
#include "pydia-color.h"
#include "pydia-font.h"
#include "pydia-image.h"
#include "pydia-error.h"

/*
 * The PyDiaRenderer is currently defined in Python only. The
 * DiaPyRenderer is using it's interface to map the gobject
 * DiaRenderer to it. A next step could be to let Python code
 * directly inherit from DiaPyRenderer.
 * To do that probably some code from pygtk.gobject needs to
 * be borrowed/shared
 */
#include "diarenderer.h"

#define DIA_TYPE_PY_RENDERER           (dia_py_renderer_get_type ())
#define DIA_PY_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_PY_RENDERER, DiaPyRenderer))
#define DIA_PY_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_PY_RENDERER, DiaPyRendererClass))
#define DIA_IS_PY_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_PY_RENDERER))
#define DIA_PY_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_PY_RENDERER, DiaPyRendererClass))

GType dia_py_renderer_get_type (void) G_GNUC_CONST;

typedef struct _DiaPyRenderer DiaPyRenderer;
typedef struct _DiaPyRendererClass DiaPyRendererClass;

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

/*
 * Members overwritable by Python scripts
 */
static void
begin_render(DiaRenderer *renderer)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  DIA_PY_RENDERER(renderer)->old_locale = setlocale(LC_NUMERIC, "C");

  func = PyObject_GetAttrString (self, "begin_render");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(Os)", 
                         DIA_PY_RENDERER(renderer)->diagram_data, 
                         DIA_PY_RENDERER(renderer)->filename);
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF (func);
    Py_DECREF (self);
  }
}

static void
end_render(DiaRenderer *renderer)
{
  PyObject *func, *res, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "end_render");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    res = PyEval_CallObject (func, (PyObject *)NULL);
    ON_RES(res, FALSE);
    Py_DECREF(func);
    Py_DECREF(self);
  }

  Py_DECREF (DIA_PY_RENDERER(renderer)->diagram_data);
  g_free (DIA_PY_RENDERER(renderer)->filename);

  setlocale(LC_NUMERIC, DIA_PY_RENDERER(renderer)->old_locale);
}

static void
set_linewidth(DiaRenderer *renderer, real linewidth)
{  
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "set_linewidth");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(d)", linewidth);
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_DECREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
set_linecaps(DiaRenderer *renderer, LineCaps mode)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  switch(mode) {
  case LINECAPS_BUTT:
    break;
  case LINECAPS_ROUND:
    break;
  case LINECAPS_PROJECTING:
    break;
  default:
    message_error("DiaPyRenderer : Unsupported fill mode specified!\n");
  }

  func = PyObject_GetAttrString (self, "set_linecaps");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(i)", mode);
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_DECREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
set_linejoin(DiaRenderer *renderer, LineJoin mode)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  switch(mode) {
    case LINEJOIN_MITER:
    break;
  case LINEJOIN_ROUND:
    break;
  case LINEJOIN_BEVEL:
    break;
  default:
    message_error("DiaPyRenderer : Unsupported fill mode specified!\n");
  }

  func = PyObject_GetAttrString (self, "set_linejoin");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(i)", mode);
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_DECREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
set_linestyle(DiaRenderer *renderer, LineStyle mode)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  /* line type */
  switch (mode) {
  case LINESTYLE_SOLID:
    break;
  case LINESTYLE_DASHED:
    break;
  case LINESTYLE_DASH_DOT:
    break;
  case LINESTYLE_DASH_DOT_DOT:
    break;
  case LINESTYLE_DOTTED:
    break;
  default:
    message_error("DiaPyRenderer : Unsupported fill mode specified!\n");
  }

  func = PyObject_GetAttrString (self, "set_linestyle");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(i)", mode);
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_DECREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
set_dashlength(DiaRenderer *renderer, real length)
{  
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "set_dashlength");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(d)", length);
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_DECREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
set_fillstyle(DiaRenderer *renderer, FillStyle mode)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  switch(mode) {
  case FILLSTYLE_SOLID:
    break;
  default:
    message_error("DiaPyRenderer : Unsupported fill mode specified!\n");
  }

  func = PyObject_GetAttrString (self, "set_fillstyle");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(i)", mode);
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_DECREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
set_font(DiaRenderer *renderer, DiaFont *font, real height)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "set_font");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(Od)", PyDiaFont_New (font), height);
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_DECREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
draw_line(DiaRenderer *renderer, 
          Point *start, Point *end, 
          Color *line_colour)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "draw_line");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(OOO)", PyDiaPoint_New (start),
                                  PyDiaPoint_New (end),
                                  PyDiaColor_New (line_colour));
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_DECREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
draw_polyline(DiaRenderer *renderer, 
	      Point *points, int num_points, 
	      Color *line_colour)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "draw_polyline");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(OO)", PyDiaPointTuple_New (points, num_points),
                                 PyDiaColor_New (line_colour));
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_DECREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
draw_polygon(DiaRenderer *renderer, 
	     Point *points, int num_points, 
	     Color *line_colour)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "draw_polygon");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(OO)", PyDiaPointTuple_New (points, num_points),
                                 PyDiaColor_New (line_colour));
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_DECREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
fill_polygon(DiaRenderer *renderer, 
	     Point *points, int num_points, 
	     Color *colour)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "fill_polygon");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(OO)", PyDiaPointTuple_New (points, num_points),
                                 PyDiaColor_New (colour));
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_DECREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
draw_rect(DiaRenderer *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *colour)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "draw_rect");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(OO)", PyDiaRectangle_New_FromPoints (ul_corner, lr_corner),
                                 PyDiaColor_New (colour));
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_DECREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
fill_rect(DiaRenderer *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *colour)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "fill_rect");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(OO)", PyDiaRectangle_New_FromPoints (ul_corner, lr_corner),
                                 PyDiaColor_New (colour));
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_DECREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
draw_arc(DiaRenderer *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "draw_arc");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(OddddO)", PyDiaPoint_New (center),
                                     width, height, angle1, angle2,
                                     PyDiaColor_New (colour));
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_DECREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
fill_arc(DiaRenderer *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "fill_arc");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(OddddO)", PyDiaPoint_New (center),
                                     width, height, angle1, angle2,
                                     PyDiaColor_New (colour));
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_DECREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
draw_ellipse(DiaRenderer *renderer, 
	     Point *center,
	     real width, real height,
	     Color *colour)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "draw_ellipse");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(OddO)", PyDiaPoint_New (center),
                                   width, height,
                                   PyDiaColor_New (colour));
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_DECREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
fill_ellipse(DiaRenderer *renderer, 
	     Point *center,
	     real width, real height,
	     Color *colour)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "fill_ellipse");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(OddO)", PyDiaPoint_New (center),
                                   width, height,
                                   PyDiaColor_New (colour));
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_DECREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
draw_bezier(DiaRenderer *renderer, 
	    BezPoint *points,
	    int num_points,
	    Color *colour)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "draw_bezier");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(OO)", PyDiaBezPointTuple_New (points, num_points),
                                 PyDiaColor_New (colour));
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_DECREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
fill_bezier(DiaRenderer *renderer, 
	    BezPoint *points, /* Last point must be same as first point */
	    int num_points,
	    Color *colour)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "fill_bezier");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(OO)", PyDiaBezPointTuple_New (points, num_points),
                                 PyDiaColor_New (colour));
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_DECREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
draw_string(DiaRenderer *renderer,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *colour)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);
  int len;

  switch (alignment) {
  case ALIGN_LEFT:
    break;
  case ALIGN_CENTER:
    break;
  case ALIGN_RIGHT:
    break;
  }
  /* work out size of first chunk of text */
  len = strlen(text);

  func = PyObject_GetAttrString (self, "draw_string");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(sOiO)", text,
                                   PyDiaPoint_New (pos),
                                   alignment,
                                   PyDiaColor_New (colour));
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_DECREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
draw_image(DiaRenderer *renderer,
	   Point *point,
	   real width, real height,
	   DiaImage image)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "draw_image");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(OddO)", PyDiaPoint_New (point),
                                   width, height,
                                   PyDiaImage_New (image));
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_DECREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

void
PyDia_export_data(DiagramData *data, const gchar *filename, 
                  const gchar *diafilename, void* user_data)
{
  DiaPyRenderer *renderer;

  {
    FILE *file;
    file = fopen(filename, "w"); /* "wb" for binary! */

    if (file == NULL) {
      message_error(_("Couldn't open '%s' for writing.\n"), 
		    dia_message_filename(filename));
      return;
    }
    else
      fclose (file);
  }

  renderer = g_object_new (DIA_TYPE_PY_RENDERER, NULL);

  renderer->filename = g_strdup (filename);
  renderer->diagram_data = PyDiaDiagramData_New(data);

  /* The Python Renderer object was created at PyDia_Register */
  renderer->self = (PyObject*)user_data;

  /* this will call the required callback functions above */
  data_render(data, DIA_RENDERER(renderer), NULL, NULL, NULL);

  g_object_unref(renderer);
}

/*
 * GObject boiler plate
 */
static void dia_py_renderer_class_init (DiaPyRendererClass *klass);

static gpointer parent_class = NULL;

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

  if (renderer->filename)
    g_free (renderer->filename);

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

  renderer_class->set_linewidth  = set_linewidth;
  renderer_class->set_linecaps   = set_linecaps;
  renderer_class->set_linejoin   = set_linejoin;
  renderer_class->set_linestyle  = set_linestyle;
  renderer_class->set_dashlength = set_dashlength;
  renderer_class->set_fillstyle  = set_fillstyle;

  renderer_class->set_font  = set_font;

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
  renderer_class->fill_bezier   = fill_bezier;
}

