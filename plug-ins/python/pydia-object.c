/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Python plug-in for dia
 * Copyright (C) 1999  James Henstridge
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
#include "pydia-cpoint.h"
#include "pydia-handle.h"

PyObject *
PyDiaObject_New(Object *object)
{
    PyDiaObject *self;

    self = PyObject_NEW(PyDiaObject, &PyDiaObject_Type);

    if (!self) return NULL;
    self->object = object;
    return (PyObject *)self;
}

static void
PyDiaObject_Dealloc(PyDiaObject *self)
{
     PyMem_DEL(self);
}

static int
PyDiaObject_Compare(PyDiaObject *self, PyDiaObject *other)
{
    if (self->object == other->object) return 0;
    if (self->object > other->object) return -1;
    return 1;
}

static long
PyDiaObject_Hash(PyDiaObject *self)
{
    return (long)self->object;
}

static PyObject *
PyDiaObject_Str(PyDiaObject *self)
{
    gchar *strname = g_strdup_printf("<DiaObject of type \"%s\" at %lx>",
				     self->object->type->name,
				     (long)self->object);
    PyObject *ret = PyString_FromString(strname);

    g_free(strname);
    return ret;
}

static PyObject *
PyDiaObject_GetAttr(PyDiaObject *self, gchar *attr)
{
    if (!strcmp(attr, "__members__"))
	return Py_BuildValue("[ssss]", "bounding_box", "connections",
			     "handles", "type");
    else if (!strcmp(attr, "type"))
	return PyDiaObjectType_New(self->object->type);
    else if (!strcmp(attr, "bounding_box"))
	return Py_BuildValue("(dddd)", self->object->bounding_box.top,
			     self->object->bounding_box.left,
			     self->object->bounding_box.bottom,
			     self->object->bounding_box.right);
    else if (!strcmp(attr, "handles")) {
	int i;
	PyObject *ret = PyTuple_New(self->object->num_handles);

	for (i = 0; i < self->object->num_handles; i++)
	    PyTuple_SetItem(ret, i, PyDiaHandle_New(self->object->handles[i]));
	return ret;
    } else if (!strcmp(attr, "connections")) {
	int i;
	PyObject *ret = PyTuple_New(self->object->num_connections);

	for (i = 0; i < self->object->num_connections; i++)
	    PyTuple_SetItem(ret, i, PyDiaConnectionPoint_New(
				self->object->connections[i]));
	return ret;
    }

    PyErr_SetString(PyExc_AttributeError, attr);
    return NULL;
}

PyTypeObject PyDiaObject_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "DiaObject",
    sizeof(PyDiaObject),
    0,
    (destructor)PyDiaObject_Dealloc,
    (printfunc)0,
    (getattrfunc)0,
    (setattrfunc)0,
    (cmpfunc)PyDiaObject_Compare,
    (reprfunc)PyDiaObject_Str,
    0,
    0,
    0,
    (hashfunc)PyDiaObject_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaObject_Str,
    0L,0L,0L,0L,
    NULL
};

PyObject *
PyDiaObjectType_New(ObjectType *otype)
{
    PyDiaObjectType *self;

    self = PyObject_NEW(PyDiaObjectType, &PyDiaObjectType_Type);

    if (!self) return NULL;
    self->otype = otype;
    return (PyObject *)self;
}

static void
PyDiaObjectType_Dealloc(PyDiaObjectType *self)
{
     PyMem_DEL(self);
}

static int
PyDiaObjectType_Compare(PyDiaObjectType *self, PyDiaObjectType *other)
{
    if (self->otype == other->otype) return 0;
    if (self->otype > other->otype) return -1;
    return 1;
}

static long
PyDiaObjectType_Hash(PyDiaObjectType *self)
{
    return (long)self->otype;
}

static PyObject *
PyDiaObjectType_Repr(PyDiaObjectType *self)
{
    gchar *strname = g_strdup_printf("<DiaObjectType \"%s\">",
				     self->otype->name);
    PyObject *ret = PyString_FromString(strname);

    g_free(strname);
    return ret;
}

static PyObject *
PyDiaObjectType_Str(PyDiaObjectType *self)
{
    return PyString_FromString(self->otype->name);
}

static PyObject *
PyDiaObjectType_GetAttr(PyDiaObjectType *self, gchar *attr)
{
    if (!strcmp(attr, "__members__"))
	return Py_BuildValue("[ss]", "name", "version");
    else if (!strcmp(attr, "name"))
	return PyString_FromString(self->otype->name);
    else if (!strcmp(attr, "version"))
	return PyInt_FromLong(self->otype->version);

    PyErr_SetString(PyExc_AttributeError, attr);
    return NULL;
}

PyTypeObject PyDiaObjectType_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "DiaObjectType",
    sizeof(PyDiaObjectType),
    0,
    (destructor)PyDiaObjectType_Dealloc,
    (printfunc)0,
    (getattrfunc)0,
    (setattrfunc)0,
    (cmpfunc)PyDiaObjectType_Compare,
    (reprfunc)PyDiaObjectType_Repr,
    0,
    0,
    0,
    (hashfunc)PyDiaObjectType_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaObjectType_Str,
    0L,0L,0L,0L,
    NULL
};

