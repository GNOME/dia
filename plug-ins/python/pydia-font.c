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

#include <glib.h>

#include "pydia-object.h"
#include "pydia-font.h"

/*
 * New
 */
PyObject* PyDiaFont_New (Font* font)
{
  PyDiaFont *self;
  
  self = PyObject_NEW(PyDiaFont, &PyDiaFont_Type);
  if (!self) return NULL;
  
  self->font.name = g_strdup(font->name);

  return (PyObject *)self;
}

/*
 * Dealloc
 */
static void
PyDiaFont_Dealloc(PyDiaFont *self)
{
  g_free (self->font.name);
  PyMem_DEL(self);
}

/*
 * Compare
 */
static int
PyDiaFont_Compare(PyDiaFont *self,
                  PyDiaFont *other)
{
  return strcmp(self->font.name, other->font.name);
}

/*
 * Hash
 */
static long
PyDiaFont_Hash(PyObject *self)
{
  return (long)self;
}

/*
 * GetAttr
 */
static PyObject *
PyDiaFont_GetAttr(PyDiaFont *self, gchar *attr)
{
  if (!strcmp(attr, "__members__"))
    return Py_BuildValue("[s]", "name");
  else if (!strcmp(attr, "name"))
    return PyString_FromString(self->font.name);

  PyErr_SetString(PyExc_AttributeError, attr);
  return NULL;
}

/*
 * Repr / _Str
 */
static PyObject *
PyDiaFont_Str(PyDiaFont *self)
{
  return PyString_FromString(self->font.name);
}

/*
 * Python objetc
 */

PyTypeObject PyDiaFont_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "DiaFont",
    sizeof(PyDiaFont),
    0,
    (destructor)PyDiaFont_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaFont_GetAttr,
    (setattrfunc)0,
    (cmpfunc)PyDiaFont_Compare,
    (reprfunc)0,
    0,
    0,
    0,
    (hashfunc)PyDiaFont_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaFont_Str,
    0L,0L,0L,0L,
    NULL
};
