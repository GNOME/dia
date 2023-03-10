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
#include "pydia-export.h"

#include <structmember.h> /* PyMemberDef */

PyObject *
PyDiaExportFilter_New(DiaExportFilter *filter)
{
    PyDiaExportFilter *self;

    self = PyObject_NEW(PyDiaExportFilter, &PyDiaExportFilter_Type);

    if (!self) return NULL;
    self->filter = filter;
    return (PyObject *)self;
}


static void
PyDiaExportFilter_Dealloc (PyObject *self)
{
  PyObject_DEL (self);
}


static PyObject *
PyDiaExportFilter_RichCompare (PyObject *self, PyObject *other, int op)
{
  Py_RETURN_RICHCOMPARE (((PyDiaExportFilter *) self)->filter,
                         ((PyDiaExportFilter *) other)->filter,
                         op);
}


static Py_hash_t
PyDiaExportFilter_Hash (PyObject *self)
{
  return (long) ((PyDiaExportFilter *) self)->filter;
}


static PyObject *
PyDiaExportFilter_Str (PyObject *self)
{
  return PyUnicode_FromString (((PyDiaExportFilter *) self)->filter->description);
}

/*
 * "real" member function implementaion ?
 */

static PyMethodDef PyDiaExportFilter_Methods[] = {
    {NULL, 0, 0, NULL}
};

#define T_INVALID -1 /* can't allow direct access due to pyobject->data indirection */
static PyMemberDef PyDiaExportFilter_Members[] = {
    { "name", T_INVALID, 0, RESTRICTED|READONLY,
      "The description for the filter." },
    { "unique_name", T_INVALID, 0, RESTRICTED|READONLY,
      "A unique name within filters to allow disambiguation.", },
    { NULL }
};


static PyObject *
PyDiaExportFilter_GetAttr (PyObject *obj, PyObject *arg)
{
  PyDiaExportFilter *self;
  const char *attr;

  if (PyUnicode_Check (arg)) {
    attr = PyUnicode_AsUTF8 (arg);
  } else {
    goto generic;
  }

  self = (PyDiaExportFilter *) obj;

  if (!g_strcmp0 (attr, "__members__")) {
    return Py_BuildValue ("[ss]", "name", "unique_name");
  } else if (!g_strcmp0 (attr, "name")) {
    return PyUnicode_FromString (self->filter->description);
  } else if (!g_strcmp0 (attr, "unique_name")) {
    return PyUnicode_FromString (self->filter->unique_name);
  }

generic:
  return PyObject_GenericGetAttr (obj, arg);
}


PyTypeObject PyDiaExportFilter_Type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  .tp_name = "dia.ExportFilter",
  .tp_basicsize = sizeof (PyDiaExportFilter),
  .tp_dealloc = PyDiaExportFilter_Dealloc,
  .tp_getattro = PyDiaExportFilter_GetAttr,
  .tp_richcompare = PyDiaExportFilter_RichCompare,
  .tp_hash = PyDiaExportFilter_Hash,
  .tp_str = PyDiaExportFilter_Str,
  .tp_doc = "returned by dia.register_export() but not used otherwise yet.",
  .tp_methods = PyDiaExportFilter_Methods,
  .tp_members = PyDiaExportFilter_Members,
};
