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

#include <glib.h>

#include "pydia-display.h"
#include "pydia-diagram.h"
#include "pydia-object.h" /* for PyObject_HEAD_INIT */

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
PyDiaDisplay_Dealloc(PyDiaDisplay *self)
{
     PyMem_DEL(self);
}

static int
PyDiaDisplay_Compare(PyDiaDisplay *self, PyDiaDisplay *other)
{
    if (self->disp == other->disp) return 0;
    if (self->disp > other->disp) return -1;
    return 1;
}

static long
PyDiaDisplay_Hash(PyDiaDisplay *self)
{
    return (long)self->disp;
}

static PyObject *
PyDiaDisplay_Str(PyDiaDisplay *self)
{
    return PyString_FromString(self->disp->diagram->filename);
}

/* methods here */

static PyObject *
PyDiaDisplay_AddUpdateAll(PyDiaDisplay *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":DiaDisplay.add_update_all"))
	return NULL;
    ddisplay_add_update_all(self->disp);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDisplay_Flush(PyDiaDisplay *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":DiaDisplay.flush"))
	return NULL;
    ddisplay_flush(self->disp);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDisplay_SetOrigion(PyDiaDisplay *self, PyObject *args)
{
    double x, y;

    if (!PyArg_ParseTuple(args, "dd:DiaDisplay.set_origion", &x, &y))
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

    if (!PyArg_ParseTuple(args, "(dd)d:DiaDisplay.zoom", &p.x, &p.y, &zoom))
	return NULL;
    ddisplay_zoom(self->disp, &p, zoom);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDisplay_ResizeCanvas(PyDiaDisplay *self, PyObject *args)
{
    int width, height;

    if (!PyArg_ParseTuple(args, "ii:DiaDisplay.resize_canvas", &width,&height))
	return NULL;
    ddisplay_resize_canvas(self->disp, width, height);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDisplay_Close(PyDiaDisplay *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":DiaDisplay.close"))
	return NULL;
    ddisplay_close(self->disp);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDisplay_SetTitle(PyDiaDisplay *self, PyObject *args)
{
    gchar *title;

    if (!PyArg_ParseTuple(args, "s:DiaDisplay.set_title", &title))
	return NULL;
    ddisplay_set_title(self->disp, title);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDisplay_Scroll(PyDiaDisplay *self, PyObject *args)
{
    Point delta;

    if (!PyArg_ParseTuple(args, "dd:DiaDisplay.scroll", &delta.x, &delta.y))
	return NULL;
    ddisplay_scroll(self->disp, &delta);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDisplay_ScrollUp(PyDiaDisplay *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":DiaDisplay.scroll_up"))
	return NULL;
    ddisplay_scroll_up(self->disp);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDisplay_ScrollDown(PyDiaDisplay *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":DiaDisplay.scroll_down"))
	return NULL;
    ddisplay_scroll_down(self->disp);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDisplay_ScrollLeft(PyDiaDisplay *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":DiaDisplay.scroll_left"))
	return NULL;
    ddisplay_scroll_left(self->disp);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDisplay_ScrollRight(PyDiaDisplay *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":DiaDisplay.scroll_right"))
	return NULL;
    ddisplay_scroll_right(self->disp);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef PyDiaDisplay_Methods[] = {
    {"add_update_all", (PyCFunction)PyDiaDisplay_AddUpdateAll, 1},
    {"flush", (PyCFunction)PyDiaDisplay_Flush, 1},
    {"set_origion", (PyCFunction)PyDiaDisplay_SetOrigion, 1},
    {"zoom", (PyCFunction)PyDiaDisplay_Zoom, 1},
    {"resize_canvas", (PyCFunction)PyDiaDisplay_ResizeCanvas, 1},
    {"close", (PyCFunction)PyDiaDisplay_Close, 1},
    {"set_title", (PyCFunction)PyDiaDisplay_SetTitle, 1},
    {"scroll", (PyCFunction)PyDiaDisplay_Scroll, 1},
    {"scroll_up", (PyCFunction)PyDiaDisplay_ScrollUp, 1},
    {"scroll_down", (PyCFunction)PyDiaDisplay_ScrollDown, 1},
    {"scroll_left", (PyCFunction)PyDiaDisplay_ScrollLeft, 1},
    {"scroll_right", (PyCFunction)PyDiaDisplay_ScrollRight, 1},
    {NULL, 0, 0, NULL}
};

static PyObject *
PyDiaDisplay_GetAttr(PyDiaDisplay *self, gchar *attr)
{
    if (!strcmp(attr, "__members__"))
	return Py_BuildValue("[ssss]", "diagram", "origin", "visible",
			     "zoom_factor");
    else if (!strcmp(attr, "diagram"))
	return PyDiaDiagram_New(self->disp->diagram);
    else if (!strcmp(attr, "origo") || !strcmp(attr, "origion"))
	return Py_BuildValue("(dd)", self->disp->origo.x, self->disp->origo.y);
    else if (!strcmp(attr, "zoom_factor"))
	return PyFloat_FromDouble(self->disp->zoom_factor);
    else if (!strcmp(attr, "visible"))
	return Py_BuildValue("(dddd)", self->disp->visible.top,
			     self->disp->visible.left,
			     self->disp->visible.bottom,
			     self->disp->visible.right);

    return Py_FindMethod(PyDiaDisplay_Methods, (PyObject *)self, attr);
}

PyTypeObject PyDiaDisplay_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "DiaDisplay",
    sizeof(PyDiaDisplay),
    0,
    (destructor)PyDiaDisplay_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaDisplay_GetAttr,
    (setattrfunc)0,
    (cmpfunc)PyDiaDisplay_Compare,
    (reprfunc)0,
    0,
    0,
    0,
    (hashfunc)PyDiaDisplay_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaDisplay_Str,
    0L,0L,0L,0L,
    NULL
};
