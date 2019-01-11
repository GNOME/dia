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
#include "pydia-color.h"

#include <structmember.h> /* PyMemberDef */

/*
 * New
 */
PyObject* PyDiaColor_New (Color* color)
{
  PyDiaColor *self;
  
  self = PyObject_NEW(PyDiaColor, &PyDiaColor_Type);
  if (!self) return NULL;
  
  self->color = *color;

  return (PyObject *)self;
}

/*
 * Dealloc
 */
static void
PyDiaColor_Dealloc(PyObject *self)
{
  PyObject_DEL(self);
}

/*
 * Compare
 */
static int
PyDiaColor_Compare(PyDiaColor *self,
                  PyDiaColor *other)
{
  return memcmp(&(self->color), &(other->color), sizeof(Color));
}

/*
 * Hash
 */
static long
PyDiaColor_Hash(PyObject *self)
{
  return (long)self;
}

/*
 * Repr / _Str
 */
static PyObject *
PyDiaColor_Str(PyDiaColor *self)
{
  PyObject* py_s;
  gchar* s = g_strdup_printf("(%f,%f,%f,%f)",
                             (float)(self->color.red),
                             (float)(self->color.green),
                             (float)(self->color.blue),
                             (float)(self->color.alpha));
  py_s = PyString_FromString(s);
  g_free (s);
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
 * Python objetcs
 */
PyTypeObject PyDiaColor_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "DiaColor",
    sizeof(PyDiaColor),
    0,
    (destructor)PyDiaColor_Dealloc,
    (printfunc)0,
    (getattrfunc)0,
    (setattrfunc)0,
    (cmpfunc)PyDiaColor_Compare,
    (reprfunc)0,
    0,
    0,
    0,
    (hashfunc)PyDiaColor_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaColor_Str,
    PyObject_GenericGetAttr, /* tp_getattro */
    (setattrofunc)0,
    (PyBufferProcs *)0,
    0L, /* Flags */
    "A color either defined by a color string or by a tuple with three elements "
    "(r, g, b) with type float 0.0 ... 1.0 or range int 0 ... 65535",
    (traverseproc)0,
    (inquiry)0,
    (richcmpfunc)0,
    0, /* tp_weakliszoffset */
    (getiterfunc)0,
    (iternextfunc)0,
    0, /* tp_methods */
    PyDiaColor_Members, /* tp_members */
    0
};
