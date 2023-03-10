/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Python plug-in for dia
 * Copyright (C) 1999  James Henstridge
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

#include "config.h"

#include <glib/gi18n-lib.h>

#include "pydia-display.h"
#include "pydia-diagram.h"
#include "pydia-object.h" /* for PyObject_HEAD_INIT */

#include <structmember.h> /* PyMemberDef */

PyObject *
PyDiaDisplay_New(DDisplay *disp)
{
    PyDiaDisplay *self;

    self = PyObject_NEW(PyDiaDisplay, &PyDiaDisplay_Type);

    if (!self) return NULL;
    self->disp = disp;
    return (PyObject *)self;
}


static void
PyDiaDisplay_Dealloc (PyObject *self)
{
  PyObject_DEL (self);
}


static PyObject *
PyDiaDisplay_RichCompare (PyObject *self, PyObject *other, int op)
{
  Py_RETURN_RICHCOMPARE (self, other, op);
}


static Py_hash_t
PyDiaDisplay_Hash (PyObject *self)
{
  return (long) ((PyDiaDisplay *) self)->disp;
}


static PyObject *
PyDiaDisplay_Str (PyObject *self)
{
  return PyUnicode_FromString (((PyDiaDisplay *) self)->disp->diagram->filename);
}


/* methods here */

static PyObject *
PyDiaDisplay_AddUpdateAll(PyDiaDisplay *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":Display.add_update_all"))
	return NULL;
    ddisplay_add_update_all(self->disp);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDisplay_Flush(PyDiaDisplay *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":Display.flush"))
	return NULL;
    ddisplay_flush(self->disp);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDisplay_SetOrigion(PyDiaDisplay *self, PyObject *args)
{
    double x, y;

    if (!PyArg_ParseTuple(args, "dd:Display.set_origion", &x, &y))
	return NULL;
    ddisplay_set_origo(self->disp, x, y);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDisplay_Zoom(PyDiaDisplay *self, PyObject *args)
{
    Point p;
    double zoom;

    if (!PyArg_ParseTuple(args, "(dd)d:Display.zoom", &p.x, &p.y, &zoom))
	return NULL;
    ddisplay_zoom(self->disp, &p, zoom);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDisplay_ResizeCanvas(PyDiaDisplay *self, PyObject *args)
{
    int width, height;

    if (!PyArg_ParseTuple(args, "ii:Display.resize_canvas", &width,&height))
	return NULL;
    ddisplay_resize_canvas(self->disp, width, height);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDisplay_Close(PyDiaDisplay *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":Display.close"))
	return NULL;
    ddisplay_close(self->disp);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDisplay_SetTitle(PyDiaDisplay *self, PyObject *args)
{
    gchar *title;

    if (!PyArg_ParseTuple(args, "s:Display.set_title", &title))
	return NULL;
    ddisplay_set_title(self->disp, title);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDisplay_Scroll(PyDiaDisplay *self, PyObject *args)
{
    Point delta;

    if (!PyArg_ParseTuple(args, "dd:Display.scroll", &delta.x, &delta.y))
	return NULL;
    ddisplay_scroll(self->disp, &delta);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDisplay_ScrollUp(PyDiaDisplay *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":Display.scroll_up"))
	return NULL;
    ddisplay_scroll_up(self->disp);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDisplay_ScrollDown(PyDiaDisplay *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":Display.scroll_down"))
	return NULL;
    ddisplay_scroll_down(self->disp);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDisplay_ScrollLeft(PyDiaDisplay *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":Display.scroll_left"))
	return NULL;
    ddisplay_scroll_left(self->disp);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDisplay_ScrollRight(PyDiaDisplay *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":Display.scroll_right"))
	return NULL;
    ddisplay_scroll_right(self->disp);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef PyDiaDisplay_Methods[] = {
    {"add_update_all", (PyCFunction)PyDiaDisplay_AddUpdateAll, METH_VARARGS,
     "add_update_all() -> None.  Add the diagram visible area to the update queue."},
    {"flush", (PyCFunction)PyDiaDisplay_Flush, METH_VARARGS,
     "flush() -> None.  If no display update is queued, queue update."},
    {"set_origion", (PyCFunction)PyDiaDisplay_SetOrigion, METH_VARARGS,
     "set_origion(real: x, real, y) -> None."
     "  Move the given diagram point to the top-left corner of the visible area."},
    {"zoom", (PyCFunction)PyDiaDisplay_Zoom, METH_VARARGS,
     "zoom(real[2]: point, real: factor) -> None.  Zoom around the given point."},
    {"resize_canvas", (PyCFunction)PyDiaDisplay_ResizeCanvas, METH_VARARGS,
     "resize_canvas(int: width, int: height) -> None."
     "  BEWARE: Changing the drawing area but not the window size."},
    {"close", (PyCFunction)PyDiaDisplay_Close, METH_VARARGS,
     "close() -> None.  Close the display possibly asking to save."},
    {"set_title", (PyCFunction)PyDiaDisplay_SetTitle, METH_VARARGS,
     "set_title(string: title) -> None.  Temporary change of the diagram title."},
    {"scroll", (PyCFunction)PyDiaDisplay_Scroll, METH_VARARGS,
     "scroll(real: dx, real: dy) -> None.  Scroll the visible area by the given delta."},
    {"scroll_up", (PyCFunction)PyDiaDisplay_ScrollUp, METH_VARARGS,
     "scroll_up() -> None. Scroll upwards by a fixed amount."},
    {"scroll_down", (PyCFunction)PyDiaDisplay_ScrollDown, METH_VARARGS,
     "scroll_down() -> None. Scroll downwards by a fixed amount."},
    {"scroll_left", (PyCFunction)PyDiaDisplay_ScrollLeft, METH_VARARGS,
     "scroll_left() -> None. Scroll to the left by a fixed amount."},
    {"scroll_right", (PyCFunction)PyDiaDisplay_ScrollRight, METH_VARARGS,
     "scroll_right() -> None. Scroll to the right by a fixed amount."},
    {NULL, 0, 0, NULL}
};

