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
#include "pydia-image.h"

/*
 * New
 */
PyObject* PyDiaImage_New (DiaImage image)
{
  PyDiaImage *self;
  
  self = PyObject_NEW(PyDiaImage, &PyDiaImage_Type);
  if (!self) return NULL;
  
  dia_image_add_ref (image);
  self->image = image;

  return (PyObject *)self;
}

/*
 * Dealloc
 */
static void
PyDiaImage_Dealloc(PyDiaImage *self)
{
  dia_image_release (self->image);
  PyMem_DEL(self);
}

/*
 * Compare
 */
static int
PyDiaImage_Compare(PyDiaImage *self,
                   PyDiaImage *other)
{
  return memcmp(&(self->image), &(other->image), sizeof(DiaImage));
}

/*
 * Hash
 */
static long
PyDiaImage_Hash(PyObject *self)
{
  return (long)self;
}

/*
 * GetAttr
 */
static PyObject *
PyDiaImage_GetAttr(PyDiaImage *self, gchar *attr)
{
  if (!strcmp(attr, "__members__"))
    return Py_BuildValue("[sssss]", "width", "height", 
                                    "rgb_data", "mask_data",
                                    "filename");
  else if (!strcmp(attr, "width"))
    return PyInt_FromLong(dia_image_width(self->image));
  else if (!strcmp(attr, "height"))
    return PyInt_FromLong(dia_image_height(self->image));
  else if (!strcmp(attr, "filename")) {
    char* s = dia_image_filename(self->image);
    PyObject* py_s = PyString_FromString(s);
    g_free (s);
    return py_s;
  }
  else if (!strcmp(attr, "rgb_data")) {
    char* s = dia_image_rgb_data(self->image);
    int len = dia_image_width(self->image) * dia_image_height(self->image) * 3;
    PyObject* py_s = PyString_FromStringAndSize(s, len);
    g_free (s);
    return py_s;
  }
  else if (!strcmp(attr, "mask_data")) {
    char* s = dia_image_rgb_data(self->image);
    int len = dia_image_width(self->image) * dia_image_height(self->image);
    PyObject* py_s = PyString_FromStringAndSize(s, len);
    g_free (s);
    return py_s;
  }

  PyErr_SetString(PyExc_AttributeError, attr);
  return NULL;
}

/*
 * Repr / _Str
 */
static PyObject *
PyDiaImage_Str(PyDiaImage *self)
{
  PyObject* py_s;
  gchar* name = dia_image_filename(self->image);
  gchar* s = g_strdup_printf("%ix%i,%s",
                             dia_image_width(self->image),
                             dia_image_height(self->image),
                             name);
  py_s = PyString_FromString(s);
  g_free (s);
  g_free (name);
  return py_s;
}

/*
 * Python objetcs
 */
PyTypeObject PyDiaImage_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "DiaImage",
    sizeof(PyDiaImage),
    0,
    (destructor)PyDiaImage_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaImage_GetAttr,
    (setattrfunc)0,
    (cmpfunc)PyDiaImage_Compare,
    (reprfunc)0,
    0,
    0,
    0,
    (hashfunc)PyDiaImage_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaImage_Str,
    0L,0L,0L,0L,
    NULL
};
