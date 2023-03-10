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

#include "config.h"

#include <glib/gi18n-lib.h>

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
  g_clear_pointer (&sLoc, g_free);
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
PyDiaError_Dealloc (PyObject *self)
{
  if (((PyDiaError *) self)->str) {
    g_string_free (((PyDiaError *) self)->str, TRUE);
  }

  PyObject_DEL (self);
}


/*
 * Compare
 */
static PyObject *
PyDiaError_RichCompare (PyObject *a,
                        PyObject *b,
                        int       op)
{
  PyDiaError *self = (PyDiaError *) a;
  PyDiaError *other = (PyDiaError *) b;
  PyObject *left_str;
  PyObject *right_str;
  PyObject *result;

  if (self->str) {
    left_str = PyUnicode_FromStringAndSize (self->str->str, self->str->len);
  } else {
    left_str = Py_None;
    Py_INCREF (left_str);
  }

  if (other->str) {
    right_str = PyUnicode_FromStringAndSize (other->str->str, other->str->len);
  } else {
    right_str = Py_None;
    Py_INCREF (right_str);
  }

  result = PyUnicode_RichCompare (left_str, right_str, op);

  Py_DECREF (left_str);
  Py_DECREF (right_str);

  return result;
}


/*
 * Hash
 */
static Py_hash_t
PyDiaError_Hash (PyObject *self)
{
  return (long) self;
}


static PyObject *
PyDiaError_Write (PyDiaError *self, PyObject *args)
{
  PyObject *obj;
  const char *s;

  if (!PyArg_ParseTuple (args, "O", &obj)) {
    return NULL;
  }

  s = PyUnicode_AsUTF8 (obj);

  if (self->str) {
    g_string_append (self->str, s);
  }

  g_printerr ("%s", s);

  Py_RETURN_NONE;
}


static PyMethodDef PyDiaError_Methods [] = {
    { "write", (PyCFunction)PyDiaError_Write, METH_VARARGS },
    { NULL, 0, 0, NULL }
};


/*
 * Repr / _Str
 */
static PyObject *
PyDiaError_Str (PyObject *self)
{
  return PyUnicode_FromString (((PyDiaError *) self)->str->str);
}


/*
 * Python objetcs
 */
PyTypeObject PyDiaError_Type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  .tp_name = "DiaError",
  .tp_basicsize = sizeof (PyDiaError),
  .tp_dealloc = PyDiaError_Dealloc,
  .tp_getattro = PyObject_GenericGetAttr,
  .tp_richcompare = PyDiaError_RichCompare,
  .tp_hash = PyDiaError_Hash,
  .tp_str = PyDiaError_Str,
  .tp_doc = "The error object is just a helper to redirect errors to "
            "messages",
  .tp_methods = PyDiaError_Methods,
};
