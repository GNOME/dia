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

#include "pydia-cpoint.h"
#include "pydia-object.h"

PyObject *
PyDiaConnectionPoint_New(ConnectionPoint *cpoint)
{
    PyDiaConnectionPoint *self;

    self = PyObject_NEW(PyDiaConnectionPoint, &PyDiaConnectionPoint_Type);

    if (!self) return NULL;
    self->cpoint = cpoint;
    return (PyObject *)self;
}

static void
PyDiaConnectionPoint_Dealloc(PyDiaConnectionPoint *self)
{
     PyMem_DEL(self);
}

static int
PyDiaConnectionPoint_Compare(PyDiaConnectionPoint *self,
			     PyDiaConnectionPoint *other)
{
    if (self->cpoint == other->cpoint) return 0;
    if (self->cpoint > other->cpoint) return -1;
    return 1;
}

static long
PyDiaConnectionPoint_Hash(PyDiaConnectionPoint *self)
{
    return (long)self->cpoint;
}

static PyObject *
PyDiaConnectionPoint_GetAttr(PyDiaConnectionPoint *self, gchar *attr)
{
    if (!strcmp(attr, "__members__"))
	return Py_BuildValue("[sss]", "connected", "object", "pos");
    else if (!strcmp(attr, "pos"))
	return Py_BuildValue("(dd)", self->cpoint->pos.x, self->cpoint->pos.y);
    else if (!strcmp(attr, "object"))
	return PyDiaObject_New(self->cpoint->object);
    else if (!strcmp(attr, "connected")) {
	PyObject *ret;
	GList *tmp;
	gint i;

	ret = PyTuple_New(g_list_length(self->cpoint->connected));
	for (i = 0, tmp = self->cpoint->connected; tmp; i++, tmp = tmp->next)
	    PyTuple_SetItem(ret, i, PyDiaObject_New((Object *)tmp->data));
	return ret;
    }

    PyErr_SetString(PyExc_AttributeError, attr);
    return NULL;
}

PyTypeObject PyDiaConnectionPoint_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "DiaConnectionPoint",
    sizeof(PyDiaConnectionPoint),
    0,
    (destructor)PyDiaConnectionPoint_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaConnectionPoint_GetAttr,
    (setattrfunc)0,
    (cmpfunc)PyDiaConnectionPoint_Compare,
    (reprfunc)0,
    0,
    0,
    0,
    (hashfunc)PyDiaConnectionPoint_Hash,
    (ternaryfunc)0,
    (reprfunc)0,
    0L,0L,0L,0L,
    NULL
};
