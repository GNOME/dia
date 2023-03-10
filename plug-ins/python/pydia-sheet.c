/* Python plug-in for dia
 * Copyright (C) 1999  James Henstridge
 * Copyright (C) 2014  Hans Breuer
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
#include "pydia-sheet.h"

#include <structmember.h> /* PyMemberDef */

PyObject *
PyDiaSheet_New(Sheet *sheet)
{
    PyDiaSheet *self;

    self = PyObject_NEW(PyDiaSheet, &PyDiaSheet_Type);

    if (!self) return NULL;
    self->sheet = sheet;
    return (PyObject *)self;
}


static void
PyDiaSheet_Dealloc (PyObject *self)
{
  PyObject_DEL (self);
}


static PyObject *
PyDiaSheet_RichCompare (PyObject *self, PyObject *other, int op)
{
  Py_RETURN_RICHCOMPARE (((PyDiaSheet *) self)->sheet,
                         ((PyDiaSheet *) other)->sheet,
                         op);
}


static Py_hash_t
PyDiaSheet_Hash (PyObject *self)
{
  return (long) ((PyDiaSheet *) self)->sheet;
}


static PyObject *
PyDiaSheet_Str (PyObject *self)
{
  return PyUnicode_FromString (((PyDiaSheet *) self)->sheet->description);
}

/*
 * "real" member function implementaion ?
 */

static PyMethodDef PyDiaSheet_Methods[] = {
    {NULL, 0, 0, NULL}
};

#define T_INVALID -1 /* can't allow direct access due to pyobject->data indirection */
static PyMemberDef PyDiaSheet_Members[] = {
    { "name", T_INVALID, 0, RESTRICTED|READONLY,
      "The name for the sheet." },
    { "description", T_INVALID, 0, RESTRICTED|READONLY,
      "The description for the sheet." },
    { "filename", T_INVALID, 0, RESTRICTED|READONLY,
      "The filename for the sheet." },
    { "user", T_INVALID, 0, RESTRICTED|READONLY,
      "The sheet scope is user provided, not system." },
    { "objects", T_INVALID, 0, RESTRICTED|READONLY,
      "The list of sheet objects referenced by the sheet." },
    { NULL }
};

static PyObject *
PyDiaSheet_GetAttr (PyObject *obj, PyObject *arg)
{
  PyDiaSheet *self;
  const char *attr;

  if (PyUnicode_Check (arg)) {
    attr = PyUnicode_AsUTF8 (arg);
  } else {
    goto generic;
  }

  self = (PyDiaSheet *) obj;

  if (!g_strcmp0 (attr, "__members__")) {
    return Py_BuildValue ("[ssss]",
                          "name", "description", "filename", "objects");
  } else if (!g_strcmp0 (attr, "name")) {
    return PyUnicode_FromString (self->sheet->name);
  } else if (!g_strcmp0 (attr, "description")) {
    return PyUnicode_FromString (self->sheet->description);
  } else if (!g_strcmp0 (attr, "filename")) {
    return PyUnicode_FromString (self->sheet->filename);
  } else if (!g_strcmp0 (attr, "user")) {
    return PyLong_FromLong (self->sheet->scope == SHEET_SCOPE_USER ? 1 : 0);
  } else if (!g_strcmp0 (attr, "objects")) {
    /* Just returning tuples with information for now. Wrapping SheetObject
    * looks like overkill for the time being.
    *  - DiaObjectType or None
    *  - description of the SheetObject
    *  - filename of the icon file
    */
    PyObject *ret = PyList_New (0);
    GSList *list;

    for (list = self->sheet->objects; list != NULL; list = list->next) {
      SheetObject *so = list->data;
      DiaObjectType *ot = object_get_type (so->object_type);
      PyObject *py_ot;
      PyObject *val;

      if (ot) {
        py_ot = PyDiaObjectType_New (ot);
      } else {
        py_ot = Py_None;
        Py_INCREF (Py_None);
      }

      val = Py_BuildValue ("(Oss)", py_ot, so->description, so->pixmap_file);

      PyList_Append (ret, val);
    }

    return ret;
  }

generic:
  return PyObject_GenericGetAttr (obj, arg);
}


PyTypeObject PyDiaSheet_Type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  .tp_name = "dia.Sheet",
  .tp_basicsize = sizeof (PyDiaSheet),
  .tp_dealloc = PyDiaSheet_Dealloc,
  .tp_getattro = PyDiaSheet_GetAttr,
  .tp_richcompare = PyDiaSheet_RichCompare,
  .tp_hash = PyDiaSheet_Hash,
  .tp_str = PyDiaSheet_Str,
  .tp_doc = "returned by dia.register_export() but not used otherwise yet.",
  .tp_methods = PyDiaSheet_Methods,
  .tp_members = PyDiaSheet_Members,
};
