/* Python plug-in for dia
 * Copyright (C) 1999  James Henstridge
 * Copyright (C) 2000  Hans Breuer
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

#include "pydia-object.h"
#include "pydia-error.h"
#include "message.h"

/*
 * A little helper to dump pythons last error info either to file only or 
 * additional popup a message_error
 */
void 
_pyerror_report_last (gboolean popup, const char* fn, const char* file, int line)
{
  PyObject *exc, *v, *tb, *ef;
  char* sLoc;

  if (strlen(fn) > 0)
    sLoc = g_strdup_printf ("PyDia Error (%s):\n", fn); /* GNU_PRETTY_FUNCTION is enough */
  else
    sLoc = g_strdup_printf ("PyDia Error (%s:%d):\n", file, line);

  PyErr_Fetch (&exc, &v, &tb);
  PyErr_NormalizeException(&exc, &v, &tb);

  ef = PyDiaError_New(sLoc, popup ? FALSE : TRUE);
  PyFile_WriteObject (exc, ef, 0);
  PyFile_WriteObject (v, ef, 0);
  PyTraceBack_Print(tb, ef);
  if (((PyDiaError*)ef)->str && popup) 
    message_error ("%s", ((PyDiaError*)ef)->str->str);
  g_free (sLoc);
  Py_DECREF (ef);
  Py_XDECREF(exc);
  Py_XDECREF(v);
  Py_XDECREF(tb);
}

/*
 * New
 */
PyObject* PyDiaError_New (const char* s, gboolean unbuffered)
{
  PyDiaError *self;
  
  self = PyObject_NEW(PyDiaError, &PyDiaError_Type);
  if (!self) return NULL;
  if (unbuffered) {
    self->str = NULL;
  }
  else {
    if (s)
      self->str = g_string_new (s);
    else
      self->str = g_string_new ("");
  }

  return (PyObject *)self;
}

/*
 * Dealloc
 */
static void
PyDiaError_Dealloc(PyDiaError *self)
{
  if (self->str)
    g_string_free (self->str, TRUE);
  PyObject_DEL(self);
}

/*
 * Compare
 */
static int
PyDiaError_Compare(PyDiaError *self,
                  PyDiaError *other)
{
  int len = 0;

  if (self->str == other->str) return 0;
  if (NULL == self->str) return -1;
  if (NULL == other->str) return -1;

  len = (self->str->len > other->str->len ? other->str->len : self->str->len);
  return memcmp(self->str->str, other->str->str, len);
}

/*
 * Hash
 */
static long
PyDiaError_Hash(PyObject *self)
{
  return (long)self;
}

static PyObject *
PyDiaError_Write(PyDiaError *self, PyObject *args)
{
  PyObject* obj;
  gchar* s;

  if (!PyArg_ParseTuple(args, "O", &obj))
    return NULL;

  s = PyString_AsString (obj);

  if (self->str)
    g_string_append (self->str, s);

  g_print ("%s", s);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyMethodDef PyDiaError_Methods [] = {
    { "write", (PyCFunction)PyDiaError_Write, METH_VARARGS },
    { NULL, 0, 0, NULL }
};

/*
 * GetAttr
 */
static PyObject *
PyDiaError_GetAttr(PyDiaError *self, gchar *attr)
{
  return Py_FindMethod(PyDiaError_Methods, (PyObject *)self, attr);
}

/*
 * Repr / _Str
 */
static PyObject *
PyDiaError_Str(PyDiaError *self)
{
  return PyString_FromString(self->str->str);
}

/*
 * Python objetcs
 */
PyTypeObject PyDiaError_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "DiaError",
    sizeof(PyDiaError),
    0,
    (destructor)PyDiaError_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaError_GetAttr,
    (setattrfunc)0,
    (cmpfunc)PyDiaError_Compare,
    (reprfunc)0,
    0,
    0,
    0,
    (hashfunc)PyDiaError_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaError_Str,
    (getattrofunc)0L,
    (setattrofunc)0L,
    (PyBufferProcs *)0L,
    0L, /* Flags */
    "The error object is just a helper to redirect errors to messages",
    (traverseproc)0,
    (inquiry)0,
    (richcmpfunc)0,
    0, /* tp_weakliszoffset */
    (getiterfunc)0,
    (iternextfunc)0,
    PyDiaError_Methods, /* tp_methods */
    NULL, /* tp_members */
    0
};
