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
#include "pydia-color.h"

#include <structmember.h> /* PyMemberDef */


/*
 * New
 */
PyObject *
PyDiaColor_New (Color* color)
{
  PyDiaColor *self;

  self = PyObject_NEW (PyDiaColor, &PyDiaColor_Type);

  if (!self) return NULL;

  self->color = *color;

  return (PyObject *) self;
}


/*
 * Dealloc
 */
static void
PyDiaColor_Dealloc (PyObject *self)
{
  PyObject_DEL (self);
}


/*
 * Compare
 */
static PyObject *
PyDiaColor_RichCompare (PyObject *self, PyObject *other, int op)
{
  int res = memcmp (&(((PyDiaColor *) self)->color),
                    &(((PyDiaColor *) other)->color),
                    sizeof (Color));

  switch (op) {
    case Py_LT:
      if (res < 0) {
        Py_RETURN_TRUE;
      }
      break;
    case Py_LE:
      if (res <= 0) {
        Py_RETURN_TRUE;
      }
      break;
    case Py_NE:
      if (res != 0) {
        Py_RETURN_TRUE;
      }
      break;
    case Py_GT:
      if (res > 0) {
        Py_RETURN_TRUE;
      }
      break;
    case Py_GE:
      if (res >= 0) {
        Py_RETURN_TRUE;
      }
      break;
    case Py_EQ:
      if (res == 0) {
        Py_RETURN_TRUE;
      }
      break;
    default:
      Py_RETURN_NOTIMPLEMENTED;
  }

  Py_RETURN_FALSE;
}


/*
 * Hash
 */
static Py_hash_t
PyDiaColor_Hash (PyObject *self)
{
  return (long) self;
}


/*
 * Repr / _Str
 */
static PyObject *
PyDiaColor_Str (PyObject *self)
{
  PyObject *py_s;
  char *s = g_strdup_printf ("(%f,%f,%f,%f)",
                             (float) (((PyDiaColor *) self)->color.red),
                             (float) (((PyDiaColor *) self)->color.green),
                             (float) (((PyDiaColor *) self)->color.blue),
                             (float) (((PyDiaColor *) self)->color.alpha));

  py_s = PyUnicode_FromString (s);
  g_clear_pointer (&s, g_free);

  return py_s;
}


static PyMemberDef PyDiaColor_Members[] = {
    { "red", T_FLOAT, offsetof(PyDiaColor, color.red), 0,
      "double: red color component [0 .. 1.0]" },
    { "green", T_FLOAT, offsetof(PyDiaColor, color.green), 0,
      "double: green color component [0 .. 1.0]" },
    { "blue", T_FLOAT, offsetof(PyDiaColor, color.blue), 0,
      "double: blue color component [0 .. 1.0]" },
    { "alpha", T_FLOAT, offsetof(PyDiaColor, color.alpha), 0,
      "double: alpha color component [0 .. 1.0]" },
    { NULL }
};


/*
 * Python objects
 */
PyTypeObject PyDiaColor_Type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  .tp_name = "DiaColor",
  .tp_basicsize = sizeof (PyDiaColor),
  .tp_dealloc = PyDiaColor_Dealloc,
  .tp_richcompare = PyDiaColor_RichCompare,
  .tp_hash = PyDiaColor_Hash,
  .tp_str = PyDiaColor_Str,
  .tp_getattro = PyObject_GenericGetAttr,
  .tp_doc = "A color either defined by a color string or by a tuple with "
            "three elements (r, g, b) with type float 0.0 ... 1.0 or range "
            "int 0 ... 65535",
  .tp_members = PyDiaColor_Members,
};
