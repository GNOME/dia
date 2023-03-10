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

#include "pydia-handle.h"
#include "pydia-cpoint.h"
#include "pydia-geometry.h"
#include "pydia-object.h" /* for PyObject_HEAD_INIT */

#include <structmember.h> /* PyMemberDef */

PyObject *
PyDiaHandle_New(Handle *handle, DiaObject *owner)
{
    PyDiaHandle *self;

    self = PyObject_NEW(PyDiaHandle, &PyDiaHandle_Type);

    if (!self) return NULL;
    self->handle = handle;
    self->owner = owner;
    return (PyObject *)self;
}


static void
PyDiaHandle_Dealloc (PyObject *self)
{
  PyObject_DEL (self);
}


static PyObject *
PyDiaHandle_RichCompare (PyObject *self, PyObject *other, int op)
{
  Py_RETURN_RICHCOMPARE (((PyDiaHandle *) self)->handle,
                         ((PyDiaHandle *) other)->handle,
                         op);
}


static Py_hash_t
PyDiaHandle_Hash (PyObject *self)
{
  return (long) ((PyDiaHandle *) self)->handle;
}


static PyObject *
PyDiaHandle_Connect (PyDiaHandle *self, PyObject *args)
{
  PyObject *obj;

  if (!PyArg_ParseTuple (args, "O:Handle.connect", &obj)) {
    return NULL;
  }

  if (PyDiaConnectionPoint_Check (obj)) {
    PyDiaConnectionPoint *o = (PyDiaConnectionPoint *)obj;

    object_connect (self->owner, self->handle, o->cpoint);
  } else if (obj == Py_None) {
    object_unconnect (self->handle->connected_to->object, self->handle);
  } else {
    PyErr_SetString (PyExc_TypeError,
                     "Expecting a ConnectionPoint or None to disconnect.");
    return NULL;
  }
  Py_INCREF(Py_None);
  return Py_None;
}

static PyMethodDef PyDiaHandle_Methods[] = {
    { "connect", (PyCFunction)PyDiaHandle_Connect, METH_VARARGS,
      "connect(ConnectionPoint: cp) -> None."
      "  Connect object A's handle with object B's connection point. To disconnect a handle pass in None." },
    { NULL, 0, 0, NULL }
};

#define T_INVALID -1 /* can't allow direct access due to pyobject->handle indirection */
static PyMemberDef PyDiaHandle_Members[] = {
    { "connect_type", T_INVALID, 0, RESTRICTED|READONLY,
      "NONCONNECTABLE=0." },
    { "connected_to", T_INVALID, 0, RESTRICTED|READONLY,
      "The connected ConnectionPoint object or None." },
    { "id", T_INVALID, 0, RESTRICTED|READONLY,
      "Can be used to derive preferred directions from it." },
    { "pos", T_INVALID, 0, RESTRICTED|READONLY,
      "The position of the connection point." },
    { "type", T_INVALID, 0, RESTRICTED|READONLY,
      "NON_MOVABLE=0, MAJOR_CONTROL=1, MINOR_CONTROL=2" },
    { NULL }
};


static PyObject *
PyDiaHandle_GetAttr (PyObject *obj, PyObject *arg)
{
  PyDiaHandle *self;
  const char *attr;

  if (PyUnicode_Check (arg)) {
    attr = PyUnicode_AsUTF8 (arg);
  } else {
    goto generic;
  }

  self = (PyDiaHandle *) obj;

  if (!g_strcmp0 (attr, "__members__")) {
    return Py_BuildValue ("[sssss]",
                          "connect_type", "connected_to", "id", "pos", "type");
  } else if (!g_strcmp0 (attr, "id")) {
    return PyLong_FromLong (self->handle->id);
  } else if (!g_strcmp0 (attr, "type")) {
    return PyLong_FromLong (self->handle->type);
  } else if (!g_strcmp0 (attr, "pos")) {
    return PyDiaPoint_New (&(self->handle->pos));
  } else if (!g_strcmp0 (attr, "connect_type")) {
    return PyLong_FromLong (self->handle->connect_type);
  } else if (!g_strcmp0 (attr, "connected_to")) {
    if (self->handle->connected_to) {
      return PyDiaConnectionPoint_New (self->handle->connected_to);
    }
    Py_INCREF (Py_None);
    return Py_None;
  }

generic:
  return PyObject_GenericGetAttr (obj, arg);
}


PyTypeObject PyDiaHandle_Type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  .tp_name = "dia.Handle",
  .tp_basicsize = sizeof (PyDiaHandle),
  .tp_dealloc = PyDiaHandle_Dealloc,
  .tp_getattro = PyDiaHandle_GetAttr,
  .tp_richcompare = PyDiaHandle_RichCompare,
  .tp_hash = PyDiaHandle_Hash,
  .tp_doc = "A handle is used to connect objects or for object resizing.",
  .tp_methods = PyDiaHandle_Methods,
  .tp_members = PyDiaHandle_Members,
};
