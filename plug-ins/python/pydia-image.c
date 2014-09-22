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
#include "pydia-image.h"
#include "prop_pixbuf.h" /* pixbuf_encode_base64 */

#include <structmember.h> /* PyMemberDef */

/*
 * New
 */
PyObject* PyDiaImage_New (DiaImage *image)
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
  dia_image_unref (self->image);
  PyObject_DEL(self);
}

/*
 * Compare
 */
static int
PyDiaImage_Compare(PyDiaImage *self,
                   PyDiaImage *other)
{
  return memcmp(&(self->image), &(other->image), sizeof(DiaImage *));
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
    return Py_BuildValue("[ssssss]", "width", "height", 
                                    "rgb_data", "mask_data",
                                    "filename", "uri");
  else if (!strcmp(attr, "width"))
    return PyInt_FromLong(dia_image_width(self->image));
  else if (!strcmp(attr, "height"))
    return PyInt_FromLong(dia_image_height(self->image));
  else if (!strcmp(attr, "filename")) {
    return PyString_FromString(dia_image_filename(self->image));
  }
  else if (!strcmp(attr, "uri")) {
    GError* error = NULL;
    const gchar *fname = dia_image_filename(self->image);
    char* s;
    if (g_path_is_absolute(fname)) {
      s = g_filename_to_uri(fname, NULL, &error);
    } else {
      gchar *prefix = g_strdup_printf ("data:%s;base64,", dia_image_get_mime_type (self->image));

      s = pixbuf_encode_base64 (dia_image_pixbuf (self->image), prefix);
      g_free (prefix);
    }
    if (s) {
      PyObject* py_s = PyString_FromString(s);
      g_free(s);
      return py_s;
    } else {
      if (error) {
	PyErr_SetString(PyExc_RuntimeError, error->message);
	g_error_free (error);
      } else {
	PyErr_SetString(PyExc_RuntimeError, "Pixbuf conversion failed?");
      }
      return NULL;
    }
  }
  else if (!strcmp(attr, "rgb_data")) {
    unsigned char* s = dia_image_rgb_data(self->image);
    int len = dia_image_width(self->image) * dia_image_height(self->image) * 3;
    PyObject* py_s;

    if (!s)
      return PyErr_NoMemory();
    py_s = PyString_FromStringAndSize((const char *)s, len);
    g_free (s);
    return py_s;
  }
  else if (!strcmp(attr, "mask_data")) {
    unsigned char* s = dia_image_mask_data(self->image);
    int len = dia_image_width(self->image) * dia_image_height(self->image);
    PyObject* py_s;

    if (!s)
      return PyErr_NoMemory();
    py_s = PyString_FromStringAndSize((const char *)s, len);
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
  const gchar* name = dia_image_filename(self->image);
  gchar* s = g_strdup_printf("%ix%i,%s",
                             dia_image_width(self->image),
                             dia_image_height(self->image),
                             name ? name : "(null)");
  py_s = PyString_FromString(s);
  g_free (s);
  return py_s;
}

#define T_INVALID -1 /* can't allow direct access due to pyobject->cpoint indirection */
static PyMemberDef PyDiaImage_Members[] = {
    { "width", T_INVALID, 0, RESTRICTED|READONLY,
      "int: pixel width of the image" },
    { "height", T_INVALID, 0, RESTRICTED|READONLY,
      "int: pixel height of the image" },
    { "rgb_data", T_INVALID, 0, RESTRICTED|READONLY,
      "string: array of packed rgb pixel values" },
    { "mask_data", T_INVALID, 0, RESTRICTED|READONLY,
      "string: array of alpha pixel values" },
    { "filename", T_INVALID, 0, RESTRICTED|READONLY,
      "string: utf-8 encoded filename of the image" },
    { "uri", T_INVALID, 0, RESTRICTED|READONLY,
      "string: Uniform Resource Identifier of the image" },
    { NULL }
};
/*
 * Python objetcs
 */
PyTypeObject PyDiaImage_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "dia.Image",
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
    (getattrofunc)0,
    (setattrofunc)0,
    (PyBufferProcs *)0,
    0L, /* Flags */
    "dia.Image gets passed into DiaRenderer.draw_image",
    (traverseproc)0,
    (inquiry)0,
    (richcmpfunc)0,
    0, /* tp_weakliszoffset */
    (getiterfunc)0,
    (iternextfunc)0,
    0, /* tp_methods */
    PyDiaImage_Members, /* tp_members */
    0
};
