#include <glib.h>

#include "config.h"

#include "intl.h"
#include "message.h"
#include "geometry.h"
#include "render.h"

#include "pydia-object.h" /* for PyObject_HEAD_INIT */
#include "pydia-export.h"
#include "pydia-diagramdata.h"
#include "pydia-geometry.h"
#include "pydia-color.h"
#include "pydia-font.h"
#include "pydia-image.h"
#include "pydia-error.h"

/*
 * The PyDiaRenderer is currently defined in Python only. This file
 * is using it's interface. Probably the Renderer should be implemented
 * as Extension Class ... 
 */

#define MY_RENDERER_NAME "PyDiaRenderer"

typedef struct _MyRenderer MyRenderer;
struct _MyRenderer {
    Renderer  renderer;
    char*     filename;
    PyObject* self; 
    PyObject* diagram_data; 
};

#define PYDIA_RENDERER(renderer) (renderer->self)

/* include function declares and render object "vtable" */
#include "../renderer.inc"

#define ON_RES(r) \
if (!r) { \
  PyObject *exc, *v, *tb, *ef; \
  PyErr_Fetch (&exc, &v, &tb); \
  PyErr_NormalizeException(&exc, &v, &tb); \
\
  ef = PyDiaError_New(MY_RENDERER_NAME " Error:", FALSE); \
  PyFile_WriteObject (exc, ef, 0); \
  PyFile_WriteObject (v, ef, 0); \
  PyTraceBack_Print(tb, ef); \
\
  Py_DECREF (ef); \
  Py_XDECREF(exc); \
  Py_XDECREF(v); \
  Py_XDECREF(tb); \
} \
else \
  Py_DECREF (res)
      
static void
begin_render(MyRenderer *renderer, DiagramData *data)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "begin_render");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(Os)", renderer->diagram_data, renderer->filename);
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res);
    }
    Py_XDECREF (arg);
    Py_DECREF (func);
    Py_INCREF (self);
  }
}

static void
end_render(MyRenderer *renderer)
{
  PyObject *func, *res, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "end_render");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    res = PyEval_CallObject (func, (PyObject *)NULL);
    ON_RES(res);
    Py_DECREF(func);
    Py_INCREF(self);
  }

  Py_DECREF (renderer->diagram_data);
  g_free (renderer->filename);
}

static void
set_linewidth(MyRenderer *renderer, real linewidth)
{  
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "set_linewidth");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(d)", linewidth);
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_INCREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
set_linecaps(MyRenderer *renderer, LineCaps mode)
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
    message_error(MY_RENDERER_NAME ": Unsupported fill mode specified!\n");
  }

  func = PyObject_GetAttrString (self, "set_linecaps");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(i)", mode);
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_INCREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
set_linejoin(MyRenderer *renderer, LineJoin mode)
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
    message_error(MY_RENDERER_NAME ": Unsupported fill mode specified!\n");
  }

  func = PyObject_GetAttrString (self, "set_linejoin");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(i)", mode);
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_INCREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
set_linestyle(MyRenderer *renderer, LineStyle mode)
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
    message_error(MY_RENDERER_NAME ": Unsupported fill mode specified!\n");
  }

  func = PyObject_GetAttrString (self, "set_linestyle");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(i)", mode);
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_INCREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
set_dashlength(MyRenderer *renderer, real length)
{  
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "set_dashlength");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(d)", length);
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_INCREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
set_fillstyle(MyRenderer *renderer, FillStyle mode)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  switch(mode) {
  case FILLSTYLE_SOLID:
    break;
  default:
    message_error(MY_RENDERER_NAME ": Unsupported fill mode specified!\n");
  }

  func = PyObject_GetAttrString (self, "set_fillstyle");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(i)", mode);
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_INCREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
set_font(MyRenderer *renderer, Font *font, real height)
{
  PyObject *func, *res, *arg, *self = PYDIA_RENDERER (renderer);

  func = PyObject_GetAttrString (self, "set_font");
  if (func && PyCallable_Check(func)) {
    Py_INCREF(self);
    Py_INCREF(func);
    arg = Py_BuildValue ("(Od)", PyDiaFont_New (font), height);
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_INCREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
draw_line(MyRenderer *renderer, 
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
      ON_RES(res);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_INCREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
draw_polyline(MyRenderer *renderer, 
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
      ON_RES(res);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_INCREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
draw_polygon(MyRenderer *renderer, 
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
      ON_RES(res);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_INCREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
fill_polygon(MyRenderer *renderer, 
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
      ON_RES(res);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_INCREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
draw_rect(MyRenderer *renderer, 
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
      ON_RES(res);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_INCREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
fill_rect(MyRenderer *renderer, 
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
      ON_RES(res);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_INCREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
draw_arc(MyRenderer *renderer, 
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
      ON_RES(res);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_INCREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
fill_arc(MyRenderer *renderer, 
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
      ON_RES(res);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_INCREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
draw_ellipse(MyRenderer *renderer, 
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
      ON_RES(res);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_INCREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
fill_ellipse(MyRenderer *renderer, 
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
      ON_RES(res);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_INCREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
draw_bezier(MyRenderer *renderer, 
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
      ON_RES(res);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_INCREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
fill_bezier(MyRenderer *renderer, 
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
      ON_RES(res);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_INCREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
draw_string(MyRenderer *renderer,
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
      ON_RES(res);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_INCREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

static void
draw_image(MyRenderer *renderer,
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
      ON_RES(res);
    }
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_INCREF(self);
  }
  else /* member optional */
    PyErr_Clear();
}

void
PyDia_export_data(DiagramData *data, const gchar *filename, 
                  const gchar *diafilename, void* user_data)
{
  MyRenderer *renderer;
  Rectangle *extent;
  gint len;

  {
    FILE *file;
    file = fopen(filename, "w"); /* "wb" for binary! */

    if (file == NULL) {
      message_error(_("Couldn't open: '%s' for writing.\n"), filename);
      return;
    }
    else
      fclose (file);
  }

  renderer = g_new(MyRenderer, 1);
  renderer->renderer.ops = &MyRenderOps;
  renderer->renderer.is_interactive = 0;
  renderer->renderer.interactive_ops = NULL;

  renderer->filename = g_strdup (filename);
  renderer->diagram_data = PyDiaDiagramData_New(data);

  /* The Python Renderer object was created at PyDia_Register */
  renderer->self = (PyObject*)user_data;

  /* this will call the required callback functions above */
  data_render(data, (Renderer *)renderer, NULL, NULL, NULL);

  g_free(renderer);
}
