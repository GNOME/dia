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

#include "pydia-geometry.h"
#include "pydia-cpoint.h"
#include "pydia-object.h"

#include <structmember.h> /* PyMemberDef */

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
     PyObject_DEL(self);
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
	return Py_BuildValue("[sssss]", "connected", "object", "pos", "flags", "directions");
    else if (!strcmp(attr, "pos"))
	return PyDiaPoint_New(&(self->cpoint->pos));
    else if (!strcmp(attr, "object"))
	return PyDiaObject_New(self->cpoint->object);
    else if (!strcmp(attr, "flags"))
	return PyInt_FromLong(self->cpoint->flags);
    else if (!strcmp(attr, "directions"))
	return PyInt_FromLong(self->cpoint->directions);
    else if (!strcmp(attr, "connected")) {
	PyObject *ret;
	GList *tmp;
	gint i;

	ret = PyTuple_New(g_list_length(self->cpoint->connected));
	for (i = 0, tmp = self->cpoint->connected; tmp; i++, tmp = tmp->next)
	    PyTuple_SetItem(ret, i, PyDiaObject_New((DiaObject *)tmp->data));
	return ret;
    }

    PyErr_SetString(PyExc_AttributeError, attr);
    return NULL;
}

#define T_INVALID -1 /* can't allow direct access due to pyobject->cpoint indirection */
static PyMemberDef PyDiaConnectionPoint_Members[] = {
    { "connected", T_INVALID, 0, RESTRICTED|READONLY,
      "List of Object: read-only list of connected objects" },
    { "object", T_INVALID, 0, RESTRICTED|READONLY,
      "Object: the object owning this connection point" },
    { "pos", T_INVALID, 0, RESTRICTED|READONLY,
      "Point: read-only position of the connection point" },
    { "flags", T_INVALID, 0, RESTRICTED|READONLY,
      "Flags, e.g. CP_FLAGS_MAIN (=0x3)" },
    { "directions", T_INVALID, 0, RESTRICTED|READONLY,
      "Preferred directions away from the object (e.g. DIR_NORTH=0x1)" },
    { NULL }
};

PyTypeObject PyDiaConnectionPoint_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "dia.ConnectionPoint",
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
    (getattrofunc)0,
    (setattrofunc)0,
    (PyBufferProcs *)0,
    0L, /* Flags */
    "One of the major features of Dia are connectable objects. They work by this type accesible "
    "through dia.Object.connections[].",
    (traverseproc)0,
    (inquiry)0,
    (richcmpfunc)0,
    0, /* tp_weakliszoffset */
    (getiterfunc)0,
    (iternextfunc)0,
    0, /* tp_methods */
    PyDiaConnectionPoint_Members, /* tp_members */
    0
};
