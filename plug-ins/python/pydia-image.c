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
#include "pydia-image.h"
#include "prop_pixbuf.h" /* pixbuf_encode_base64 */

#include <structmember.h> /* PyMemberDef */


/*
 * New
 */
PyObject *
PyDiaImage_New (DiaImage *image)
{
  PyDiaImage *self;

  self = PyObject_NEW (PyDiaImage, &PyDiaImage_Type);

  if (!self) return NULL;

  self->image = g_object_ref (image);

  return (PyObject *) self;
}


/*
 * Dealloc
 */
static void
PyDiaImage_Dealloc (PyObject *self)
{
  g_clear_object (&((PyDiaImage *) self)->image);

  PyObject_DEL (self);
}


/*
 * Compare
 */
static PyObject *
PyDiaImage_RichCompare (PyObject *a,
                        PyObject *b,
                        int       op)
{
  PyDiaImage *self = (PyDiaImage *) a;
  PyDiaImage *other = (PyDiaImage *) b;

  Py_RETURN_RICHCOMPARE (self->image, other->image, op);
}


/*
 * Hash
 */
static Py_hash_t
PyDiaImage_Hash (PyObject *self)
{
  return (long) self;
}


/*
 * GetAttr
 */
static PyObject *
PyDiaImage_GetAttr (PyObject *obj, PyObject *arg)
{
  PyDiaImage *self;
  const char *attr;

  if (PyUnicode_Check (arg)) {
    attr = PyUnicode_AsUTF8 (arg);
  } else {
    goto generic;
  }

  self = (PyDiaImage *) obj;

  if (!g_strcmp0 (attr, "__members__")) {
    return Py_BuildValue ("[ssssss]",
                          "width", "height", "rgb_data", "mask_data",
                          "filename", "uri");
  } else if (!g_strcmp0 (attr, "width"))
    return PyLong_FromLong (dia_image_width(self->image));
  else if (!g_strcmp0 (attr, "height"))
    return PyLong_FromLong (dia_image_height(self->image));
  else if (!g_strcmp0 (attr, "filename")) {
    return PyUnicode_FromString (dia_image_filename (self->image));
  } else if (!g_strcmp0 (attr, "uri")) {
    GError *error = NULL;
    const char *fname = dia_image_filename (self->image);
    char *s;

    if (g_path_is_absolute (fname)) {
      s = g_filename_to_uri (fname, NULL, &error);
    } else {
      char *prefix = g_strdup_printf ("data:%s;base64,",
                                      dia_image_get_mime_type (self->image));

      s = pixbuf_encode_base64 (dia_image_pixbuf (self->image), prefix);

      g_clear_pointer (&prefix, g_free);
    }

    if (s) {
      PyObject *py_s = PyUnicode_FromString (s);
      g_clear_pointer (&s, g_free);
      return py_s;
    } else {
      if (error) {
        PyErr_SetString (PyExc_RuntimeError, error->message);
        g_clear_error (&error);
      } else {
        PyErr_SetString (PyExc_RuntimeError, "Pixbuf conversion failed?");
      }
      return NULL;
    }
  } else if (!g_strcmp0 (attr, "rgb_data")) {
    unsigned char *s = dia_image_rgb_data (self->image);
    int len = dia_image_width (self->image) * dia_image_height (self->image) * 3;
    PyObject *py_s;

    if (!s) {
      return PyErr_NoMemory ();
    }

    py_s = PyBytes_FromStringAndSize ((const char *) s, len);
    g_clear_pointer (&s, g_free);
    return py_s;
  } else if (!g_strcmp0 (attr, "mask_data")) {
    unsigned char *s = dia_image_mask_data (self->image);
    int len = dia_image_width (self->image) * dia_image_height (self->image);
    PyObject *py_s;

    if (!s) {
      return PyErr_NoMemory ();
    }
    py_s = PyBytes_FromStringAndSize ((const char *) s, len);
    g_clear_pointer (&s, g_free);
    return py_s;
  }

generic:
  return PyObject_GenericGetAttr (obj, arg);
}


/*
 * Repr / _Str
 */
static PyObject *
PyDiaImage_Str (PyObject *self)
{
  PyObject *py_s;
  const char *name = dia_image_filename (((PyDiaImage *) self)->image);
  char *s = g_strdup_printf ("%ix%i,%s",
                             dia_image_width (((PyDiaImage *) self)->image),
                             dia_image_height (((PyDiaImage *) self)->image),
                             name ? name : "(null)");

  py_s = PyUnicode_FromString (s);
  g_clear_pointer (&s, g_free);

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
  PyVarObject_HEAD_INIT (NULL, 0)
  .tp_name = "dia.Image",
  .tp_basicsize = sizeof (PyDiaImage),
  .tp_dealloc = PyDiaImage_Dealloc,
  .tp_getattro = PyDiaImage_GetAttr,
  .tp_richcompare = PyDiaImage_RichCompare,
  .tp_hash = PyDiaImage_Hash,
  .tp_str = PyDiaImage_Str,
  .tp_doc = "dia.Image gets passed into DiaRenderer.draw_image",
  .tp_members = PyDiaImage_Members,
};