#define T_INVALID -1 /* can't allow direct access due to pyobject->data indirection */
static PyMemberDef PyDiaDisplay_Members[] = {
    { "diagram", T_INVALID, 0, RESTRICTED|READONLY, /* can't allow direct access due to pyobject->disp indirection */
      "Diagram displayed." },
    { "origin", T_INVALID, 0, RESTRICTED|READONLY,
      "Point in diagram coordinates of the top-left corner of the visible area."},
    { "zoom_factor", T_INVALID, 0, RESTRICTED|READONLY,
      "Real: zoom-factor"},
    { "visible", T_INVALID, 0, RESTRICTED|READONLY,
      "Tuple(real: top, real: left, real: bottom, real: right) : visible area"},
    { NULL }
};


static PyObject *
PyDiaDisplay_GetAttr (PyObject *obj, PyObject *arg)
{
  PyDiaDisplay *self;
  const char *attr;

  if (PyUnicode_Check (arg)) {
    attr = PyUnicode_AsUTF8 (arg);
  } else {
    goto generic;
  }

  self = (PyDiaDisplay *) obj;

  if (!g_strcmp0 (attr, "__members__")) {
    return Py_BuildValue ("[ssss]",
                          "diagram", "origin", "visible", "zoom_factor");
  } else if (!g_strcmp0 (attr, "diagram")) {
    return PyDiaDiagram_New (self->disp->diagram);
    /* FIXME: shouldn't it have only one name */
  } else if (!g_strcmp0 (attr, "origo") ||
             !g_strcmp0 (attr, "origion") ||
             !g_strcmp0 (attr, "origin")) {
    return Py_BuildValue ("(dd)",
                          self->disp->origo.x,
                          self->disp->origo.y);
  } else if (!g_strcmp0 (attr, "zoom_factor")) {
    return PyFloat_FromDouble (self->disp->zoom_factor);
  } else if (!g_strcmp0 (attr, "visible")) {
    return Py_BuildValue ("(dddd)",
                          self->disp->visible.top,
                          self->disp->visible.left,
                          self->disp->visible.bottom,
                          self->disp->visible.right);
  }

generic:
  return PyObject_GenericGetAttr (obj, arg);
}


PyTypeObject PyDiaDisplay_Type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  .tp_name = "dia.Display",
  .tp_basicsize = sizeof (PyDiaDisplay),
  .tp_dealloc = PyDiaDisplay_Dealloc,
  .tp_getattro = PyDiaDisplay_GetAttr,
  .tp_richcompare = PyDiaDisplay_RichCompare,
  .tp_hash = PyDiaDisplay_Hash,
  .tp_str = PyDiaDisplay_Str,
  .tp_doc = "A Diagram can have multiple displays but every Display has just "
            "one Diagram.",
  .tp_methods = PyDiaDisplay_Methods,
  .tp_members = PyDiaDisplay_Members,
};
