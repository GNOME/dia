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

#include <config.h>

#include <glib.h>

#include "pydia-object.h"
#include "pydia-cpoint.h"
#include "pydia-handle.h"
#include "pydia-geometry.h"
#include "pydia-properties.h"

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
PyDiaObject_Destroy(PyDiaObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":DiaObject.destroy"))
	return NULL;

    if (!self->object->ops->destroy) {
	PyErr_SetString(PyExc_RuntimeError,"object does not implement method");
	return NULL;
    }

    self->object->ops->destroy(self->object);
    Py_INCREF(Py_None);
    return Py_None;
}

/* draw */

static PyObject *
PyDiaObject_DistanceFrom(PyDiaObject *self, PyObject *args)
{
    Point point;

    if (!PyArg_ParseTuple(args, "dd:DiaObject.distance_from",
			  &point.x, &point.y))
	return NULL;

    if (!self->object->ops->distance_from) {
	PyErr_SetString(PyExc_RuntimeError,"object does not implement method");
	return NULL;
    }

    return PyFloat_FromDouble(self->object->ops->distance_from(self->object,
							       &point));
}

/* select */

static PyObject *
PyDiaObject_Copy(PyDiaObject *self, PyObject *args)
{
    Object *cp;

    if (!PyArg_ParseTuple(args, ":DiaObject.copy"))
	return NULL;

    if (!self->object->ops->copy) {
	PyErr_SetString(PyExc_RuntimeError,"object does not implement method");
	return NULL;
    }

    cp = self->object->ops->copy(self->object);
    if (cp)
	return PyDiaObject_New(cp);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaObject_Move(PyDiaObject *self, PyObject *args)
{
    Point point;

    if (!PyArg_ParseTuple(args, "dd:DiaObject.move", &point.x, &point.y))
	return NULL;

    if (!self->object->ops->move) {
	PyErr_SetString(PyExc_RuntimeError,"object does not implement method");
	return NULL;
    }

    self->object->ops->move(self->object, &point);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaObject_MoveHandle(PyDiaObject *self, PyObject *args)
{
    PyDiaHandle *handle;
    Point point;
    HandleMoveReason reason;
    ModifierKeys modifiers;

    if (!PyArg_ParseTuple(args, "O!(dd)ii:DiaObject.move_handle",
			  &PyDiaHandle_Type, &handle, &point.x, &point.y,
			  &reason, &modifiers))
	return NULL;

    if (!self->object->ops->move_handle) {
	PyErr_SetString(PyExc_RuntimeError,"object does not implement method");
	return NULL;
    }

    self->object->ops->move_handle(self->object, handle->handle, &point,
				   reason, modifiers);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef PyDiaObject_Methods[] = {
    { "destroy", (PyCFunction)PyDiaObject_Destroy, 1 },
    { "distance_from", (PyCFunction)PyDiaObject_DistanceFrom, 1 },
    { "copy", (PyCFunction)PyDiaObject_Copy, 1 },
    { "move", (PyCFunction)PyDiaObject_Move, 1 },
    { "move_handle", (PyCFunction)PyDiaObject_MoveHandle, 1 },
    { NULL, 0, 0, NULL }
};

static PyObject *
PyDiaObject_GetAttr(PyDiaObject *self, gchar *attr)
{
    if (!strcmp(attr, "__members__"))
	return Py_BuildValue("[sssss]", "bounding_box", "connections",
			     "handles", "properties", "type");
    else if (!strcmp(attr, "type"))
	return PyDiaObjectType_New(self->object->type);
    else if (!strcmp(attr, "bounding_box"))
	return PyDiaRectangle_New(&(self->object->bounding_box), NULL);
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
    } else if (!strcmp(attr, "properties")) {
	return PyDiaProperties_New(self->object);
    }

    return Py_FindMethod(PyDiaObject_Methods, (PyObject *)self, attr);
}

PyTypeObject PyDiaObject_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "DiaObject",
    sizeof(PyDiaObject),
    0,
    (destructor)PyDiaObject_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaObject_GetAttr,
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
PyDiaObjectType_Create(PyDiaObjectType *self, PyObject *args)
{
    Point p;
    gint data = 0;
    gpointer user_data;
    Object *ret;
    Handle *h1 = NULL, *h2 = NULL;
    PyObject *pyret;

    if (!PyArg_ParseTuple(args, "dd|i:DiaObjectType.create", &p.x,&p.y, &data))
	return NULL;
    user_data = GINT_TO_POINTER(data);
    ret = self->otype->ops->create(&p, user_data, &h1, &h2);
    if (!ret) {
	PyErr_SetString(PyExc_RuntimeError, "could not create new object");
	return NULL;
    }
    pyret = PyTuple_New(3);
    PyTuple_SetItem(pyret, 0, PyDiaObject_New(ret));
    if (h1)
	PyTuple_SetItem(pyret, 1, PyDiaHandle_New(h1));
    else {
	Py_INCREF(Py_None);
	PyTuple_SetItem(pyret, 1, Py_None);
    }
    if (h2)
	PyTuple_SetItem(pyret, 2, PyDiaHandle_New(h2));
    else {
	Py_INCREF(Py_None);
	PyTuple_SetItem(pyret, 2, Py_None);
    }
    return pyret;
}

static PyMethodDef PyDiaObjectType_Methods[] = {
    { "create", (PyCFunction)PyDiaObjectType_Create, 1 },
    { NULL, 0, 0, NULL }
};

static PyObject *
PyDiaObjectType_GetAttr(PyDiaObjectType *self, gchar *attr)
{
    if (!strcmp(attr, "__members__"))
	return Py_BuildValue("[ss]", "name", "version");
    else if (!strcmp(attr, "name"))
	return PyString_FromString(self->otype->name);
    else if (!strcmp(attr, "version"))
	return PyInt_FromLong(self->otype->version);

    return Py_FindMethod(PyDiaObjectType_Methods, (PyObject *)self, attr);
}

PyTypeObject PyDiaObjectType_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "DiaObjectType",
    sizeof(PyDiaObjectType),
    0,
    (destructor)PyDiaObjectType_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaObjectType_GetAttr,
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

