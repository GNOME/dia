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
PyDiaExportFilter_Dealloc(PyDiaExportFilter *self)
{
     PyObject_DEL(self);
}

static int
PyDiaExportFilter_Compare(PyDiaExportFilter *self, PyDiaExportFilter *other)
{
    if (self->filter == other->filter) return 0;
    if (self->filter > other->filter) return -1;
    return 1;
}

static long
PyDiaExportFilter_Hash(PyDiaExportFilter *self)
{
    return (long)self->filter;
}

static PyObject *
PyDiaExportFilter_Str(PyDiaExportFilter *self)
{
    return PyString_FromString(self->filter->description);
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
      "A uniqe name within filters to allow disambiguation.", },
    { NULL }
};

static PyObject *
PyDiaExportFilter_GetAttr(PyDiaExportFilter *self, gchar *attr)
{
    if (!strcmp(attr, "__members__"))
	return Py_BuildValue("[ss]", "name");
    else if (!strcmp(attr, "name"))
	return PyString_FromString(self->filter->description);
    else if (!strcmp(attr, "unique_name"))
	return PyString_FromString(self->filter->unique_name);

    return Py_FindMethod(PyDiaExportFilter_Methods, (PyObject *)self, attr);
}

PyTypeObject PyDiaExportFilter_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "dia.ExportFilter",
    sizeof(PyDiaExportFilter),
    0,
    (destructor)PyDiaExportFilter_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaExportFilter_GetAttr,
    (setattrfunc)0,
    (cmpfunc)PyDiaExportFilter_Compare,
    (reprfunc)0,
    0,
    0,
    0,
    (hashfunc)PyDiaExportFilter_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaExportFilter_Str,
    (getattrofunc)0,
    (setattrofunc)0,
    (PyBufferProcs *)0,
    0L, /* Flags */
    "returned by dia.register_export() but not used otherwise yet.",
    (traverseproc)0,
    (inquiry)0,
    (richcmpfunc)0,
    0, /* tp_weakliszoffset */
    (getiterfunc)0,
    (iternextfunc)0,
    PyDiaExportFilter_Methods, /* tp_methods */
    PyDiaExportFilter_Members, /* tp_members */
    0
};
